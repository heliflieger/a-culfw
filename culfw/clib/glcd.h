//*****************************************************************************
//
// Title        : MP3stick - MP3 player
// Authors      : Michael Wolf
// File Name    : 'glcd.h'
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
#ifndef GLCD_H
#define GLCD_H

#include <stdio.h>

/*
  choose a Flash font style 
*/

#define ADVOCUT       // size 8x14
#define AUXDOTBIT     // size 8x14
#define BAUER         // size 8x14
#define DEFAULT       // size 8x14
#define FREON         // size 8x14
#define MS_SANS_SERIF // size 8x14
#define OEM_6x8       // this is a true size 6x8 font
#define OEM_8x14      // size 8x14
#define SYSTEMATIC    // size 8x14
#define TAHOMA        // size 8x14
#define ZELDADX       // size 8x14

// enable graphical functions (line, circle etc.) 
// needs a lot more program memory then, enable only if really used
#define GRAPH_FUNCTIONS

#include "board.h"

// LCD commands
#define LCD_CMD_SWRESET   0x01
#define LCD_CMD_BSTRON    0x03
#define LCD_CMD_SLEEPIN   0x10
#define LCD_CMD_SLEEPOUT  0x11
#define LCD_CMD_PTLON     0x12
#define LCD_CMD_NORON     0x13
#define LCD_CMD_INVON	  0x21
#define LCD_CMD_SETCON    0x25
#define LCD_CMD_DISPON    0x29
#define LCD_CMD_CASET     0x2A
#define LCD_CMD_PASET     0x2B
#define LCD_CMD_RAMWR     0x2C
#define LCD_CMD_RGBSET    0x2D
#define LCD_CMD_PTLAR     0x30
#define LCD_CMD_VSCRDEF   0x33
#define LCD_CMD_MADCTL    0x36
#define LCD_CMD_SEP       0x37
#define LCD_CMD_COLMOD    0x3A

#define COLORMODE_256     0x02
#define COLORMODE_4096    0x03

#define MADCTL_BGR        0x08
#define MADCTL_HORIZ      0x48
#define MADCTL_VERT       0x68

// offset to compensate tolerances on pixel area
#define PIXEL_OFFSET  1
// set window heigth and width in pixel
#define WINDOW_HEIGTH     131
#define WINDOW_WIDTH      131

#define WINDOW_TOP        PIXEL_OFFSET
#define WINDOW_LEFT       PIXEL_OFFSET
#define WINDOW_RIGHT      (WINDOW_WIDTH+PIXEL_OFFSET)
#define WINDOW_BOTTOM     (WINDOW_HEIGTH+PIXEL_OFFSET)

// number of characters in font
#define FONT_CHAR_COUNT   98

// heigth of LCD bar in pixel
#define BAR_HEIGTH        11

// define basic colors
#define BLACK   0x00
#define WHITE   0xFF
#define GREEN   0x1C
#define RED     0xE0
#define BLUE    0x03
#define YELLOW  0xF8

void lcdfunc(char *);
void lcdscroll(char *);

// macros for character positioning with lcd_gotoxy
#define LCD_LINE(line)  line*LCD_FONT_HEIGTH
#define LCD_POS(pos)    pos*LCD_FONT_WIDTH

extern uint8_t background_color;
extern uint8_t text_color;
extern uint8_t lcd_invert;

// Function prototypes
void lcd_sendcmd (uint8_t cmd);
void lcd_senddata (uint8_t data);
extern void lcd_contrast (uint8_t contrast);
extern void lcd_window (uint8_t xp,
			uint8_t yp,
			uint8_t xe,
			uint8_t ye);
extern void lcd_cls (void);
extern void lcd_init (void);
extern void lcd_sleep (void);
extern void lcd_setcolor (uint8_t back,
			  uint8_t text);
extern void lcd_gotoxy (uint8_t x,
			uint8_t y);
extern void lcd_newline (void);
extern void lcd_clrline (uint8_t ypos);
extern void lcd_puts (int8_t * string);
extern void lcd_puts_P (const int8_t * string);
extern int lcd_putchar (char data,
			FILE * stream);
extern void lcd_bargraph (uint8_t len,
			  uint8_t fill,
			  uint8_t fcol,
			  uint8_t bcol);
extern void lcd_batt_icon (uint8_t fill,
			   uint8_t fcol);


#ifdef GRAPH_FUNCTIONS
void swap (uint8_t * a,
	   uint8_t * b);
void lcd_4pixels (uint8_t x,
		  uint8_t y,
		  uint8_t xc,
		  uint8_t yc,
		  uint8_t col);
extern void lcd_pixel (uint8_t x,
		       uint8_t y,
		       uint8_t col);
extern void lcd_line (uint8_t x1,
		      uint8_t y1,
		      uint8_t x2,
		      uint8_t y2,
		      uint8_t col);
extern void lcd_rectangle (uint8_t x1,
			   uint8_t y1,
			   uint8_t x2,
			   uint8_t y2,
			   uint8_t col);
extern void lcd_ellipse (uint8_t xc,
			 uint8_t yc,
			 uint8_t a0,
			 uint8_t b0,
			 uint8_t col);
extern void lcd_circle (uint8_t x,
			uint8_t y,
			uint8_t r,
			uint8_t col);
#endif


#endif // GLCD_H
