#ifndef _LOG_H
#define _LOG_H

#define LOG_NETTOLINELEN     32
#define LOG_TIMELEN          13 // 0314 09:00:00
#define LOG_LINELEN          (LOG_TIMELEN+1+LOG_NETTOLINELEN+1)
#define LOG_NRFILES           4

void Log(char *);
void displaylog(char *);
void log_init(void);

#endif
