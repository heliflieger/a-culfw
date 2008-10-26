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

#include "fonts/courier_10x17.inc" // Antialiased: big
#define TITLE_FONT                 courier_10x17
#define TITLE_FONT_WIDTH           10
#define TITLE_FONT_HEIGHT          17
#define TITLE_HEIGHT               18
#define TITLE_LINECHARS            (VISIBLE_WIDTH/TITLE_FONT_WIDTH)

#include "fonts/courier_8x14.inc"  // Antialiased: middle
#define BODY_FONT                  courier_8x14
#define BODY_FONT_WIDTH            8
#define BODY_FONT_HEIGHT           14
#define BODY_HEIGHT                (VISIBLE_HEIGHT-TITLE_HEIGHT)
#define BODY_LINES                 (BODY_HEIGHT/BODY_FONT_HEIGHT)
#define BODY_LINECHARS             (VISIBLE_WIDTH/BODY_FONT_WIDTH)

static uint8_t current_contrast;
static uint8_t scroll_y; 

static void lcd_sendcmd (uint8_t cmd);
static void lcd_senddata (uint8_t data);
static void lcd_window (uint8_t xp, uint8_t yp, uint8_t xe, uint8_t ye);
static void drawtext(char *dataptr, uint8_t datalen, uint8_t *font,
                        uint8_t font_width, uint8_t lcd_y, uint8_t font_y,
                        uint8_t rows);
static void lcd_init(void);
static void lcd_cls(void);
static void lcd_scroll(int8_t offset);

/////////////////////////////////////////////////////////////////////
// First byte, hex: Row number. Rest: Text to display, up to 16 char.
// 00:title (big font).
// 01-08:body
// 09:scroll up, add row at bottom
// 0A:scroll down, add row at top
// 0B:redraw screen.
// FF:control functions: Switch LCD on/of, set contrast
void
lcdfunc(char *in)
{
  uint8_t hb[3];
  fromhex(in+1, hb, 3);

  if(hb[0] == 0xFF) {                              // Control: On/Off, Contrast
    if(hb[1] != 0xFF) {

      if(hb[1]) {

        LCD_BL_DDR  |= _BV(LCD_BL_PIN);            // Switch on, init
        LCD_BL_PORT |= _BV(LCD_BL_PIN);
        display_channels |= DISPLAY_GLCD;
        lcd_init();
        lcd_cls();

      } else  {

        LCD_BL_PORT &= ~_BV(LCD_BL_PIN);           // Switch off the display
        lcd_sendcmd (LCD_CMD_SLEEPIN);   
        display_channels &= ~DISPLAY_GLCD;

        return;
      }
    }

    if(hb[2] != 0xFF) {                           // Contrast
      if(hb[2] == 0xFE) {
        current_contrast++;
      } else if (hb[2] == 0xFD) {
        current_contrast--;
      } else {
        current_contrast = hb[2];
      }
      lcd_sendcmd (LCD_CMD_SETCON);
      lcd_senddata (current_contrast);
    }
    return;
  }

  uint8_t row = hb[0];
  if(row == 0) {                                // Title
    drawtext(in+3, TITLE_LINECHARS,
                TITLE_FONT, TITLE_FONT_WIDTH,
                0, 0, TITLE_FONT_HEIGHT);

  } else if(row < 9) {                          // Body, directly adressed

    row--;
    drawtext(in+3, BODY_LINECHARS,
                BODY_FONT, BODY_FONT_WIDTH,
                scroll_y+BODY_FONT_HEIGHT*row,
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

      drawtext(in+3, BODY_LINECHARS,
                  BODY_FONT, BODY_FONT_WIDTH,
                  scroll_y+dpyoffset, fontoffset, CHUNK);

      fontoffset += (row ? CHUNK : -CHUNK);
      counter += CHUNK;
    }

  }

}

static void
lcd_send( uint16_t data )
{
  CLEAR_BIT( SPCR, SPE );		// disable SPI

  CLEAR_BIT( SPI_PORT, SCK);	        // SCK Low
  asm volatile ("nop");

  CLEAR_BIT( LCD_PORT, LCD_CS);	// select Display
  asm volatile ("nop");

  data = data << 7;

  for (uint8_t i=0; i<9; i++) {
       
       if (data & 0x8000) {
            SET_BIT( SPI_PORT, MOSI);
       } else {
            CLEAR_BIT( SPI_PORT, MOSI);
       }

       asm volatile ("nop");
       SET_BIT( SPI_PORT, SCK);	        // SCK Hi
       asm volatile ("nop");
       CLEAR_BIT( SPI_PORT, SCK);	        // SCK Low

       data = data << 1;
  }

  asm volatile ("nop");
  
  SET_BIT( LCD_PORT, LCD_CS);	// deselect Display

  SET_BIT( SPCR, SPE );		// enable SPI
}


static void
lcd_sendcmd (uint8_t cmd)
{
  lcd_send( cmd );
  return;
}

static void
lcd_senddata (uint8_t data)
{
  lcd_send( data | 0x100);
  return;
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

static void
lcd_cls(void)
{
  lcd_window (0, 0, 131, 131);	// set clearance window
  lcd_sendcmd (LCD_CMD_RAMWR);	// write memory
  uint16_t i;
  for(i = 0; i < (132*132/2); i++) {
    lcd_senddata (0xff);       // r g
    lcd_senddata (0xff);       // b r
    lcd_senddata (0xff);       // g b
  }
}

static void
lcd_init (void)
{
  LCD_DIR  |= _BV (LCD_CS);	// config LCD pins
  LCD_PORT |= _BV (LCD_CS);

  DDRE  |= _BV (LCD_RST);
  PORTE |= _BV (LCD_RST);

  PORTE &= ~_BV (LCD_RST);	// LCD hardware reset
  my_delay_us (100);
  PORTE |= _BV (LCD_RST);
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
  lcd_senddata(GBUF_HEIGHT-TITLE_HEIGHT);            // Middle == scroll area
  lcd_senddata(0);              // bottom

  scroll_y = 18;
  lcd_scroll(0);                    // Switch scrolling on
}

void
lcd_scroll(int8_t offset)
{
  scroll_y += offset;
  if(scroll_y < TITLE_HEIGHT)
    scroll_y += (GBUF_HEIGHT-TITLE_HEIGHT);
  if(scroll_y > GBUF_HEIGHT)
    scroll_y -= (GBUF_HEIGHT-TITLE_HEIGHT);

  lcd_sendcmd (LCD_CMD_SEP);    // Switch scrolling on
  lcd_senddata(scroll_y);             // Set first row to the body part
}

 
////////////////////////////////////////////////////////////////////////
// Draw a text-row (or some pixel-rows of this text-row) to the display.
void
drawtext(char *dataptr, uint8_t datalen, uint8_t *font, uint8_t font_width,
        uint8_t lcd_y, uint8_t font_y, uint8_t rows)
{
  while(rows--) {                             // One pixel row

    if(lcd_y >= GBUF_HEIGHT)
      lcd_y -= (GBUF_HEIGHT-TITLE_HEIGHT);
    lcd_window(PIXEL_OFFSET, lcd_y, 
                PIXEL_OFFSET+datalen*font_width-1, lcd_y);
    lcd_y++;
    lcd_sendcmd(LCD_CMD_RAMWR);	// write memory

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

      uint8_t *fp;
      if(data < 33) { // space
        fp = font;
      } else {
        data -= 33;
        fp = font+(94*font_width/2*font_y)+data*font_width/2;
      }

      uint8_t in = 0;
      for (uint8_t x = 0; x < font_width; x++) {

        if(x&1) {

          lcd_senddata(in);
          uint8_t hb = (in&0xf);
          lcd_senddata(hb<<4|hb);

        } else {
          in = pgm_read_byte_near(fp++);

          uint8_t hb = (in>>4);
          lcd_senddata(hb<<4|hb);

        }

      }

    }

    font_y++;
  }
}
