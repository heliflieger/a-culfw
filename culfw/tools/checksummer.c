#include <stdio.h>

#define uint8_t unsigned char
#define int8_t char
#define uint16_t unsigned short
#define _BV(a) (1<<(a))

#define TYPE_EM      'E'
#define TYPE_HRM     'H'        // Hoermann
#define TYPE_FHT     'T'
#define TYPE_FS20    'F'
#define TYPE_KS300   'K'

typedef struct pxx {
     uint8_t state, sync, byteidx, bitidx;
     uint8_t data[12];          // contains parity and checksum, but no sync
     uint16_t zero;             // measured zero duration
     uint16_t avg;              // (zero+one/2), used to decide 0 or 1
} bucket_t;

static bucket_t px1;  // px1: rise-rise, px2: rise-fall
static uint8_t oby, obuf[10];   // Parity-stripped output
static uint8_t nibble;

static uint8_t
parity_even_bit(uint8_t d)
{
  int i,j=0;
  for(i = 0; i < 8; i++)
    if(d & _BV(i))
      j++;
  return j&1;
}

/////////////////
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
cksum3(uint8_t *buf, uint8_t len)               // KS300
{
  uint8_t x = 0, cnt = 0;
  while(len) {
    uint8_t d = buf[--len];
    x ^= (d>>4);
    if(!nibble || cnt)
      x ^= (d&0xf);
    cnt++;
  }
  return x;
}


static uint8_t
analyze(bucket_t *b, uint8_t t)
{
  uint8_t cnt=0, isok = 1, max, iby = 0;
  int8_t ibi=7, obi=7;

  nibble = 0;
  oby = 0;
  max = b->byteidx*8+(7-b->bitidx);
  obuf[0] = 0;
  while(cnt++ < max) {
    uint8_t bit = (b->data[iby] & _BV(ibi)) ? 1 : 0;     // Input bit
    if(ibi-- == 0) {
      iby++;
      ibi=7;
    }

    if(t == TYPE_KS300 && obi == 3) {                           // nibble check
      if(!nibble) {
        if(!bit) {
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
        if(parity_even_bit(obuf[oby]) != bit) {
          isok = 0;
          break;
        }
      }
      if(t == TYPE_EM || t == TYPE_KS300) {
        if(!bit) {
          isok = 0;
          break;
        }
      }
      obuf[++oby] = 0;
      obi = 7;

    } else {                                           // Normal bits
      if(bit) {
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
  else if(isok && t == TYPE_EM && obi == -1)          // missing last stopbit
    oby++;
  else if(nibble) {                                   // half byte msg 
    oby++;
  }
  if(oby == 0)
    isok = 0;
  return isok;
}
/////////////////



char *data[] = {
    // FS20
    "32994004D215B8",

    // ???
    //"4FEF51DEE0",
    //"4FDF51BFE0",
    //"4FFF51FE20",
    //"4FC2D1E7A0",

    // S300TH
    //"884254CE2D9D40",
    //"884B94D72D2C40",
    //"885394CF2D7DC0",
    //"885B94C72D4DC0",
    //"887254D4ADCC40",
    //"887394D72DCFC0",
    //"887B9494AD8C40",
    //"886714F5AD3C40",
  
    // KS300
    //"EC4238C7A1084E54DDFB",
    //"EC4294CEB9884E54E633",
    //"EC46D8B7A1084E54CF7F",
    //"EC47D8A7A1084E54B63F",
    //"EC4A3887A1084E5485B3",
    //"EC4BD8E7A1084E54E671",
    //"EC5A14E5A1084E54DC37",
    //"EC6614A5A1084E54FCF7",

    // EM
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
  int datatype = 0, i, j, cs;
  int bucket_type = 0;
  if(*av[1] != 'F')
    bucket_type = 1;


  for(j = 2; j < ac; j++) {
    char *s = av[j];
    for(i = 0; s[i]; i+=2)
      px1.data[i/2] = (tohex(s[i])<<4) | tohex(s[i+1]);
    if(i==14) {
      px1.byteidx = 6;
      px1.bitidx = 1;
    } else if(i==20) {
      px1.byteidx = 10;
      px1.bitidx = 7;
    } else if(i==10) {
      px1.byteidx = 4;
      px1.bitidx = 4;
    } else {
      px1.byteidx = 11;
      px1.bitidx = 6;
    }

    if(bucket_type == 0) {
      if(analyze(&px1, TYPE_FS20))
        datatype = TYPE_FS20;
      else if(analyze(&px1, TYPE_EM))
        datatype = TYPE_EM;
    } else {
      if(analyze(&px1, TYPE_KS300))
        datatype = TYPE_KS300;
    }

    printf("%s (%c, Nibble: %d)\n", s, datatype, nibble);
    if(datatype) {
      printf("  Brutto: ");
      for(i=0; i < oby; i++) {
        if(i == oby-1 && nibble)
          printf("%x",  obuf[i]);
        else 
          printf("%02x",  obuf[i]);
      }
      printf("\n");

      oby--;
      printf("  Net data (w/o checksum): ");
      for(i=0; i < oby; i++) {
        if(i == oby-1 && nibble)
          printf("%x",  obuf[i]>>4);
        else 
          printf("%02x",  obuf[i]);
      }
      printf("  Reverse: ");
      for(i=oby-1; i >= 0; i--) {
        if(i == oby-1 && nibble)
          printf("%x",  obuf[i]>>4);
        else 
          printf("%02x",  obuf[i]);
      }
      printf("\n");
    }

    printf("  Checksum ");

    cs = obuf[oby];
    if(nibble)
      cs = ((obuf[oby-1] & 0x0f) << 4) | cs;
      
    if(datatype == TYPE_FS20) {
      printf("  F: Comp: %x Sent: %x,", cksum1(6, obuf, oby), cs);
      printf("  T: Comp: %x Sent: %x\n\n", cksum1(12, obuf, oby), cs);
    } else if(datatype == TYPE_EM) { 
      printf("  E: Comp: %x Sent: %x\n\n", cksum2(obuf, oby), cs);
    } else if(datatype == TYPE_KS300) {
      printf("  K: Comp: %x Sent: %x\n\n", cksum3(obuf, oby), cs);
    } else {
      printf("Unknown.\n");
    }
  }

  return 0;
}
