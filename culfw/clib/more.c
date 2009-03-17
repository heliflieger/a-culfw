#include "board.h"
#include "pcf8833.h"            // lcd_putline
#include "fswrapper.h"          // fs
#include "log.h"                // LOG_TIMELEN
#include "qfs.h"
#include "more.h"
#include "joy.h"                // joyfunc
#include "string.h"             // strchr
#include "menu.h"               // menu_redisplay
#include "display.h"

#define MODE_INIT    0
#define MODE_LOG     1
#define MODE_NORMAL  2

static fs_inode_t morefd;
static uint8_t moremode;
static uint8_t morerow;
static uint16_t moreoffset;
static char morecolors[] = { 0xaf, 0xac, 0xfc };
#define BUFLEN (LOG_TIMELEN+1+BODY_LINECHARS)

static void more_handle_joystick(uint8_t key);

static void
morebgcol(void)
{
  if(morerow&1)
    lcd_setbgcol(morecolors[0]&0xf0, morecolors[0]<<4, morecolors[1]&0xf0);
  else
    lcd_setbgcol(morecolors[1]<<4, morecolors[2]&0xf0, morecolors[2]<<4);
  morerow++;
}

/////////////////////////////////
// fill buf with one line and return the offset of the next line or 0 if EOF
static void
moreline(uint8_t dpyline)
{
  char buf[BUFLEN], *bp;
  uint16_t len;

  len = fs_read(&fs, morefd, (uint8_t*)buf, moreoffset, sizeof(buf));

  morebgcol();
  if(len == 0) {
    if(dpyline < 9) {
      buf[0] = 0;
      lcd_putline(dpyline, buf);
    }
    return;
  }

  bp = strchr(buf, '\n');
  if(bp) {
    len = (bp-buf+1);
    moreoffset += len;
    *bp = 0;
  }

  if(moremode == MODE_INIT) {
    moremode = (len > LOG_TIMELEN && buf[4]==' ' && buf[LOG_TIMELEN]==' ') ?
                        MODE_LOG : MODE_NORMAL;
  }
  if(moremode == MODE_LOG) {
    lcd_putline(0, buf);
    lcd_putline(dpyline, buf+LOG_TIMELEN+1);
  } else {
    lcd_putline(dpyline, buf);
  }

  while(!bp) {
    moreoffset += len;
    len = fs_read(&fs, morefd, buf, moreoffset, sizeof(buf));
    if(len == 0)
      return;
    bp = strchr(buf, '\n');
    if(bp) {
      moreoffset += (bp-buf+1);
      break;
    } else {
      moreoffset += len;
    }
  }
  return;
}

void
more(char *fname)
{
  morefd = fs_get_inode(&fs, fname+1);
  if(morefd == 0xffff) {
    lcd_putline(0, "No such file");
    return;
  }
  lcd_invoff();
  moremode = MODE_INIT;
  moreoffset = 0;
  joyfunc = more_handle_joystick;
  morebgcol();
  lcd_putline(0, fname+1);

  for(uint8_t i = 0; i < BODY_LINES; i++)
    moreline(i+1);
  fs_sync(&fs);
}


static int
moreseek_bw(uint8_t lines)
{
  uint8_t buf[BUFLEN], len;

  for(;;) {

    if(moreoffset > sizeof(buf)) {
      len = sizeof(buf);
      moreoffset -= len;
    } else {
      len = (uint8_t)moreoffset;
      moreoffset = 0;
    }

    len = (uint8_t)fs_read(&fs, morefd, buf, moreoffset, len);
    if(len == 0)
      return 0;

    do {
      --len;
      if(buf[len] == '\n') {
        if(lines == 0) {
          moreoffset += len+1;
          return 1;
        }
        lines--;
      }
    } while(len);

    if(moreoffset == 0)
      return (lines == 0);
  }

}


static void
more_handle_joystick(uint8_t key)
{
  // Scrolling up/down.
  if(key == KEY_DOWN) {                 // scroll down
    moreline(9);

  } else if(key == KEY_UP) {            // scroll up

    uint16_t oldmo = moreoffset;
    if(moreseek_bw(BODY_LINES+1)) {
      moreline(10);
      moreoffset = oldmo;
      moreseek_bw(1);
    } else {
      moreoffset = oldmo;
    }

  } else if(key == KEY_RIGHT) {         // Toggle between top and bottom
    uint16_t flen = fs_size(&fs, morefd);

    if(moreoffset == flen) {
      moreoffset = 0;
    } else {
      moreoffset = flen;
      moreseek_bw(BODY_LINES);
    }
    for(uint8_t i = 0; i < BODY_LINES; i++)
      moreline(i+1);

  } else {
    menu_redisplay();
    joyfunc = menu_handle_joystick;
  }
}
