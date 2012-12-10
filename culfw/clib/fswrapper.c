#include "fswrapper.h"
#include "fncollection.h"       // EE_LOGENABLED
#include "display.h"
#include "menu.h"
#include "rf_receive.h"
#include "cc1100.h"
#include <stdlib.h>
#include <string.h>
#include <avr/wdt.h>
#include "clock.h"
#include <avr/wdt.h>
#include "ttydata.h"

#define BUFSIZE 128
fs_t fs;
static fs_size_t filesize, offset;
static fs_inode_t inode;
static void (*oldinfunc)(uint8_t);
static void write_filedata(uint8_t channel);
#ifdef HAS_LCD
static uint8_t isMenu;                  // Menu hack
#endif

//////////////////////////////////
// Input: Filename 
// Return on error: X+errno(hex)+newline
// Return if ok: length (32bit, hex),newline+data
// If filename is . then return all filenames separated with space + newline
void        
read_file(char *in)
{
  uint8_t ole = log_enabled;
  log_enabled = 0;
  if(in[1]==0 || (in[1]=='.' && in[2]==0)) {              // List directory
    char buf[FS_FILENAME+1];
    uint8_t i = 0;
    while(fs_list(&fs, 0, buf, i) == FS_OK) {
      DS(buf);
      DC('/');
      inode = fs_get_inode( &fs, buf );
      filesize = fs_size(&fs, inode);
      DH((uint16_t)((filesize>>16) & 0xffff),4);
      DH((uint16_t)(filesize & 0xffff),4);
      DC(' ');
      i++;
    }
    DNL();

  } else {

    inode = fs_get_inode( &fs, in+1 );
    if(inode == 0xffff) {         // Not found
      DC('X');
      DNL();

    } else {

      filesize = fs_size(&fs, inode);
      DH((uint16_t)((filesize>>16) & 0xffff),4);
      DH((uint16_t)(filesize & 0xffff),4);
      DNL();
      output_flush_func();       // flush as we reuse the buffer

      offset = 0;

      set_ccoff();

      while(offset < filesize) {

        TTY_Tx_Buffer.nbytes = ((filesize-offset) > TTY_BUFSIZE ?
                        TTY_BUFSIZE : filesize-offset);
        TTY_Tx_Buffer.getoff = 0;
        fs_read( &fs, inode, TTY_Tx_Buffer.buf, offset, TTY_Tx_Buffer.nbytes);
        offset += TTY_Tx_Buffer.nbytes;
        output_flush_func();
        wdt_reset();
      }
      rb_reset(&TTY_Tx_Buffer);

      set_txrestore();
    }
  }
  log_enabled = ole;
}


//////////////////////////////////
// Input: length (32bit, hex) + space + filename + newline
// If length is ffffffff then delete file
// Return on error: X+errno(hex)+newline
// Return if ok: length received (32 bit, hex),newline
void
write_file(char *in)
{
  uint8_t ole = log_enabled;
  uint8_t hb[4];
  fs_status_t ret = 0xff;

  if(!strcmp(in+1, "format")) {
    fs_init(&fs, fs.chip, 1);
    ret = 0;
    return;
  }

  if(fromhex(in+1, hb, 4) != 4) {
#ifdef HAS_FS
    log_enabled = hb[0];
    ewb(EE_LOGENABLED, log_enabled);
#endif
    return;
  }

  filesize = ((uint32_t)hb[0]<<24)|
             ((uint32_t)hb[1]<<16)|
             ((uint16_t)hb[2]<< 8)|
                       (hb[3]);

  ret = fs_remove(&fs, in+9);
  if(filesize == 0xffffffff) {                            // Delete only
    fs_sync(&fs);
    goto DONE;
  }

  ret = fs_create( &fs, in+9);
  if(ret != FS_OK || filesize == 0)                       // Create only
    goto DONE;

#ifdef HAS_LCD
  isMenu = !strcmp(in+9, "MENU");
#endif

  inode = fs_get_inode( &fs, in+9 );
  offset = 0;
  oldinfunc = input_handle_func;
  input_handle_func = write_filedata;
  rb_reset(&TTY_Rx_Buffer);

DONE:
  log_enabled = 0;

  if(ret == FS_OK) {
    if(filesize)
      set_ccoff();
    DH((uint16_t)((filesize>>16) & 0xffff),4);
    DH((uint16_t)(filesize & 0xffff),4);
  } else {
    DC('X');
    DH2(ret);
  }
  DNL();

  log_enabled = ole;
  return;
}

static void
write_filedata(uint8_t channel)
{
  uint8_t len = TTY_Rx_Buffer.nbytes;
  uint8_t odc = display_channel;
  display_channel = channel;

  fs_write(&fs, inode, TTY_Rx_Buffer.buf, offset, len);
  if(offset+len == filesize) { // Ready
    input_handle_func = oldinfunc;
    fs_sync(&fs);
#ifdef HAS_LCD
    if(isMenu) {
      menu_init();
      menu_push(0);
    }
#endif
    set_txrestore();

  } else {
    offset += len;
  }
  rb_reset(&TTY_Rx_Buffer);
  display_channel = odc;
}

#if 0 // read/write speed test
void        
test_file(char *in)
{
  fs_status_t ret = 0;
  fs_inode_t inode = 0;
  char *fname = "TESTFILE";
  char buf[32];

  wdt_disable(); 
  uint8_t *p = (uint8_t *)&ticks;
  DH2(p[1]); DH2(p[0]); DNL();
  if(in[1] == 'w') {
    DC('w');

    fs_remove( &fs, fname);
    ret = fs_create( &fs, fname);
    if(ret != FS_OK)
      goto DONE;

    inode = fs_get_inode( &fs, fname );
    for(uint16_t offset = 0; offset < 65500; offset += sizeof(buf)) {
      ret = fs_write(&fs, inode, buf, offset, sizeof(buf));
      if(ret != FS_OK)
        goto DONE;
    }

  } else {

    DC('r');
    inode = fs_get_inode( &fs, fname );
    for(uint16_t offset = 0; offset < 65500; offset += sizeof(buf)) {
      ret = fs_write(&fs, inode, buf, offset, sizeof(buf));
      if(ret != FS_OK)
        goto DONE;
    }

  }
DONE:
  DH2(ret);
  DH2(p[1]); DH2(p[0]); DNL();
  wdt_enable(WDTO_2S); 
}
#endif
