#ifndef _FLASHDATA_H
#define _FLASHDATA_H


//////////////////////////////////////////////////////////////

#define MT_ERR -1
#define MT_OK 0
#define MAX_STRING_LENGTH  24    /*  */
#define SOFTWARE_VERSION		"1.01.16"

#define SLEEPRECT_W		160
#define SLEEPRECT_H		120

#define CAMERA_NOT_INIT	100

#define	FINDER_CAMERA_INI	"finder_camera"
#define MAX_RETRY	3


typedef struct _video_mode_t
{
	char mode;//0-default,1-donggan,2-rouhe
	char part;//0-part,1-all
}video_mode_t;

typedef struct _music_mode_t
{
	char mode;//0-default,1-nengliang,2-pinpu,3-jiezou,4-gundong
	char sensitivity;
	char color[4];//RGB888

}music_mode_t;

//***********add by fn in 20180620***************
typedef struct _diy_mode_t
{
	char serial_id;//diy serial id number,set by phone and return to phone
	char diy_master_mode;
	char diy_slave_mode;
	char diy_speed;
	char diy_color[4][3];//

}diy_mode_t;

typedef struct _color_mode_t
{
	char color[4];//RGB888

}color_mode_t;

typedef struct _scene_mode_t
{
	char mode;	

}scene_mode_t;

typedef struct _led_work_mode_t
{
	char mode;
	video_mode_t video_mode;
	music_mode_t music_mode;
	color_mode_t color_mode;
	scene_mode_t scene_mode;
	diy_mode_t diy_mode;//add by fn in 20180620
}led_work_mode_t;

typedef struct _flash_write_t
{
	u8 flag;
	u16 time;
}flash_write_status;

typedef struct _time_onoff_
{
	u8 flag;
	u8 on_hour;
	u8 on_min;
	u8 off_hour;
	u8 off_min;
}time_onoff;

typedef struct _time_cur_
{
	u8 enable;
	u8 hour;
	u8 min;
	u8 sec;
}time_cur_t;

typedef struct _config_parameter_t
{
	char flash_init[4];
	char led_status;
	char led_nums;
	char led_light;
	led_work_mode_t led_work_mode;
	time_onoff	switch_time;
	char init_camera;
	char one_seg_mode;
}config_parameter_t;

typedef struct _device_parameter_t
{
	char flash_init[4];
	char production[4];
	char subproduction[4];
	char hw_version[4];
	char production_date[8];
	char led_type[4];
	char chip_number[4];
	char led_length[4];
	char chip_type[4];
}device_parameter_t;

extern config_parameter_t g_config_param;
extern device_parameter_t g_device_param;
extern time_cur_t g_time_cur;

extern u8 g_need_WriteFalsh ;



void System_Flash_Init();
void Reset_flash_config();
void Config_Write_to_Flash(void);

#endif
