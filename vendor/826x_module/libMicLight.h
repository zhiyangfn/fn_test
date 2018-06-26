#ifndef _LIB_MICLIGHT_H
#define _LIB_MICLIGHT_H


void libMicLight_start(void);
void libMicLight_stop(void);

u8 libMicLight_work_thread(void);
void libMicLight_set(u8 musicMode,u8 sensitiveLevel,u8 color_num,u8 led_num,unsigned char* rgb);

void diyRhythmMode(u8 *diy_rgbTab);

#endif
