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

#define MAXMSG  17
typedef struct pxx {
     uint8_t state, sync, byteidx, bitidx;
     uint8_t data[MAXMSG];          // contains parity and checksum, but no sync
     uint16_t zero;             // measured zero duration
     uint16_t avg;              // (zero+one/2), used to decide 0 or 1
} bucket_t;

static bucket_t px1;  // px1: rise-rise, px2: rise-fall
static uint8_t oby, obuf[10];   // Parity-stripped output
static uint8_t nibble = 0;

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
  uint8_t cnt=0, iserr = 0, max, iby = 0;
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
          iserr = 1;
          break;
        }
        nibble = !nibble;
        continue;
      }
      nibble = !nibble;
    }

    if(obi == -1) {                                    // next byte
      if(t == TYPE_FS20) {
printf(" P%d (%d/%d)\n", bit, cnt, max);
        if(parity_even_bit(obuf[oby]) != bit) {
          iserr = 2;
          break;
        }
      }
      if(t == TYPE_EM || t == TYPE_KS300) {
        if(!bit) {
          iserr = 3;
          break;
        }
      }
      obuf[++oby] = 0;
      obi = 7;

    } else {                                           // Normal bits
printf("%d", bit);
      if(bit) {
        if(t == TYPE_FS20)
          obuf[oby] |= _BV(obi);
        if(t == TYPE_EM || t == TYPE_KS300)            // LSB
          obuf[oby] |= _BV(7-obi);
      }
      obi--;
    }
  }
  if(cnt <= max && !iserr)
    iserr = 4;
  else if(iserr && t == TYPE_EM && obi == -1)          // missing last stopbit
    oby++;
  else if(nibble) {                                   // half byte msg 
    oby++;
  }
  if(oby == 0 && !iserr)
    iserr = 5;
printf("\nCNT: %d, ISERR: %d\n", cnt, iserr);
  return !iserr;
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
fromhex(const char *in, uint8_t *out, uint8_t buflen)
{
  uint8_t *op = out, c, h = 0, fnd, step = 0;
  while((c = *in++)) {
    fnd = 0;
    if(c >= '0' && c <= '9') { h |= c-'0';    fnd = 1; }
    if(c >= 'A' && c <= 'F') { h |= c-'A'+10; fnd = 1; }
    if(c >= 'a' && c <= 'f') { h |= c-'a'+10; fnd = 1; }
    if(!fnd)
      continue;
    if(step++) {
      *op++ = h;
      if(--buflen <= 0)
        return (op-out);
      step = 0;
      h = 0;
    } else {
      h <<= 4;
    }
  }
  return op-out;
}


int
main(int ac, char *av[])
{
  int datatype = 0, i, j, l, cs;
  int bucket_type = 0;
  if(*av[1] != 'F')
    bucket_type = 1;

  px1.byteidx = strtol(av[2], 0, 0);
  px1.bitidx = strtol(av[3], 0, 0);
  printf("%d / %d\n", px1.byteidx, px1.bitidx);

  for(j = 4; j < ac; j++) {
    char *s = av[j];
    l = fromhex(s, px1.data, sizeof(px1.data));
    for(i = 0; i < l; i++)
      printf("%02x", px1.data[i]);
    printf("\n");

    if(bucket_type == 0) {
      if(analyze(&px1, TYPE_FS20))
        datatype = TYPE_FS20;
      else if(analyze(&px1, TYPE_EM))
        datatype = TYPE_EM;
    } else {
      if(analyze(&px1, TYPE_KS300))
        datatype = TYPE_KS300;
    }

    printf("%s (%c, Nibble: %d, OBY: %d)\n", s, datatype, nibble, oby);
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
      printf("  W/o CS: ");
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
