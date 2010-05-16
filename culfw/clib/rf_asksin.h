#ifndef _RF_ASKSIN_H
#define _RF_ASKSIN_H

#define MAX_ASKSIN_MSG 64

void rf_asksin_init(void);
void rf_asksin_task(void);
void asksin_send(char *in);

#endif
