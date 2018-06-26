#ifndef _APP_LIGHT_H
#define _APP_LIGHT_H

enum _LED_WORK_MODE_
{
	SEND_SOFT0_MODE=0,
	SEND_SOFT1_MODE=1,
	SEND_SOFT2_MODE=2,
};

typedef struct _product_led_
{
	char product[4];
	char led_type;
	char chip_number;
	char led_length;
}product_led;
//typedef struct _time_rgbchange_
//{
//	u8 r_begin;
//	u8 g_begin;
//	u8 b_begin;
//	u8 r_end;
//	u8 g_end;
//	u8 b_end;
//	u8 cycle;		//
//	u32 time;
//}time_rgbchange;


#define MCU_STATUS_PIN		GPIO_PB7
#define READ_MCU_STATUS		 gpio_read(MCU_STATUS_PIN)

void libzled_RebeginChangeOfTime();
void libzled_setConstColor(u8 r,u8 g,u8 b,u8 mode);
void libzled_setColor(u8 r,u8 g,u8 b,u8 luminance ,u8 mode);

void libzled_stop();
u8 libzled_send(u8 *data,u8 bytes,u8 softFlag);
u8 libzled_init();
void libzled_setMaxLuminance(u8 Luminance);
u8 libzled_getMaxLuminance();

u8 start_led_service();
u8 stop_led_service();
void stop_all_led_service(void);//add by fn,apply to colse diy music multiple mode,in 20180623 


u8 libzled_get(void);
void libTime_Onoff_thread();
void libRGBW_testing(void);

extern u16 ChangeRgbTime;
extern u8 g_RGBW_test_enable ;

#endif
