/*
 *
 * (c)2008 Rudolf Koenig 
 * Quasi-FS: Very primitive FS designed for the Atmel 2Mbit flash chip.
 *
 * Why: fs.c from Alexander Neumann (etherrape) is too slow (ca 300B/sec write)
 * and buggy (writing more than 512Byte does not work and reading crashes too).
 * fs.c initialization (not formatting!) is annoyingly slow with between 5 and
 * 17 seconds.
 *
 * In qfs we sacrifice space for simplicity and speed (i.e. for as few as
 * possible flash operations).
 * NOTE: qfs may cause a premature wear out of the flash.
 *
 * "Features" of qfs:
 * - memory requirements: 6+(#DF*22) bytes, program memory: ca 2kB
 * - up to 32 files, each up to 64kB on a 2MB flash (up to the last file which
 *   is 512bytes shorter).
 * - filenames are up to 14Bytes, no directory support
 * - for an at45db161 with 4mBit SPI (fosc/2)
 *   write speed is  26758 Byte/sec, read 27662 Byte/sec
 * - sending/reading the data to/from
 *   USB (USB buffersize 32 byte): write  9523B/sec, read 26000B/sec.
 *   USB (USB buffersize 64 byte): write 21934B/sec, read 26000B/sec.
 *   TCP/IP                        write 23290B/sec, read 26576B/sec.
 *
 * - qfs is using only 512 bytes per flash-block (ignoring 16 bytes / 3%)
 *
 * The interface is compatible with the more advanced fs.c up to the following
 * exception: to ensure that data is written to the flash you have to use
 * fs_sync.
 *
 * It uses extensively the feature of the dataflash chip with its two buffers:
 * DF_BUF1 is used for data caching, DF_BUF2 for caching the superblock, which
 * holds all the inodes.
 *
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <string.h>

#include "display.h"
#include "qfs.h"


////////////////////////////////////////////////////////////////////////
fs_status_t
fs_init(fs_t *fs, df_chip_t chip, uint8_t force)
{
  fs->chip = chip;
  fs->lastpage = 0xffff;
  fs->flags = 0;
  fs->last_inode_index = 0xff;

  df_buf_load(fs->chip, DF_BUF2, QFS_ROOT_PAGE);
  df_wait(fs->chip);
  df_buf_read(fs->chip, DF_BUF2,
                        &fs->last_inode, QFS_MAGIC_OFFSET, sizeof(inode_t));

  // Initialize FS
  if(fs->last_inode.len != QFS_MAGIC_LEN ||
     strncmp(fs->last_inode.name, QFS_MAGIC, QFS_MAGIC_LEN) ||
     force) {

    // First the magic
    fs->last_inode.len = QFS_MAGIC_LEN;
    strncpy(fs->last_inode.name, QFS_MAGIC, QFS_FILENAMESIZE);
    df_buf_write(fs->chip, DF_BUF2,
                        &fs->last_inode, QFS_MAGIC_OFFSET, sizeof(inode_t));

    // Then write each inode with no-name -> unused
    fs->last_inode.name[0] = 0;
    for(uint16_t off = 0; off < QFS_MAGIC_OFFSET; off += sizeof(inode_t))
      df_buf_write(fs->chip, DF_BUF2, &fs->last_inode, off, sizeof(inode_t));

    fs->flags = QFS_ROOTPAGE_MODIFIED;
    fs_sync(fs);
  }
  return FS_OK;
}

////////////////////////////////////////////////////////////////////////
fs_status_t
fs_create(fs_t *fs, char *name)
{
  fs->last_inode_index = 0;
  for(uint16_t off = 0; off < QFS_MAGIC_OFFSET; off += sizeof(inode_t)) {
    df_buf_read(fs->chip, DF_BUF2, &fs->last_inode, off, sizeof(inode_t));

    if(!fs->last_inode.name[0]) {
      fs->last_inode.len = 0;
      strncpy(fs->last_inode.name, name, QFS_FILENAMESIZE);
      df_buf_write(fs->chip, DF_BUF2, &fs->last_inode, off, sizeof(inode_t));
      fs->flags |= QFS_ROOTPAGE_MODIFIED;
      return FS_OK;
    }
    fs->last_inode_index++;

  }
  return FS_EOF;
}

fs_status_t
fs_rename(fs_t *fs, char *from, char *to)
{
  if(*to)
    fs_remove(fs, to);
  if(fs_get_inode(fs, from) == 0xffff)
    return FS_NOSUCHFILE;
  strncpy(fs->last_inode.name, to, QFS_FILENAMESIZE);
  df_buf_write(fs->chip, DF_BUF2, &fs->last_inode,
                         fs->last_inode_index*sizeof(inode_t), sizeof(inode_t));
  fs->flags |= QFS_ROOTPAGE_MODIFIED;
  return FS_OK;
}

////////////////////////////////////////////////////////////////////////
fs_status_t
fs_remove(fs_t *fs, char *name)
{
  return fs_rename(fs, name, "");
}


////////////////////////////////////////////////////////////////////////
fs_status_t
fs_list(fs_t *fs, char *dir, char *buf, fs_index_t index)
{
  fs->last_inode_index = 0;
  for(uint16_t off = 0; off < QFS_MAGIC_OFFSET; off += sizeof(inode_t)) {

    df_buf_read(fs->chip, DF_BUF2, &fs->last_inode, off, sizeof(inode_t));

    if(fs->last_inode.name[0]) {
      if(index == 0) {
        strncpy(buf, fs->last_inode.name, QFS_FILENAMESIZE);
        buf[QFS_FILENAMESIZE] = 0;
        return FS_OK;
      }
      index--;
    }
    fs->last_inode_index++;
  }
  return FS_EOF;
}


////////////////////////////////////////////////////////////////////////
fs_inode_t
fs_get_inode(fs_t *fs, char *file)
{
  if(!strncmp(fs->last_inode.name, file, QFS_FILENAMESIZE))
    return fs->last_inode_index;

  fs->last_inode_index = 0;
  for(uint16_t off = 0; off < QFS_MAGIC_OFFSET; off += sizeof(inode_t)) {
    df_buf_read(fs->chip, DF_BUF2, &fs->last_inode, off, sizeof(inode_t));
    if(!strncmp(fs->last_inode.name, file, QFS_FILENAMESIZE))
      return fs->last_inode_index;
    fs->last_inode_index++;
  }
  return 0xffff;        // Not found
}

////////////////////////////////////////////////////////////////////////
static void
cache_last_inode(fs_t *fs, fs_inode_t inode)
{
  if(fs->last_inode_index != inode) {
    fs->last_inode_index = inode;
    df_buf_read(fs->chip, DF_BUF2, &fs->last_inode,
                inode*sizeof(inode_t), sizeof(inode_t));
  }
}

////////////////////////////////////////////////////////////////////////
static void
cache_page(fs_t *fs, uint16_t page, uint8_t doload)
{
  if(fs->lastpage == page)
    return;

  //DC(doload?'P':'p'); DH(page,2); DNL();
  if(fs->flags & QFS_LASTPAGE_MODIFIED) {
    df_buf_save(fs->chip, DF_BUF1, fs->lastpage);
    fs->flags &= ~QFS_LASTPAGE_MODIFIED;
    df_wait(fs->chip);
  }

  fs->lastpage = page;

  if(doload) {
    df_buf_load(fs->chip, DF_BUF1, page);
    df_wait(fs->chip);
  }
}

static uint16_t
offset2page(uint16_t inode, uint16_t offset)
{
  return 1 + inode*128 + offset/QFS_BLOCKSIZE; // root_page, 128 pages/file
}

////////////////////////////////////////////////////////////////////////
fs_size_t
fs_read(fs_t *fs, fs_inode_t inode,
        void *buf, fs_size_t offset, fs_size_t length)
{
  uint16_t poffset = (uint16_t)offset;  // only the lower bits are used
  uint16_t len = (uint16_t)length;   
  uint16_t tlen = len;
  uint16_t bufoffset = 0;

  cache_last_inode(fs, inode);
  if(poffset > fs->last_inode.len) 
    return 0;
  if(poffset+len > fs->last_inode.len)
    tlen = len = fs->last_inode.len-poffset;

  uint16_t page = offset2page(inode, poffset);
  poffset = poffset % QFS_BLOCKSIZE;

  while(len) {

    cache_page(fs, page, 1);

    uint16_t plen = (poffset+len > QFS_BLOCKSIZE) ? QFS_BLOCKSIZE-poffset : len;
    //DC('r'); DH(page,4); DH(poffset,4); DH(len,2); DNL();
    df_buf_read(fs->chip, DF_BUF1, buf+bufoffset, poffset, plen);
    len -= plen;
    poffset = 0;
    bufoffset += plen;
    page++;
  }

  return tlen;
}

////////////////////////////////////////////////////////////////////////
fs_status_t
fs_write(fs_t *fs, fs_inode_t inode,
         void *buf, fs_size_t offset, fs_size_t length)
{
  if(offset+length > 0xffff) 
    return FS_EOF;
  if(inode == QFS_MAX_INODE-1 && offset+length > 0xfdff) 
    return FS_EOF;

  uint16_t poffset = (uint16_t)offset;  // only the lower bits are used
  uint16_t len = (uint16_t)length;   
  uint16_t oldsize;
  uint16_t bufoffset = 0;

  cache_last_inode(fs, inode);
  oldsize = fs->last_inode.len;

  if(oldsize < poffset+len) {      // Store the length only if changed
    fs->last_inode.len = poffset+len;
    df_buf_write(fs->chip, DF_BUF2, &fs->last_inode,
                inode*sizeof(inode_t), sizeof(uint16_t));
    fs->flags |= QFS_ROOTPAGE_MODIFIED;
  }
  
  uint16_t page = offset2page(inode, poffset);
  poffset = poffset % QFS_BLOCKSIZE;

  uint16_t lastoldpage = offset2page(inode, oldsize);
  if(oldsize % QFS_BLOCKSIZE)
    lastoldpage++;

  while(len) {

    cache_page(fs, page, lastoldpage > page);

    uint16_t plen = (poffset+len > QFS_BLOCKSIZE) ? QFS_BLOCKSIZE-poffset : len;
    df_buf_write(fs->chip, DF_BUF1, buf+bufoffset, poffset, plen);
    fs->flags |= QFS_LASTPAGE_MODIFIED;
    len -= plen;
    poffset = 0;
    bufoffset += plen;
    page++;
  }

  return FS_OK;
}

////////////////////////////////////////////////////////////////////////
fs_size_t
fs_size(fs_t *fs, fs_inode_t inode)
{
  cache_last_inode(fs, inode);
  return fs->last_inode.len;
}


////////////////////////////////////////////////////////////////////////
fs_status_t
fs_sync(fs_t *fs)
{
  if(fs->flags & QFS_LASTPAGE_MODIFIED) {
    df_buf_save(fs->chip, DF_BUF1, fs->lastpage);
    fs->flags &= ~QFS_LASTPAGE_MODIFIED;
    df_wait(fs->chip);
  }

  if(fs->flags & QFS_ROOTPAGE_MODIFIED) {
    df_buf_save(fs->chip, DF_BUF2, QFS_ROOT_PAGE);
    fs->flags &= ~QFS_ROOTPAGE_MODIFIED;
    df_wait(fs->chip);
  }

  return FS_OK;
}
