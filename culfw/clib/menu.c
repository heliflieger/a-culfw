/*
 * (c)2008 Rudolf Koenig 
 * Menu handling for the CUR
 *
 */
#include "qfs.h"                // MENU file
#include "menu.h"
#include "fncollection.h"       // EEPROM offsets
#include "fswrapper.h"          // global fs ponter
#include "pcf8833.h"            // LCD
#include "display.h"            // debugging
#include "joy.h"                // input
#include "ttydata.h"            // callfn
#include "battery.h"            // bat_drawstate
#include "mysleep.h"            // dosleep
#include "fht.h"                // fht_hc
#include <string.h>

#include <avr/eeprom.h>

#define NMENUS     64   // Total number of menu's / macros
#define NITEMS     32   // Maximum number of subitems in a single menu
#define MLINESIZE  48   // Length of one menu line. 32-48 for a Gxxx command
#define MENUSTACK  12   // Number of recursively called menus

static fs_inode_t minode;
static uint16_t menu_filelen;

static uint16_t menu_offset[NMENUS];    // File offsets of all menus
static uint16_t menu_item_offset[NITEMS];  // File offsets of the current items
static uint8_t  menu_stack[MENUSTACK];
static uint8_t  menu_stackidx;          // Stack(!)-Index of current menu
static uint8_t  menu_curitem;           // Current item in current menu
static uint8_t  menu_topitem;           // Top visible item in current menu
static uint8_t  menu_nitems;            // Number of items in the current menu
static uint8_t  menu_cols[5];           // 3x3 nibbles = 4.5 bytes
static uint8_t  menu_lastsel[NMENUS];   // Last menu item selected per menu


static uint16_t menu_get_line(uint16_t offset, uint8_t *buf, uint8_t len);
static void menu_pop(void);
static void menu_getlineword(uint8_t, uint8_t *, uint8_t *, uint8_t);



///////////////////////////////////////////////
void
menu_init()
{
  // Parse the MENU file, cache the menu offsets
  minode = fs_get_inode(&fs, "MENU");
  if(minode == 0xffff)
    return;
  menu_filelen = (uint16_t)fs_size(&fs, minode);
  uint16_t nextoffset, offset = 0;
  uint8_t menu_line[8], idx = 0;

  while((nextoffset = menu_get_line(offset,menu_line,sizeof(menu_line))) != 0){
    if(menu_line[0] == 'M' || menu_line[0] == 'D') {    // Menu or Macro-def
      menu_offset[idx] = offset;
      if(idx < NMENUS)
        idx++;
    }
    offset = nextoffset;
  }
  menu_stackidx = 0;
  joyfunc = menu_handle_joystick;       // parse input
  menu_push(0);
}

static void
menu_setbg(uint8_t row)
{
  if(row&1)
    lcd_setbgcol(menu_cols[1]<<4, menu_cols[2]&0xf0, menu_cols[2]<<4);
  else
    lcd_setbgcol(menu_cols[3]&0xf0, menu_cols[3]<<4, menu_cols[4]&0xf0);
}

void
menu_redisplay()
{
  menu_push(menu_stack[--menu_stackidx]);
}

///////////////////////////////////////////////
// Display a menu
void
menu_push(uint8_t idx)
{
  uint8_t menu_line[MLINESIZE+1], dpybuf[20], picbuf[24];
  uint16_t off;

  menu_stack[menu_stackidx] = idx;
  if(menu_stackidx < MENUSTACK)
    menu_stackidx++;
  else
    return;

  // Title
  off = menu_get_line(menu_offset[idx], menu_line, sizeof(menu_line));

  menu_getlineword(3, menu_line, dpybuf, sizeof(dpybuf));
  if(dpybuf[0]) {
    dpybuf[9] = '0'; dpybuf[10] = 0; // fromhex needs an even number of chars
    fromhex((char *)dpybuf, menu_cols, sizeof(menu_cols));
  } else {
    menu_cols[0] = menu_cols[1] = menu_cols[2] = 
    menu_cols[3] = menu_cols[4] = 0xff;
  }

  menu_getlineword(2, menu_line, dpybuf, sizeof(dpybuf));
  if(!dpybuf[0])
    menu_getlineword(1, menu_line, dpybuf, sizeof(dpybuf));

  lcd_setbgcol(menu_cols[0]&0xf0, menu_cols[0]<<4, menu_cols[1]&0xf0);
  lcd_putline(0, (char *)dpybuf);

  menu_item_offset[0] = off;
  picbuf[0] = 0;
  for(menu_nitems = 1; menu_nitems <= NITEMS; menu_nitems++) {
    off = menu_get_line(off, menu_line, sizeof(menu_line));
    if(menu_line[0] == 'P') {
      strncpy((char*)picbuf, (char*)menu_line, sizeof(picbuf));
      break;
    }
    if(off == 0 || !menu_line[0])
      break;
    if(menu_nitems < NITEMS)
      menu_item_offset[menu_nitems] = off;
  }
  menu_nitems--;

  // Now display the data.
  // Load the last offset
  menu_curitem = menu_lastsel[menu_stack[menu_stackidx-1]];

  // Compute the top line
  if(menu_curitem >= menu_nitems) {     // Invalid/not yet entered menu
    menu_topitem = 0;
    menu_curitem = 0;
  } else {
    if(menu_curitem >= BODY_LINES)
      menu_topitem = menu_curitem-BODY_LINES/2;
    else 
      menu_topitem = 0;
  }

  if(idx == 0)
    lcd_resetscroll();

  uint8_t fullsizepic = 0;
  if(picbuf[0]) {
    uint8_t hb[4];
    fromhex((char *)picbuf+1, hb, 4); // x, y, w, h
    if(hb[2] >= VISIBLE_WIDTH &&
       hb[3] >= VISIBLE_HEIGHT)
      fullsizepic = 1;
  }

  if(!fullsizepic) {
    for(uint8_t i = 0; i < BODY_LINES; i++) {
      if(menu_topitem+i < menu_nitems) {
        menu_get_line(menu_item_offset[menu_topitem+i],
                          menu_line, sizeof(menu_line));
        menu_getlineword(1, menu_line, dpybuf+1, sizeof(dpybuf)-1);
        dpybuf[0] = (menu_topitem+i == menu_curitem ? '>' : ' ');
      } else {
        dpybuf[0] = 0;         // Clear the rest
      }

      menu_setbg(i);
      lcd_putline(i+1, (char *)dpybuf);
    }
  }

  if(picbuf[0])
    lcd_drawpic((char *)picbuf);

#if 0
  if(idx == 0)
    lcd_drawlogo();
#endif
}

///////////////////////////////////////////////
// Pop a menu from the stack
static void
menu_pop()
{
  if(menu_stackidx < 2)
    return;
  menu_stackidx -= 2;
  menu_push(menu_stack[menu_stackidx]);
}

/////////////////////////////////
// fill buf with one line and return the offset of the next line or 0 if EOF
static uint16_t
menu_get_line(uint16_t offset, uint8_t *buf, uint8_t len)
{
  uint8_t *bufp = buf, lbuf[16];

  while(offset < menu_filelen) {
    if(offset+len > menu_filelen)
      len = menu_filelen-offset;

    fs_read(&fs, minode, bufp, offset, len);

    uint8_t off;
    for(off = 0; off < len; off++)
      if(bufp[off] == '\n')
        break;

    if(bufp[off] == '\n') {
      bufp[off] = 0;
      return offset+off+1;
    }
    bufp[len-1] = 0;
    offset += len;
    bufp = lbuf;
    len = sizeof(lbuf);
  }
  return 0;
}

/////////////////////////////////
// Extract the word "wordnr" from the buffer from, and copy it into the
// buffer to, repllacing expressions like <XY> with the Y word from the
// selected menu-item from the menu X from top of the menustack.
static void
menu_getlineword(uint8_t wordnr, uint8_t *frombuf, uint8_t *tobuf, uint8_t max)
{
  uint8_t cnt = 0, sep = frombuf[1];

  // Skip prefix
  while(*frombuf && cnt < wordnr)
    if(*frombuf++ == sep)
      cnt++;

  // Parse data
  while(*frombuf && *frombuf != sep) {

    if((frombuf[0] == '<' && frombuf[3] == '>') ||
       (frombuf[0] == '[' && frombuf[3] == ']')) {
      uint8_t mnu = menu_stack[menu_stackidx - 1 - (frombuf[1]-'0')];
      uint8_t lnr = menu_lastsel[mnu]+1;
      uint8_t line[MLINESIZE+1];

      // Read the menu line from the file
      uint16_t off = menu_get_line(menu_offset[mnu], line, sizeof(line));
      while(lnr-- > 0)
        off = menu_get_line(off, line, sizeof(line));

      // Look for the word in the line
      uint8_t *p = line;
      uint8_t sep2 = p[1];
      wordnr = frombuf[2]-'0';
      cnt = 0;
      while(*p && cnt < wordnr)
        if(*p++ == sep2)
          cnt++;

      // Copy the word
      while(*p && *p != sep2) {
        if(frombuf[0] == '<') { // plain copy
          if(max > 1) {
            max--;
            *tobuf++ = *p;
          }
        } else {                // convert it to hex
          if(max > 2) {
            tohex(*p, tobuf);
            max -=2; tobuf += 2;
          }
        }
        p++;
      }

      frombuf += 3;     // +1 at the end
    }

    else if(*frombuf == '$') {

      if(max > 4) {
        tohex(fht_hc0, tobuf);
        tohex(fht_hc1, tobuf+2);
        tobuf += 4; max -= 4;
      }

    }

    else {    // Plain data: copy it
      if(max > 1) {
        max--;
        *tobuf++ = *frombuf;
      }
    }

    frombuf++;
  }
  *tobuf = 0;
  return;
}

void
menu_handle_joystick(uint8_t key)
{
  uint8_t menu_line[MLINESIZE+1];

  ////////////////////////////////////////
  // Scrolling up/down.
  if(key == KEY_DOWN || key == KEY_UP) {
    menu_setbg(menu_curitem);
    lcd_putchar(menu_curitem-menu_topitem+1,  ' ');

    uint8_t insert_line = 0;

    if(key == KEY_DOWN) { 

      if(menu_curitem == menu_nitems-1)
        menu_curitem = 0;
      else
        menu_curitem++;

      if(menu_curitem - menu_topitem >= BODY_LINES) {
        menu_topitem++;
        insert_line = 9;
      }
    }

    if(key == KEY_UP) {

      if(menu_curitem == 0)
        menu_curitem = menu_nitems-1;
      else
        menu_curitem--;

      if(menu_topitem > menu_curitem) {
        menu_topitem--;
        insert_line = 10;
      }
    }

    menu_setbg(menu_curitem);
    if(insert_line) {
      uint8_t dpybuf[17];
      menu_get_line(menu_item_offset[menu_curitem],
                        menu_line, sizeof(menu_line));
      menu_getlineword(1, menu_line, dpybuf+1, sizeof(dpybuf)-1);
      dpybuf[0] = ' ';

      lcd_putline(insert_line, (char *)dpybuf);
    }
    lcd_putchar(menu_curitem-menu_topitem+1, '>');
  }

  ////////////////////////////////////////
  // Exec current command
  if(key == KEY_RIGHT) {

    // Save the current position
    menu_lastsel[menu_stack[menu_stackidx-1]] = menu_curitem;

    menu_get_line(menu_item_offset[menu_curitem],
                        menu_line, sizeof(menu_line));

    uint8_t arg[MLINESIZE];
    menu_getlineword(2, menu_line, arg, sizeof(arg));

    if(menu_line[0] == 'S') {   // submenu
      uint8_t sm;
      fromhex((char *)arg, &sm, 1);
      menu_push(sm);
    }

    if(menu_line[0] == 'C') {   // Command
      lcd_invon();
      callfn((char *)arg);
      lcd_invoff();
    }

    if(menu_line[0] == 'm') {   // Macro
      uint8_t idx;
      uint16_t off;

      lcd_invon();
      fromhex((char *)arg, &idx, 1);

      off = menu_get_line(menu_offset[idx], menu_line, sizeof(menu_line));
      while(off && menu_line[0]) {
        off = menu_get_line(off, menu_line, sizeof(menu_line));
        if(off == 0 || !menu_line[0])
          break;
        callfn((char *)menu_line);
      }

      lcd_invoff();

    }

  }

  if(key == KEY_LEFT) {         // pop menu stack
    // Save the current position
    menu_lastsel[menu_stack[menu_stackidx-1]] = menu_curitem;
    menu_pop();
  }


  ////////////////////////////////////////
  // Switch display on / off
  if(key == KEY_ENTER) {
    if(lcd_on) {
      if(lcd_on == 0xff) {

        lcd_switch(1);
        lcd_cls();
        lcd_contrast(0xfc);
        menu_stackidx = 0;
        menu_push(0);

      } else {

        dosleep();

      }

    } else {

      lcd_switch(1);

    }
    bat_drawstate();
  }
}
