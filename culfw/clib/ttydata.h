#ifndef _TTYDATA_H_
#define _TTYDATA_H_

typedef struct _fntab {
  unsigned char name;
  void (* const fn)(char *);
} t_fntab;

void analyze_ttydata(void);
void tty_init(void);
uint8_t callfn(char *buf);

#endif
