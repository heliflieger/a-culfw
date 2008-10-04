#ifndef _TTYDATA_H_
#define _TTYDATA_H_

typedef struct _fntab {
  unsigned char name;
  void (* const fn)(char *);
} t_fntab;
extern t_fntab fntab[];

void analyze_ttydata(uint8_t ucCommand);

#endif
