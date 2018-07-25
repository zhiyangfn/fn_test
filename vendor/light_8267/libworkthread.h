/*
 * libworkthread.h
 *
 *  Created on: 2018-7-17
 *      Author: FN
 */

#ifndef LIBWORKTHREAD_H_
#define LIBWORKTHREAD_H_
#include "../../proj/tl_common.h"

void masterWorkThread(void);
void setWorkStatus(u8 cmd,u8 *params,u8 params_len);
void getNotifyData(u8 *get_notify_data_buf);
void setDefaultWorkStatus(void);
void closeAllLed(void);
void modeSetInit(void);
void setLedStatus(u8 on_off);
u8 getLedStatus(void);


enum _status_station_{
	DEFAULT_MODE,
	SCENE_MODE,
	DIY_MODE,
	WHITE_MODE
};

typedef struct _scene_mode_t
{
	char mode;	

}scene_mode_t;

typedef struct _diy_mode_t
{
	char serial_id;//diy serial id number,set by phone and return to phone
	char diy_master_mode;
	char diy_speed;
	char diy_color[4][3];//

}diy_mode_t;

typedef struct _ct_white_mode_t
{
	char cool_white_lum;
	char warm_white_lum;
}ct_white_mode_t;

typedef struct _color_mode_t
{
	char color_RGB[3];
}color_mode_t;

typedef struct _led_work_mode_t
{
	char mode;
	scene_mode_t scene_mode;
	diy_mode_t diy_mode;
	ct_white_mode_t ct_white_mode;	
	color_mode_t color_mode;
}led_work_mode_t;

typedef struct _config_parameter_t
{
	u8 flash_init[4];
	u8 led_status;
	u8 status_station;
	u8 uid[8];
	led_work_mode_t led_work_mode;
	//u8 param_buf[20];
}config_parameter_t;

typedef struct _device_parameter_t
{
	u8 flash_init[4];
	u8 production[4];//production name
	u8 subproduction[4];//sub production name
	u8 production_date[8];
	u8 hw_version[4];//hradware version
}device_parameter_t;



#endif /* LIBWORKTHREAD_H_ */
