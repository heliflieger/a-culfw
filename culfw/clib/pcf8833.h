#ifndef pcf8833_H
#define pcf8833_H

#include <stdio.h>
#include "board.h"

// LCD commands
#define LCD_CMD_SWRESET   0x01
#define LCD_CMD_BSTRON    0x03
#define LCD_CMD_SLEEPIN   0x10
#define LCD_CMD_SLEEPOUT  0x11
#define LCD_CMD_PTLON     0x12
#define LCD_CMD_NORON     0x13
#define LCD_CMD_INVOFF	  0x20
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

// Visible area
#define PIXEL_OFFSET  1
#define VISIBLE_HEIGHT     130
#define VISIBLE_WIDTH      130
#define GBUF_HEIGHT        132

#define WINDOW_TOP        PIXEL_OFFSET
#define WINDOW_LEFT       PIXEL_OFFSET
#define WINDOW_RIGHT      (VISIBLE_WIDTH+PIXEL_OFFSET)
#define WINDOW_BOTTOM     (VISIBLE_HEIGTH+PIXEL_OFFSET)

#define TITLE_FONT_WIDTH           10
#define TITLE_FONT_HEIGHT          17
#define TITLE_HEIGHT               18
#define TITLE_LINECHARS            (VISIBLE_WIDTH/TITLE_FONT_WIDTH)

#define BODY_FONT_WIDTH            8
#define BODY_FONT_HEIGHT           14
#define BODY_HEIGHT                (VISIBLE_HEIGHT-TITLE_HEIGHT)
#define BODY_LINES                 (BODY_HEIGHT/BODY_FONT_HEIGHT)
#define BODY_LINECHARS             (VISIBLE_WIDTH/BODY_FONT_WIDTH)

void lcdfunc(char *);
void lcd_putline(uint8_t, char *);
void lcd_putchar(uint8_t y, char content);
void lcd_pixel(uint8_t x, uint8_t y, uint16_t col);
void lcd_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint16_t col);
void lcd_cls(void);
void lcd_setbgcol(uint8_t r, uint8_t g, uint8_t b);
void lcd_drawlogo(void);
void lcd_resetscroll(void);
void lcd_invoff(void);
void lcd_invon(void);
void lcd_contrast(uint8_t hb);
void lcd_switch(uint8_t hb);
void lcd_drawpic(char *in);
void lcd_txmon(uint8_t risetime, uint8_t falltime);

extern uint8_t lcd_on;

#endif
