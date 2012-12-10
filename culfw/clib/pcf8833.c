/*
 * Copyright by R.Koenig
 * Inspired by Michael Wolf's 'glcd.c'
 *
 * This driver uses the pcf8833 in 12bit mode to display an 18pixel top line
 * and 8 14 pixel "body" lines wich can be scrolled up and down.
 *
 * License: GPL v2
 */
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "pcf8833.h"
#include "spi.h"
#include "led.h"
#include "delay.h"
#include "display.h"
#include "clock.h"
#include "fncollection.h"          // EEPROM OFFSETS
#include <avr/eeprom.h>

#include "board.h"
#include "fswrapper.h"             // Drawing picture from file

#include "fonts/courier_10x17.inc" // Antialiased: big
#define TITLE_FONT                 courier_10x17

#include "fonts/courier_8x14.inc"  // Antialiased: middle
#define BODY_FONT                  courier_8x14

static uint8_t lcd_scroll_y = 18;
       uint8_t lcd_on = 0xff;
static uint8_t lcd_col_bg0, lcd_col_bg1, lcd_col_bg2;

static void lcd_window (uint8_t xp, uint8_t yp, uint8_t xe, uint8_t ye);
static void drawtext(char *dataptr, uint8_t datalen, const uint8_t *font,
                        uint8_t font_width, uint8_t lcd_y, uint8_t font_y,
                        uint8_t rows);
static void lcd_init(void);
static void lcd_scroll(int8_t offset);

static void lcd_send(uint8_t fbit, uint8_t cmd);
static void lcd_sendcmd(uint8_t cmd);
static void lcd_senddata(uint8_t data);


static void lcd_blk_senddata(uint8_t d);
static void lcd_blk_setup(void);
static void lcd_blk_teardown(void);

void
lcd_switch(uint8_t hb)
{
  if(hb) {
    LCD_BL_DDR  |= _BV(LCD_BL_PIN);            // Switch on, init
#ifdef LCD_BL_PWM
    LCD_BL_PWM = erb((uint8_t*)EE_BRIGHTNESS);
#else
    LCD_BL_PORT |= _BV(LCD_BL_PIN);
#endif
    lcd_init();
    lcd_on = 1;

  } else {
#ifdef LCD_BL_PWM
    LCD_BL_PWM = 0;
    my_delay_ms(1);                            // else the dpy is still on.
#endif
    LCD_BL_PORT &= ~_BV(LCD_BL_PIN);           // Switch off the display

    lcd_sendcmd (LCD_CMD_SLEEPIN);   
    lcd_on = 0;

  }
}

#ifdef LCD_BL_PWM
void
lcd_brightness(uint8_t hb)
{
  int16_t brightness = erb((uint8_t*)EE_BRIGHTNESS);

  if(hb == 0xFE) {
    brightness += 0x20;
  } else if (hb == 0xFD) {
    brightness -= 0x20;
  } else if (hb == 0xFC) {
    //keep the eeprom value
  } else {
    brightness = hb;
  }
  if(brightness < 0)   brightness = 0;
  if(brightness > 255) brightness = 255;

  ewb((uint8_t*)EE_BRIGHTNESS, brightness);
  LCD_BL_PWM = brightness;

  DS_P( PSTR("Brightns:") );
  DU(brightness*100/255, 3);
  DC('%');
  DNL();
}
#endif

void
lcd_contrast(uint8_t hb)
{
  uint8_t contrast = erb((uint8_t*)EE_CONTRAST);

    
  if(hb == 0xFE) {
    contrast--;
  } else if (hb == 0xFD) {
    contrast++;
  } else if (hb == 0xFC) {
    //keep the eeprom value
  } else {
    contrast = hb;
  }
  if(contrast < 40) contrast = 40;
  if(contrast > 80) contrast = 80;

  ewb((uint8_t*)EE_CONTRAST, contrast);
  lcd_sendcmd (LCD_CMD_SETCON);
  lcd_senddata (contrast);

  DS_P( PSTR("Contrast:") );
  DU(100-(contrast-40)*100/40, 3);
  DC('%');
  DNL();
}

/////////////////////////////////////////////////////////////////////
// First byte, hex: Row number if less then 32, Display text in title else
// Rest: Text to display, up to 16 char.
// 00:title (big font).
// 01-08:body
// 09:scroll up, add row at bottom
// 0A:scroll down, add row at top
// 0B:redraw screen.
// FF:control functions: Switch LCD on/of, set contrast
void
lcdfunc(char *in)
{
  uint8_t hb[4];
  uint8_t narg = fromhex(in+1, hb, 4);

  if(narg > 0 && hb[0] == 0xFF) {

    if(hb[1] != 0xFF) lcd_switch(hb[1]);
    if(hb[2] != 0xFF) lcd_contrast(hb[2]);
#ifdef LCD_BL_PWM
    if(hb[3] != 0xFF) lcd_brightness(hb[3]);
#endif

    return;
  }

  if(narg == 0 || hb[0] >=  0x20) {   // Strange lines go to the title
    hb[0] = 0;
    in += 1;
  } else {
    in += 3;
  }
  lcd_putline(hb[0], in);
}

void
lcd_putline(uint8_t row, char *in)
{
  if(row == 0) {                                // Title
    drawtext(in, TITLE_LINECHARS,
                TITLE_FONT, TITLE_FONT_WIDTH,
                0, 0, TITLE_FONT_HEIGHT);

  } else if(row < 9) {                          // Body, directly adressed

    row--;
    drawtext(in, BODY_LINECHARS,
                BODY_FONT, BODY_FONT_WIDTH,
                lcd_scroll_y+BODY_FONT_HEIGHT*row,
                0, BODY_FONT_HEIGHT);

  } else if(row == 9 || row == 10) {            // Scrolling

    if(row == 9) {                              // Up
      row = BODY_LINES-1;

    } else {                                    // Down
      row = 0;

    }

#define CHUNK 1                                 // 2 lines of offscrean area,
                                                // but only usable for up

    uint8_t dpyoffset = (row? (row+1)*BODY_FONT_HEIGHT : 0);
    uint8_t fontoffset = (row ? 0 : BODY_FONT_HEIGHT-1);
    uint8_t counter = 0;

    // Scroll speed is ca 12-14 lines/sec
    while(counter != BODY_FONT_HEIGHT) {

      lcd_scroll(row == 0 ? -CHUNK : CHUNK);

      drawtext(in, BODY_LINECHARS,
                  BODY_FONT, BODY_FONT_WIDTH,
                  lcd_scroll_y+dpyoffset, fontoffset, CHUNK);

      fontoffset += (row ? CHUNK : -CHUNK);
      counter += CHUNK;
    }

  }
}

void
lcd_setbgcol(uint8_t r, uint8_t g, uint8_t b)
{
  lcd_col_bg0 = (r>>4)+1;
  lcd_col_bg1 = (g>>4)+1;
  lcd_col_bg2 = (b>>4)+1;
}

static uint8_t lcd_spi_used;

///////////////////
// "Block" data transfer
static void
lcd_blk_setup(void)
{
  CLEAR_BIT( LCD1_PORT, LCD1_CS);	// select Display
  lcd_spi_used = 0;
}

static void
lcd_blk_senddata( uint8_t data )
{
  // Send one bit "manually"
  if(lcd_spi_used)
    spi_wait();
  lcd_spi_used = 1;
  CLEAR_BIT( SPCR, SPE );		// disable SPI
  CLEAR_BIT( SPI_PORT, SPI_SCLK);	// SCK Low
  SET_BIT( SPI_PORT, SPI_MOSI);
  SET_BIT( SPI_PORT, SPI_SCLK);	        // SCK Hi
  CLEAR_BIT( SPI_PORT, SPI_SCLK);	// SCK Low
  SET_BIT( SPCR, SPE );                 // enable SPI
  SPDR = data;
}

static void
lcd_blk_teardown(void)
{
  if(lcd_spi_used)
    spi_wait();
  SET_BIT( LCD1_PORT, LCD1_CS);         // deselect Display
}



static void
lcd_send(uint8_t fbit, uint8_t data )
{
  CLEAR_BIT(LCD1_PORT, LCD1_CS);	// select Display
  CLEAR_BIT(SPCR, SPE );		// disable SPI
  CLEAR_BIT(SPI_PORT, SPI_SCLK);	// SCK Low
  if(fbit)
    SET_BIT(SPI_PORT, SPI_MOSI);
  else
    CLEAR_BIT(SPI_PORT, SPI_MOSI);
  SET_BIT( SPI_PORT, SPI_SCLK);	        // SCK Hi
  CLEAR_BIT( SPI_PORT, SPI_SCLK);	        // SCK Low
  SET_BIT( SPCR, SPE );                 // enable SPI
  SPDR = data;
  spi_wait();
  SET_BIT( LCD1_PORT, LCD1_CS);         // deselect Display
}


static void
lcd_sendcmd (uint8_t cmd)
{
  lcd_send(0, cmd);
}

static void
lcd_senddata(uint8_t data)
{
  lcd_send(1, data);
}


static void
lcd_window (uint8_t xs,
	    uint8_t ys,
	    uint8_t xe,
	    uint8_t ye)
{
  lcd_sendcmd (LCD_CMD_CASET);	// set X start & end pixel
  lcd_senddata (xs);
  lcd_senddata (xe);

  lcd_sendcmd (LCD_CMD_PASET);	// set Y start & end pixel
  lcd_senddata (ys);
  lcd_senddata (ye);
}

void
lcd_cls(void)
{
  lcd_window (0, 0, 131, 131);	// set clearance window
  lcd_sendcmd (LCD_CMD_RAMWR);	// write memory

  lcd_blk_setup();
  uint16_t i;
  for(i = 0; i < (132*132/2); i++) {
    lcd_blk_senddata(0xff);       // r g
    lcd_blk_senddata(0xff);       // b r
    lcd_blk_senddata(0xff);       // g b
  }
  lcd_blk_teardown();
}

static void
lcd_init (void)
{
  LCD1_DDR  |= _BV (LCD1_CS);	// config LCD pins
  LCD1_PORT |= _BV (LCD1_CS);

  LCD2_DDR  |= _BV (LCD2_RST);
  LCD2_PORT |= _BV (LCD2_RST);

  LCD2_PORT &= ~_BV (LCD2_RST);	// LCD hardware reset
  my_delay_us (100);
  LCD2_PORT |= _BV (LCD2_RST);
  my_delay_us (100);

  lcd_sendcmd (LCD_CMD_SWRESET);	// LCD software reset
  my_delay_us (100);

  lcd_sendcmd (LCD_CMD_SLEEPOUT);	// Sleep_out
  lcd_sendcmd (LCD_CMD_DISPON);	// display on
  lcd_sendcmd (LCD_CMD_BSTRON);	// booster on
  my_delay_us (10);

  lcd_sendcmd (LCD_CMD_MADCTL);	// Memory data access control
  lcd_senddata (0xD0);	        // Mirror X and Y, LAO=1

  lcd_sendcmd(LCD_CMD_VSCRDEF); // Scroll definition: 18 / 124
  lcd_senddata(TITLE_HEIGHT);   // top
  lcd_senddata(GBUF_HEIGHT-TITLE_HEIGHT);  // Middle == scroll area
  lcd_senddata(0);              // bottom

  lcd_scroll(0);                // Switch scrolling on
}

void
lcd_scroll(int8_t offset)
{
  lcd_scroll_y += offset;
  if(lcd_scroll_y < TITLE_HEIGHT)
    lcd_scroll_y += (GBUF_HEIGHT-TITLE_HEIGHT);
  if(lcd_scroll_y > GBUF_HEIGHT)
    lcd_scroll_y -= (GBUF_HEIGHT-TITLE_HEIGHT);

  lcd_sendcmd (LCD_CMD_SEP);    // Switch scrolling on
  lcd_senddata(lcd_scroll_y);             // Set first row to the body part
}

 
////////////////////////////////////////////////////////////////////////
// Draw a text-row (or some pixel-rows of this text-row) to the display.
static void
drawtext(char *dataptr, uint8_t datalen, const uint8_t *font, uint8_t font_width,
        uint8_t lcd_y, uint8_t font_y, uint8_t rows)
{
  while(rows--) {                             // One pixel row

    if(lcd_y >= GBUF_HEIGHT)
      lcd_y -= (GBUF_HEIGHT-TITLE_HEIGHT);
    lcd_window(PIXEL_OFFSET, lcd_y, 
                PIXEL_OFFSET+datalen*font_width-1, lcd_y);
    lcd_y++;
    lcd_sendcmd(LCD_CMD_RAMWR);	// write memory

    lcd_blk_setup();

    uint8_t spacemode = 0;
    for(uint8_t didx = 0; didx < datalen; didx++)  {

      char data;
      if(spacemode) {           // Print space after end of line
        data = ' ';
      } else {
        data = dataptr[didx] & 0x7f;
        if(data == 0) {
          spacemode = 1;
          data = ' ';
        }
      }

      const uint8_t *fp;
      if(data < 33) { // space
        fp = font;
      } else {
        data -= 33;
        fp = font+(94*font_width/2*font_y)+data*font_width/2;
      }

      uint8_t in = 0, h, l;
      for (uint8_t x = 0; x < font_width; x+=2) {

        in = pgm_read_byte_near(fp++);
        h = (in>>4);
        l = (in&0xf);

        lcd_blk_senddata(((h*lcd_col_bg0)&0xf0) | ((h*lcd_col_bg1)>>4));
        lcd_blk_senddata(((h*lcd_col_bg2)&0xf0) | ((l*lcd_col_bg0)>>4));
        lcd_blk_senddata(((l*lcd_col_bg1)&0xf0) | ((l*lcd_col_bg2)>>4));

      }

    }
    lcd_blk_teardown();

    font_y++;
  }
}

///////////////////////////
// Note: no x can be specified. Used for displaying the row selektor >
void
lcd_putchar(uint8_t row, char content)
{
  char data[2];
  data[0] = content, data[1] = 0;

  drawtext(data, 1, BODY_FONT, BODY_FONT_WIDTH,
                lcd_scroll_y+BODY_FONT_HEIGHT*(row-1), 0, BODY_FONT_HEIGHT);
}

////////////////////////////
// Description: Draw one pixel on LCD
void
lcd_pixel(uint8_t x, uint8_t y, uint16_t col)
{
  lcd_window(x, y, x + 1, y + 1);
  lcd_sendcmd(LCD_CMD_RAMWR);	// write memory
  lcd_senddata(col>>8);
  lcd_senddata(col&0xff);
}

static void
swap(uint8_t *a, uint8_t *b)
{
  uint8_t c;
  c = *a;
  *a = *b;
  *b = c;
}

////////////////////////////
// Description: Draw a line using the Bresenham's line algorithm
void
lcd_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint16_t col)
{
  int16_t d, dx, dy;
  int16_t ainc, binc, yinc;
  int16_t x, y;

  if(x1 > x2) {
    swap(&x1, &x2);
    swap(&y1, &y2);
  }

  if(x2 == x1) {
    if(y1 > y2)
      swap(&y1, &y2);
    for(d = y1; d < y2; d++)
      lcd_pixel (x1, d, col);
    return;
  }

  if(y2 > y1)
    yinc = 1;
  else
    yinc = -1;

  dx = x2 - x1;
  dy = abs (y2 - y1);
  d = 2 * dy - dx;
  ainc = 2 * (dy - dx);
  binc = 2 * dy;
  x = x1;
  y = y1;
  lcd_pixel (x, y, col);
  x = x1 + 1;

  do {
    if(d >= 0) {
      y += yinc;
      d += ainc;
    }
    else
      d += binc;
    lcd_pixel (x, y, col);
  } while (++x < x2);
}

void
lcd_resetscroll()
{
  lcd_sendcmd (LCD_CMD_SEP);    // Switch scrolling on
  lcd_scroll_y = 18;
  lcd_senddata(lcd_scroll_y);   // Set first row to the body part
}

#if 0
#include "fonts/house2.inc"        // Logo
void
lcd_drawlogo()
{
  lcd_window(36, 60, 93, 119);
  lcd_sendcmd(LCD_CMD_RAMWR);	// write memory

  uint8_t *fp = house2_58x60;

  lcd_blk_setup();
  for(uint16_t i = 5220; i; i--) {
    uint8_t data = pgm_read_byte_near(fp++);
    lcd_blk_senddata(data);
  }
  lcd_blk_teardown();
}
#endif

//P243c3a3chouse2_58x60
void
lcd_drawpic(char *in)
{
  uint8_t hb[4], buf[128], idx;
  uint16_t offset = 0;

  if(fromhex(in+1, hb, 4) != 4) // x, y, w, h
    return;

  uint16_t fd = fs_get_inode(&fs, in+9);
  if(fd == 0xffff)
    return;
    
  idx = sizeof(buf);
  uint8_t to = hb[2]+hb[2]/2;

  for(uint8_t y = 0; y < hb[3]; y++) {

    lcd_window(hb[0], hb[1]+y, hb[0]+hb[2]-1, hb[1]+y);
    lcd_sendcmd(LCD_CMD_RAMWR);	// write memory
    lcd_blk_setup();

    for(uint8_t x = 0; x < to; x++) {
      if(idx == sizeof(buf)) {
        lcd_blk_teardown();
        offset += fs_read(&fs, fd, buf, offset, sizeof(buf));
        lcd_blk_setup();
        idx = 0;
      }
      lcd_blk_senddata(buf[idx++]);
    }
    lcd_blk_teardown();
  }
}

void
lcd_invon(void)
{
  lcd_sendcmd(LCD_CMD_INVON);
}

void
lcd_invoff(void)
{
  lcd_sendcmd(LCD_CMD_INVOFF);
}

////////////////////////////////////////////////
// Used to dispay Real-Time data, so it should be fast
void
lcd_drcol(uint8_t x, uint8_t ytop, uint8_t ybottom, uint16_t col)
{
  uint8_t c[3];
  
  c[0] = (col>>4) & 0xff;
  c[1] = ((col<<4)&0xf0) | ((col>>8) & 0x0f);
  c[2] = col&0xff;

  if(ybottom < ytop)
    return;
  lcd_window(x, ytop, x, ybottom);
  lcd_sendcmd(LCD_CMD_RAMWR);	// write memory
  lcd_blk_setup();
  int i = 0;
  while(ytop++ <= ybottom) {
    lcd_blk_senddata(c[i++]);
    if(i > 2) i = 0;

    if(ytop & 1) {
      lcd_blk_senddata(c[i++]);
      if(i > 2) i = 0;
    }
  }
  if(ytop & 1)
    lcd_blk_senddata(c[i]);
  lcd_blk_teardown();
}

#include "cc1100.h"

void
lcd_txmon(uint8_t trise, uint8_t tfall)
{
  static uint8_t xoff = WINDOW_LEFT;
  static uint8_t yoff = TITLE_HEIGHT;
  static uint16_t lc = 0xf00;

  // timing is in 16us units
  // we plan to display max 1600us rise+fall intervals
  uint8_t h = BODY_HEIGHT/8;


  trise = h*trise/(100);
  if(trise >= h) trise = h;

  tfall = h*tfall/(100);
  tfall += trise;
  if(tfall >= h) tfall = h-trise;
  tfall = h-tfall;
  trise = h-trise;

  uint8_t rssi = cc1100_readReg(CC1100_RSSI);    //  0..256
  rssi = (rssi >= 128 ? rssi-128 : rssi+128);    // Swap
  if(rssi < 64)                                  // Drop low and high 25%
    rssi = 0;
  else if(rssi >= 192)
    rssi = 15;
  else 
    rssi = (rssi-80)/8;

  lcd_drcol(xoff, yoff,       yoff+tfall, 0xfff);   // Empty
  lcd_drcol(xoff, yoff+tfall, yoff+trise, lc);      // falltime
  lcd_drcol(xoff, yoff+trise, yoff+h-1,   rssi<<4); // risetime, shades of green

  if(++xoff >= WINDOW_RIGHT) {
    xoff = WINDOW_LEFT;
    yoff += h;
    if(yoff >= VISIBLE_HEIGHT) {
      yoff = TITLE_HEIGHT;
      lc = (lc == 0xf00 ? 0x00f : 0xf00);
    }
  }
}
