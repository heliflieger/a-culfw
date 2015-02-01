#ifndef __FHT_H
#define __FHT_H

void fhtsend(char *in);
void fht_hook(uint8_t *in);
void fht_tf_timer(uint8_t ind); // fht TF timer method
void fht8v_timer(void);
void fht80b_timer(void);
void fht_init(void);

extern uint16_t fht8v_timeout;
extern uint8_t  fht80b_timeout;
extern uint8_t fht80b_minute;

extern uint8_t fht_hc0, fht_hc1; // Our housecode.
extern uint8_t fht80b_state;

#define FHT_ACTUATOR   0x00
#define FHT_ACK        0x4B
#define FHT_CAN_XMIT   0x53
#define FHT_CAN_RCV    0x54
#define FHT_ACK2       0x69
#define FHT_START_XMIT 0x7D
#define FHT_END_XMIT   0x7E
#define FHT_MINUTE     0x64
#define FHT_HOUR       0x63

#define FHT_DATA       0xff
#define FHT_FOREIGN    0xff

#define FHT_TIMER_DISABLED   0xff

#define FHT_CSUM_START   12

#define FHT_8V_NUM         8    // Needs 2 byte per 8v. For sync must by <14
#define FHT_8V_DISABLED 0xff

#define FHT_TF_START	0x69    // first byte of housecode starts above 0x69
#define FHT_TF_DATA        4  // 4 bytes per FHT 80 TF (HH HH AA BB)
#define FHT_TF_NUM         4  // supported window FHT 80 TF
#define FHT_TF_DISABLED 0xff  // TF not used

// used in clock.c -> keeps the timeout for each TF
extern int16_t fht_tf_timeout_Array[3 * FHT_TF_NUM];

#endif
