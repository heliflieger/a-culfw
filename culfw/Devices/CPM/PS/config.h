#ifndef CONFIG_H
#define CONFIG_H

// for USF1000
#define HOUSECODE (0xa5ce)
#define BUTTON (0xaa)

// pressure sensor calibration
#define FOFFSET (0.04)
#define FSCALE (0.09)

// USF1000 emulation
#define USFOFFSET (50.0)
#define USFHEIGHT (95.0)
#define CORR1 (0.0)
#define CORR2 (30.0/22.0)

//#define SEND_PRESSURE


#endif