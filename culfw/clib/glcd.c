//*****************************************************************************
//
// Title        : MP3stick - MP3 player
// Authors      : Michael Wolf
// File Name    : 'glcd.c'
// Date         : January 1, 2006
// Version      : 1.00
// Target MCU   : Atmel AVR ATmega128/1281
// Editor Tabs  : 2
//
// NOTE: The authors in no way will be responsible for damages that you
//       coul'd be using this code.
//       Use this code at your own risk.
//
//       This code is distributed under the GNU Public License
//       which can be found at http://www.gnu.org/licenses/gpl.txt
//
// Change Log
//
// Version  When        Who           What
// -------  ----        ---           ----
// 1.00     01/01/2006  Michael Wolf  Initial Release
//
//*****************************************************************************
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "glcd.h"
#include "spi.h"
#include "led.h"
#include "delay.h"
#include "display.h"

// check which font file should be used, define default
#if 0
#include "fonts/AdvoCut.inc"            // Nicht perfekt
#include "fonts/AuxDotBit.inc"          // doof/klein
#include "fonts/Bauer.inc"              // Fett, haesslich
#include "fonts/Freon.inc"              // Kaputtfett
#include "fonts/MS_Sans_Serif.inc"      // schmal
#include "fonts/OEM_8x14.inc"           // 6x8 mit mehr spacing
#include "fonts/Systematic.inc"         // rundfett, schlecht lesbar
#include "fonts/Tahoma.inc"             // == Default.inc
#include "fonts/ZeldaDX.inc"            // kursiv
#endif

#include "fonts/Default.inc"            // Breit
#include "fonts/OEM_6x8.inc"            // klein

// check for valid bargraph height
#if(BAR_HEIGTH < 5)
    #undef BAR_HEIGTH
#define BAR_HEIGTH  5
    #warning "BAR_HEIGTH set to minimum of 5 pixel"
#endif

// define default LCD contrast
#ifndef LCD_CONTRAST
    #define LCD_CONTRAST 70
#endif

uint8_t background_color = 0;	// actual background color
uint8_t text_color = 0xff;	// actual text color
uint8_t lcd_x = 0;		// actual "cursor" X position
uint8_t lcd_y = 0;		// actual "cursor" Y position
uint8_t set_color = 0;		// used to detect color change request using \a\x in strings
uint8_t lcd_invert = 0;		// flag to invert lcd font

char *flash_font = OEM_6x8_font;
uint8_t font_h = 8, font_w = 6;

void
lcdfunc(char *in)
{
  uint8_t hb[3];    // backlight, contrast, font

  fromhex(in+1, hb, 3);

  if(hb[0]) {

    LCD_BL_DDR  |= _BV(LCD_BL_PIN);            // Switch on, init
    LCD_BL_PORT |= _BV(LCD_BL_PIN);
    lcd_init();
    lcd_setcolor( WHITE, BLACK );
    display_channels |= DISPLAY_GLCD;

  } else  {

    LCD_BL_PORT &= ~_BV(LCD_BL_PIN);           // Switch off the display
    lcd_sendcmd (LCD_CMD_SLEEPIN);   
    display_channels &= ~DISPLAY_GLCD;

    return;
  }

  lcd_contrast( hb[1] );

  if(hb[2]) {
    flash_font = Default_font;
    font_h = 14; font_w = 8;
  } else {
    flash_font = OEM_6x8_font;
    font_h = 8; font_w = 6;
  }
}

void
lcdscroll(char *in)
{
  uint8_t hb[3], r;    // backlight, contrast, font

  r = fromhex(in+1, hb, 3);
  if(r == 3) {
    lcd_sendcmd (LCD_CMD_VSCRDEF);// Scroll definition
    lcd_senddata (hb[0]);
    lcd_senddata (hb[1]);
    lcd_senddata (hb[2]);
  } else if(r == 2) {
    lcd_sendcmd (LCD_CMD_PTLAR);    // Partial area
    lcd_senddata (hb[0]);
    lcd_senddata (hb[1]);
    lcd_sendcmd (LCD_CMD_PTLON);    // Partial area
  } else if(r == 1) {
    lcd_sendcmd (LCD_CMD_SEP);    // Scrolling
    lcd_senddata (hb[0]);
  } else if(r == 0) {
    lcd_sendcmd (LCD_CMD_NORON);    // Scrolling
  }
}

void lcd_send( uint16_t data ) {
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


//*****************************************************************************
// Function: lcd_sendcmd
// Parameters: command byte
// Returns: none.
//
// Description: send a command byte to the LCD
//*****************************************************************************
void
lcd_sendcmd (uint8_t cmd) {

     lcd_send( cmd );
     return;
}

//*****************************************************************************
// Function: lcd_senddata
// Parameters: data byte
// Returns: none.
//
// Description: send a data byte to the LCD
//*****************************************************************************
void
lcd_senddata (uint8_t data)
{
     lcd_send( data | 0x100);
     return;
}

//*****************************************************************************
// Function: lcd_contrast
// Parameters: contrast value
// Returns: none.
//
// Description: set the LCD contrast
//*****************************************************************************
void
lcd_contrast (uint8_t contrast)
{

    lcd_sendcmd (LCD_CMD_SETCON);	// set contrast cmd
    lcd_senddata (contrast);	// set contrast
}

//*****************************************************************************
// Function: lcd_window
// Parameters:  X start
//              Y start
//              X end
//              Y end
// Returns: none.
//
// Description: set the window for display RAM access
//*****************************************************************************
void
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

//*****************************************************************************
// Function: lcd_cls
// Parameters: none.
// Returns: none.
//
// Description: clear complete LCD screen
//              set background_color as required
//*****************************************************************************
void
lcd_cls (void)
{

    lcd_window (0, 0, 131, 131);	// set clearance window

    lcd_sendcmd (LCD_CMD_RAMWR);	// write memory

    uint16_t i;

    // clear screen with background color (full screen 132x132 pixel)
    for (i = 0; i < 0x4410; i++) {
        lcd_senddata (background_color);
    }
}

//*****************************************************************************
// Function: lcd_init
// Parameters: none.
// Returns: none.
//
// Description: Init LCD
//*****************************************************************************
void
lcd_init (void)
{

    LCD_DIR  |= _BV (LCD_CS);	// config LCD pins
    LCD_PORT |= _BV (LCD_CS);

    DDRE  |= _BV (LCD_RST);
    PORTE |= _BV (LCD_RST);

    background_color = WHITE;
    text_color = BLACK;
    set_color = 0;		// no color change request
    lcd_x = PIXEL_OFFSET;	// set home position
    lcd_y = PIXEL_OFFSET;

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
    lcd_senddata (0xC0);	        // X Mirror and BGR format (needed)
//  lcd_senddata (MADCTL_BGR);	        // X Mirror and BGR format (needed)

    lcd_sendcmd (LCD_CMD_COLMOD);	// Colour mode
    lcd_senddata (COLORMODE_256);	//  256 colour mode select

    lcd_sendcmd (LCD_CMD_INVON);	// Non Invert mode
    lcd_sendcmd (LCD_CMD_RGBSET);	// LUT write

    // set Red look-up table
    lcd_senddata (0x00);
    lcd_senddata (0x02);
    lcd_senddata (0x04);
    lcd_senddata (0x06);
    lcd_senddata (0x08);
    lcd_senddata (0x0A);
    lcd_senddata (0x0C);
    lcd_senddata (0x0F);

    // set green look-up table
    lcd_senddata (0x00);
    lcd_senddata (0x02);
    lcd_senddata (0x04);
    lcd_senddata (0x06);
    lcd_senddata (0x08);
    lcd_senddata (0x0A);
    lcd_senddata (0x0C);
    lcd_senddata (0x0F);

    // set Blue look-up table
    lcd_senddata (0x00);
    lcd_senddata (0x04);
    lcd_senddata (0x0B);
    lcd_senddata (0x0F);

    lcd_contrast (LCD_CONTRAST);	// set contrast
}

//*****************************************************************************
// Function: lcd_sleep
// Parameters: none.
// Returns: none.
//
// Description: Put LCD in sleep mode
//*****************************************************************************
void
lcd_sleep (void)
{

    background_color = 0x00;	// black
    lcd_cls ();

    lcd_sendcmd (LCD_CMD_SLEEPIN);	// set LCD sleep mode
}

//*****************************************************************************
// Function: lcd_setcolor
// Parameters: background color
//             text color
// Returns: none.
//
// Description: Set background and text colors.
//*****************************************************************************
void
lcd_setcolor (uint8_t back,
	      uint8_t text)
{

    background_color = back;
    text_color = text;
}

//*****************************************************************************
// Function: lcd_gotoxy
// Parameters: X position
//             Y position
// Returns: none.
//
// Description: Move pixel pointer to X,Y position
//*****************************************************************************
void
lcd_gotoxy (uint8_t x,
	    uint8_t y)
{

    lcd_x = x + PIXEL_OFFSET;
    lcd_y = y + PIXEL_OFFSET;

    if (lcd_x >= WINDOW_RIGHT)
        lcd_x = WINDOW_RIGHT-1;
    if (lcd_y >= WINDOW_BOTTOM)
        lcd_y = WINDOW_BOTTOM-1;
}

//*****************************************************************************
// Function: lcd_newline
// Parameters: none.
// Returns: none.
//
// Description: Move pixel pointer to new text line
//*****************************************************************************
void
lcd_newline (void)
{

    lcd_x = WINDOW_LEFT;
    if (lcd_y < WINDOW_BOTTOM-font_h)
        lcd_y += font_h;
}

//*****************************************************************************
// Function: lcd_clrline
// Parameters: none.
// Returns: none.
//
// Description: Clear one line on lcd
//*****************************************************************************
void
lcd_clrline (uint8_t ypos)
{

    lcd_gotoxy (2, ypos * font_h);
    for (uint8_t i = 0; i < 15; i++) {
        lcd_putchar (' ', NULL);
    }
    lcd_gotoxy (2, ypos * font_h);
}

//*****************************************************************************
// Function: lcd_puts
// Parameters: Text string in RAM
// Returns: none.
//
// Description: Print RAM string on LCD
//*****************************************************************************
void
lcd_puts (int8_t * string)
{

    while (*string)
        lcd_putchar (*string++, NULL);
}

//*****************************************************************************
// Function: lcd_puts_P 
// Parameters: Text string in program memory
// Returns: none.
//
// Description: Print Flash string to LCD
//*****************************************************************************
void
lcd_puts_P (const int8_t * string)
{

    while (pgm_read_byte (&*string))
        lcd_putchar (pgm_read_byte (&*string++), NULL);
}

//*****************************************************************************
// Function: lcd_putchar
// Parameters: character byte
// Returns: none.
//
// Description: Print one character on LCD
//*****************************************************************************
int
lcd_putchar (char data,
	     FILE * stream)
{
    // check if text color change was requested
    if (set_color) {
        text_color = data;
        set_color = 0;
        return 0;
    }

    data &= 0x7F;		// limit to standard ASCII code

    // set LF/CR on column overflow
    if ((lcd_x + font_w) >= WINDOW_RIGHT)
        lcd_newline ();
    // set HOME on row overflow
    if ((lcd_y + font_h) > WINDOW_BOTTOM) {
      lcd_y = WINDOW_TOP;
      lcd_cls();
    }

    switch (data) {
            // change text color to value of data byte
        case '\a':
            set_color = data;
            break;
            // set LF
        case '\n':
            lcd_newline ();
            break;
            // set CR
        case '\r':
            lcd_x = PIXEL_OFFSET;
            break;
            // print char by default
        default:
            data -= 30;		// subtract offset to first char

            // set RAM window to print character
            lcd_window (lcd_x, lcd_y, lcd_x + (font_w - 1), lcd_y + (font_h - 1));	// set clearance window

            lcd_sendcmd (LCD_CMD_RAMWR);	// write memory
      
            uint8_t temp_char;
            // write font_h*font_w bytes to LCD RAM
            for (uint8_t y = 0; y < font_h; y++) {
                // read font data from FLASH look-up table
                temp_char = pgm_read_byte_near (&flash_font[(data*font_h) + y]);

                if (lcd_invert)
                    temp_char ^= 0xFF;

                for (uint8_t x = 0; x < font_w; x++) {
                    // check if pixel should be set or cleared
                    if ((temp_char & _BV (7)) == _BV (7))
                        lcd_senddata (text_color);
                    else
                        lcd_senddata (background_color);
                    temp_char <<= 1;	// next pixel
                }
            }
            lcd_x += font_w;	// next char
    }				// end switch

    return 0;
}

//*****************************************************************************
// Function: lcd_bargraph
// Parameters: Bar length
//             Bar fill length
//             Foreground color
//             Background color
// Returns: none.
//
// Description: Draw an bar graph on LCD with given length, fill level and
//              colors.
//              Define bar graph heigth in pixel in glcd.h
//*****************************************************************************
void
lcd_bargraph (uint8_t len,
	      uint8_t fill,
	      uint8_t fcol,
	      uint8_t bcol)
{

    // check for limits
    if (len > WINDOW_WIDTH)
        len = WINDOW_WIDTH;
    if (fill > len)
        fill = len;

    lcd_sendcmd (LCD_CMD_MADCTL);	// Memory data acces control
    lcd_senddata (MADCTL_VERT);	// X Mirror + vertical addressing mode

    // set display RAM window
    lcd_window (lcd_x, lcd_y, lcd_x + len, lcd_y + BAR_HEIGTH - 1);

    lcd_sendcmd (LCD_CMD_RAMWR);	// write memory

    for (uint8_t x = 0; x < len - 1; x++) {
        // draw bar
        if (x < fill) {
            lcd_senddata (fcol - 0x04);
            lcd_senddata (fcol + 0x84);
            lcd_senddata (fcol + 0x64);
            lcd_senddata (fcol + 0x44);
            lcd_senddata (fcol + 0x20);
            lcd_senddata (fcol);
            lcd_senddata (fcol + 0x20);
            lcd_senddata (fcol + 0x44);
            lcd_senddata (fcol + 0x64);
            lcd_senddata (fcol + 0x84);
            lcd_senddata (fcol - 0x04);
        }
        else {
            lcd_senddata (bcol - 0x44);
            lcd_senddata (bcol);
            lcd_senddata (bcol);
            lcd_senddata (bcol);
            lcd_senddata (bcol);
            lcd_senddata (bcol);
            lcd_senddata (bcol);
            lcd_senddata (bcol);
            lcd_senddata (bcol);
            lcd_senddata (bcol);
            lcd_senddata (bcol - 0x44);
        }
    }

    lcd_sendcmd (LCD_CMD_MADCTL);	// Memory data access control
    lcd_senddata (MADCTL_HORIZ);	// X Mirror and BGR format (needed)
}


//*****************************************************************************
// Function: lcd_batt_icon
// Parameters: Battery fill length
//             Fill color
// Returns: none.
//
// Description: Draw the battery icon on LCD with given fill level and
//              color.
//*****************************************************************************
void
lcd_batt_icon (uint8_t fill,
	       uint8_t fcol)
{

    if (fill > 16 - 1)
        fill = 16 - 1;

    lcd_sendcmd (LCD_CMD_MADCTL);	// Memory data acces control
    lcd_senddata (MADCTL_VERT);	// X Mirror + vertical addressing mode

    // set display RAM window
    lcd_window (lcd_x, lcd_y, lcd_x + 18, lcd_y + 9 - 1);

    lcd_sendcmd (LCD_CMD_RAMWR);	// write memory

    // battery frame start line
    for (uint8_t y = 0; y < 9; y++) {
        lcd_senddata (BLACK);
    }

    for (uint8_t x = 0; x < 16 - 1; x++) {
        lcd_senddata (BLACK);	// battery frame top line
        // draw battery
        for (uint8_t y = 0; y < 9 - 2; y++) {
            if (x < fill)
                lcd_senddata (fcol);	// color depends on fill level
            else
                lcd_senddata (background_color);
        }
        lcd_senddata (BLACK);	// battery frame bottom line
    }

    // battery frame end line
    for (uint8_t y = 0; y < 9; y++) {
        lcd_senddata (BLACK);
    }

    // battery + contact
    lcd_senddata (background_color);
    lcd_senddata (background_color);
    lcd_senddata (BLACK);
    lcd_senddata (BLACK);
    lcd_senddata (BLACK);
    lcd_senddata (BLACK);
    lcd_senddata (BLACK);
    lcd_senddata (background_color);
    lcd_senddata (background_color);

    lcd_sendcmd (LCD_CMD_MADCTL);	// Memory data access control
    lcd_senddata (MADCTL_HORIZ);	// X Mirror and BGR format (needed)
}

#ifdef GRAPH_FUNCTIONS
//*****************************************************************************
// Function: swap
// Parameters: bytes to swap
// Returns: none.
//
// Description: swap the content of the given bytes
//*****************************************************************************
void
swap (uint8_t * a,
      uint8_t * b)
{

    uint8_t c;
    c = *a;
    *a = *b;
    *b = c;
}

//*****************************************************************************
// Function: lcd_4pixel
// Parameters: pixel center
//             pixel offset
//             pixel color
// Returns: none.
//
// Description: Draw 4 pixels on LCD
//*****************************************************************************
void
lcd_4pixels (uint8_t x,
	     uint8_t y,
	     uint8_t xc,
	     uint8_t yc,
	     uint8_t col)
{

    lcd_pixel (xc + x, yc + y, col);
    lcd_pixel (xc - x, yc + y, col);
    lcd_pixel (xc + x, yc - y, col);
    lcd_pixel (xc - x, yc - y, col);
}

//*****************************************************************************
// Function: lcd_pixel
// Parameters: X position
//             Y position
//             Pixel color
// Returns: none.
//
// Description: Draw one pixel on LCD
//*****************************************************************************
void
lcd_pixel (uint8_t x,
	   uint8_t y,
	   uint8_t col)
{

    lcd_window (x, y, x + 1, y + 1);
    lcd_sendcmd (LCD_CMD_RAMWR);	// write memory
    lcd_senddata (col);
}

//*****************************************************************************
// Function: lcd_line
// Parameters: X start
//             Y start
//             X end
//             Y end
//             Color
// Returns: none.
//
// Description: Draw a line using the Bresenham's line algorithm
//*****************************************************************************
void
lcd_line (uint8_t x1,
	  uint8_t y1,
	  uint8_t x2,
	  uint8_t y2,
	  uint8_t col)
{

    int16_t d, dx, dy;
    int16_t ainc, binc, yinc;
    int16_t x, y;


    if (x1 > x2) {
        swap (&x1, &x2);
        swap (&y1, &y2);
    }

    if (x2 == x1) {
        if (y1 > y2)
            swap (&y1, &y2);
        for (d = y1; d < y2; d++)
            lcd_pixel (x1, d, col);
        return;
    }

    if (y2 > y1)
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
        if (d >= 0) {
            y += yinc;
            d += ainc;
        }
        else
            d += binc;
        lcd_pixel (x, y, col);
    } while (++x < x2);
}

//*****************************************************************************
// Function: lcd_rectangle
// Parameters: X start
//             Y start
//             X end
//             Y end
//             Color
// Returns: none.
//
// Description: Draw a rectangle
//*****************************************************************************
void
lcd_rectangle (uint8_t x1,
	       uint8_t y1,
	       uint8_t x2,
	       uint8_t y2,
	       uint8_t col)
{

    lcd_line (x1, y1, x2, y1, col);
    lcd_line (x1, y2, x2, y2, col);
    lcd_line (x1, y1, x1, y2, col);
    lcd_line (x2, y1, x2, y2, col);
}

//*****************************************************************************
// Function: lcd_ellipse
// Parameters: X center
//             Y center
//             Width
//             Heigth
//             Color
// Returns: none.
//
// Description: Draw an ellipse using the Bresenham's ellipse algorithm
//*****************************************************************************
void
lcd_ellipse (uint8_t xc,
	     uint8_t yc,
	     uint8_t a0,
	     uint8_t b0,
	     uint8_t col)
{

    int16_t x, y;
    float a, b;
    float asqr, twoasqr;
    float bsqr, twobsqr;
    float d, dx, dy;

    x = 0;
    y = b0;
    a = a0;
    b = b0;
    asqr = a * a;
    twoasqr = 2 * asqr;
    bsqr = b * b;
    twobsqr = 2 * bsqr;

    d = bsqr - asqr * b + asqr / 4;
    dx = 0;
    dy = twoasqr * b;

    while (dx < dy) {
        lcd_4pixels (x, y, xc, yc, col);
        if (d > 0) {
            y--;
            dy -= twoasqr;
            d -= dy;
        }
        x++;
        dx += twobsqr;
        d = d + bsqr + dx;
    }

    d = d + (3 * (asqr - bsqr) / 2 - (dx + dy)) / 2;

    while (y >= 0) {
        lcd_4pixels (x, y, xc, yc, col);
        if (d < 0) {
            x++;
            dx += twobsqr;
            d += dx;
        }
        y--;
        dy -= twoasqr;
        d = d + asqr - dy;
    }
}

//*****************************************************************************
// Function: lcd_circle
// Parameters: X center
//             Y cender
//             Radius
//             Color
// Returns: none.
//
// Description: Draw a circle
//*****************************************************************************
void
lcd_circle (uint8_t x,
	    uint8_t y,
	    uint8_t r,
	    uint8_t col)
{

    lcd_ellipse (x, y, r, r, col);
}
#endif
