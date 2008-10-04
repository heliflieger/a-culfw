#include <stdio.h>

#define uint8_t unsigned char
#define int8_t char
#define uint16_t unsigned short
#define _BV(a) (1<<(a))

#define TYPE_FS20    'F'
#define TYPE_FHT     'T'
#define TYPE_EM      'E'
#define TYPE_HMS     'H'
#define TYPE_KS300   'K'

typedef struct pxx {
     uint8_t state, sync, byteidx, bitidx;
     uint8_t data[12];          // contains parity and checksum, but no sync
     uint16_t zero;             // measured zero duration
     uint16_t avg;              // (zero+one/2), used to decide 0 or 1
} px_t;

static px_t px1;  // px1: rise-rise, px2: rise-fall
static uint8_t oby, obuf[10];   // Parity-stripped output

static uint8_t
parity_even_bit(uint8_t d)
{
  int i,j=0;
  for(i = 0; i < 8; i++)
    if(d & _BV(i))
      j++;
  return j&1;
}

static uint8_t
cksum1(uint8_t s, uint8_t *buf, uint8_t len)    // FS20 / FHT
{
  while(len)
    s += buf[--len];
  return s;
}

static uint8_t
cksum2(uint8_t *buf, uint8_t len)               // EM
{
  uint8_t s = 0;
  while(len)
    s ^= buf[--len];
  return s;
}

static uint8_t
cksum3(uint8_t *buf, uint8_t len)
{
  uint8_t x = 0, s=5;
  while(len) {
    uint8_t d = buf[--len];
    uint8_t n = d>>4;
    x ^= n;
    s += n;

    n = d&0xf;
    x ^= n;
    s += n;
  }
  printf("->%02x\n", s);
  return (s<<4)|x;
}


static uint8_t
analyze(volatile px_t *p, uint8_t t)
{
  uint8_t cnt=0, isok = 1, max, iby = 0, nibble=1;
  int8_t ibi=7, obi=7;

  oby = 0;
  max = p->byteidx*8+(7-p->bitidx);
  obuf[0] = 0;
  while(cnt++ < max) {
    uint8_t b = (p->data[iby] & _BV(ibi)) ? 1 : 0;     // Input bit
    if(ibi-- == 0) {
      iby++;
      ibi=7;
    }
    if(t == TYPE_KS300 && obi == 3) {                           // nibble check
      if(nibble) {
        if(!b) {
          isok = 0;
          break;
        }
        nibble = !nibble;
        continue;
      }
      nibble = !nibble;
    }

    if(obi == -1) {                                    // next byte
      if(t == TYPE_FS20) {
        if(parity_even_bit(obuf[oby]) != b) {
          isok = 0;
          break;
        }
      }
      if(t == TYPE_EM || t == TYPE_KS300) {
        if(!b) {
          isok = 0;
          break;
        }
      }
      obuf[++oby] = 0;
      obi = 7;

    } else {                                           // Normal bits
      if(b) {
        if(t == TYPE_FS20)
          obuf[oby] |= _BV(obi);
        if(t == TYPE_EM || t == TYPE_KS300)            // LSB
          obuf[oby] |= _BV(7-obi);
      }
      obi--;
    }
  }
  if(cnt <= max)
    isok = 0;
  else if(isok && t == TYPE_EM && obi == -1)           // missing last stopbit
    oby++;
  if(oby == 0)
    isok = 0;
  return isok;
}


char *data[] = {
    //"884254CE2D9D40",
    //"884B94D72D2C40",
    //"885394CF2D7DC0",
    //"885B94C72D4DC0",
    //"887254D4ADCC40",
    //"887394D72DCFC0",
    //"887B9494AD8C40",
    //"886714F5AD3C40",
  
    //"EC4238C7A1084E54DDFB",
    //"EC4294CEB9884E54E633",
    //"EC46D8B7A1084E54CF7F",
    //"EC47D8A7A1084E54B63F",
    //"EC4A3887A1084E5485B3",
    //"EC4BD8E7A1084E54E671",
    //"EC5A14E5A1084E54DC37",
    //"EC6614A5A1084E54FCF7",

    //"40D04066B44804020100C800",
    //"40D07FA6B44804020100B780",
    //"40D072A03CC804020100F380",
    //"40D077A7344F4403A500BC80",
    //"80C054B7B6DF04033F00FE80",
    //"80C044A6B6D88402B580C000",
  0
};

int
tohex(char a)
{
  if(a >= '0' && a <= '9')
    return a-'0';
  else
    return a-'A'+10;
}

int
main(int ac, char *av[])
{
  int datatype = 0, i, j;

  for(j = 1; j < ac; j++) {
    char *s = av[j];
    for(i = 0; s[i]; i+=2)
      px1.data[i/2] = (tohex(s[i])<<4) | tohex(s[i+1]);
    if(i==14) {
      px1.byteidx = 6;
      px1.bitidx = 5;
    } else if(i==20) {
      px1.byteidx = 10;
      px1.bitidx = 7;
    } else {
      px1.byteidx = 11;
      px1.bitidx = 6;
    }

    if(analyze(&px1, TYPE_FS20))
      datatype = TYPE_FS20;
    else if(analyze(&px1, TYPE_EM))
      datatype = TYPE_EM;
    else if(analyze(&px1, TYPE_KS300))
      datatype = TYPE_KS300;

    printf("%s (%c)\n", s, datatype);
    if(datatype) {
      oby--;
      for(i=0; i < oby; i++)
        printf("%02x",  obuf[i]);
      printf("  ");
      for(i=oby-1; i >= 0; i--)
        printf("%02x",  obuf[i]);
      printf("\n");
    }

    if(datatype == TYPE_FS20) {
      printf("CkF:  Comp: %x Sent: %x\n\n", cksum1(6, obuf, oby), obuf[oby]);
      printf("CkT: Comp: %x Sent: %x\n\n", cksum1(12, obuf, oby), obuf[oby]);
    }
    if(datatype == TYPE_EM) { 
      printf("CkE: Comp: %x Sent: %x\n\n", cksum2(obuf, oby), obuf[oby]);
    }
    if(datatype == TYPE_KS300)
      printf("CkK: Comp: %x Sent: %x\n\n", cksum3(obuf, oby), obuf[oby]);
  }

  return 0;
}
