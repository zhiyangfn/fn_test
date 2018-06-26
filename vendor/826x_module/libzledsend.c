#include "../../proj/tl_common.h"
#include "../../proj_lib/ble/blt_config.h"
#include "../../proj/drivers/i2c.h"
#include "../../proj/mcu/watchdog_i.h"

#include "tl8267_uart.h"
#include "libzledsend.h"
#include "tl8267_led.h"
#include "libflashdata.h"
#include "libScenes.h"
#include "libMicLight.h"
#include "libdiylight.h"


static u8 gAllGroup = 0;
static u8 gMaxLuminance = 255;
static u8 gCurLimit = 74;
static u8 gLedType = 0;
u8 g_RGBW_test_enable =0;

void stop_all_led_service(void);//add by fn

void libzled_set(u8 allGroup,u8 type)
{
	#if 0
	gAllGroup = 60;// allGroup;
	gLedType  = 0;//type;
	#endif
	
	gAllGroup = allGroup;// allGroup;
	if(type==LED_TYPE_WS2811){
		gLedType  = LED_TYPE_WS2811;//type;
	}
	else{
		gLedType  = LED_TYPE_RGB;//type;
	}
	gm_printf("aAllGroup=%d ,gLedType=%d\r\n",gAllGroup,gLedType);
}

u8 libzled_get(void)
{
	return gAllGroup;
}
void libzled_setMaxLuminance(u8 Luminance)
{
	g_config_param.led_light = Luminance;
	gMaxLuminance = Luminance;
}

u8 libzled_getMaxLuminance()
{
	return gMaxLuminance;
}
const product_led ihoment_ble_product[4] =
{
	//H6102		IC
	{0x06,0x01,0x00,0x02,0x00,0x1e,0x02},
	//H6127		IC
	{0x06,0x01,0x02,0x07,0x01,0x32,0x05},
	//H6113		RGB
	{0x06,0x01,0x01,0x03,0x00,0x04,0x02},
	//H6129		RGB
	{0x06,0x01,0x02,0x09,0x00,0x02,0x01},
};


u8 libzled_init()
{
	u8 i ,j;
	u8 flag = 0;
	u8 product_id = 0xff;
	for(i=0;i<4;i++){
		flag = 0;
		for(j=0;j<4;j++){	
			if(g_device_param.production[j] == ihoment_ble_product[i].product[3-j]){
				flag ++;
				if(flag==4){
					product_id = i;
				}
			}
		}
	}
	gm_printf("Flash Production H-%d-%d-%d-%d , id=%d\r\n",g_device_param.production[3],g_device_param.production[2],\
											g_device_param.production[1],g_device_param.production[0],product_id);
	gm_printf("Flash subproduction %d-%d-%d-%d\r\n",g_device_param.subproduction[3],g_device_param.subproduction[2],\
											g_device_param.subproduction[1],g_device_param.subproduction[0]);
	gm_printf("Flash hw_version %d-%d-%d-%d\r\n",g_device_param.hw_version[3],g_device_param.hw_version[2],\
											g_device_param.hw_version[1],g_device_param.hw_version[0]);
	gm_printf("Flash YEAR %d-%d-%d-%d\r\n",g_device_param.production_date[3],g_device_param.production_date[2],\
											g_device_param.production_date[1],g_device_param.production_date[0]);
	gm_printf("Flash DATE %d-%d-%d-%d\r\n",g_device_param.production_date[7],g_device_param.production_date[6],\
											g_device_param.production_date[5],g_device_param.production_date[4]);
	gm_printf("Flash CHIP %d-%d-%d-%d\r\n",g_device_param.led_type[0],g_device_param.chip_number[0],\
											g_device_param.led_length[0],g_device_param.chip_type[0]);
#if 1
	if(LED_TYPE_RGB == g_device_param.led_type[0]){		// init pwm
		gm_printf("Init_PWM_RGB\r\n");
		Init_PWM_RGB(10);//khz	
	}
	else if(LED_TYPE_WS2811== g_device_param.led_type[0]){
		gm_printf("Init_IIC_RGB\r\n");
		// init led
		Init_IIC_RGB();								//IIC to MCU
		I2c_set_color_rank(LED_COLOR_GRB);
	}
	else{
		gm_printf("Init_PWM_ERR\r\n");
		Init_PWM_RGB(10);//khz		
	}
#endif

#if 0
	Init_PWM_RGB(10);
	Init_IIC_RGB();
#endif
	I2c_set_color_rank(LED_COLOR_GRB);

	libzled_set(g_device_param.chip_number[0],g_device_param.led_type[0]);

	
	wd_clear();
	WaitMs(300);
	gm_printf("Init red test \r\n");
	wd_clear();
	libzled_setConstColor(0xFF,0x00,0x00,0);
	WaitMs(300);
	
	gm_printf("Init green test \r\n");
	wd_clear();
	libzled_setConstColor(0x00,0xFF,0x00,0);
	WaitMs(300);
	
	gm_printf("Init blue test \r\n");
	wd_clear();
	libzled_setConstColor(0x00,0x00,0xFF,0);
	WaitMs(300);
	libzled_setConstColor(0x00,0x00,0x00,0);
	wd_clear();

	libzled_setMaxLuminance(g_config_param.led_light);
	g_config_param.led_status = 1;
	start_led_service();
	
	return 0;
}

static void luminance_proc(u8* data,u8 bytes)
{
	u8 i;
	u16 tmp;
	if(gMaxLuminance >255){
		return;
	}
	for(i=0;i<bytes;i++){
		tmp = (u16)data[i];
		tmp*=gMaxLuminance;
		tmp/=255;
		data[i] = (u8)tmp;
	}
}

void luminance_limit(u8* data,u8 bytes)	//only for h6102
{
	u8 i;
	u16 tmp;
	if(g_device_param.production[3]!=6||g_device_param.production[2]!=1||\
		g_device_param.production[1]!=0||g_device_param.production[0]!=2){
		return ;
	}
	if(gCurLimit >100){
		return;
	}
	for(i=0;i<bytes;i++){
		tmp = (u16)data[i];
		tmp*=gCurLimit;
		tmp/=100;
		data[i] = (u8)tmp;
	}
}

u8 libzled_send(u8 *data,u8 bytes,u8 softFlag)
{
	
	if(LED_TYPE_RGB==gLedType){
		RGB_set_color_value(data[0],data[1],data[3],gMaxLuminance);
	}
	else{
		luminance_proc(data,bytes);
		luminance_limit(data,bytes);
		I2c_set_color_buf(data,bytes,softFlag);		//
	}
	return 0;
}


void libzled_setConstColor(u8 r,u8 g,u8 b,u8 mode)
{	
	if(LED_TYPE_RGB==gLedType){
		RGB_set_color_value(r,g,b,gMaxLuminance);
	}
	else{
		I2c_set_color_value(r,g,b,gMaxLuminance,mode);
	}
}

void libzled_setColor(u8 r,u8 g,u8 b,u8 luminance ,u8 mode)
{
	if(LED_TYPE_RGB==gLedType){
		RGB_set_color_value(r,g,b,luminance);
	}
	else{
		I2c_set_color_value(r,g,b,luminance,mode);
	}
}

u8 start_led_service()
{
	u8 cur_mode=0;
	cur_mode=g_config_param.led_work_mode.mode;
	g_RGBW_test_enable =0;
	ble_printf("ble:start_led_service:%d\r\n",cur_mode);
	if(cur_mode==LED_WORK_MODE_SCENE){
		libSceneLight_set(g_config_param.led_work_mode.scene_mode.mode);
		libSceneLight_start();
	}
	if(cur_mode==LED_WORK_MODE_MUSIC){
		libMicLight_set(g_config_param.led_work_mode.music_mode.mode, g_config_param.led_work_mode.music_mode.sensitivity,7,gAllGroup,g_config_param.led_work_mode.music_mode.color);
		libMicLight_start();
	}
	if(cur_mode==LED_WORK_MODE_COLOR){
		libzled_setConstColor(g_config_param.led_work_mode.color_mode.color[0],g_config_param.led_work_mode.color_mode.color[1],g_config_param.led_work_mode.color_mode.color[2],SEND_SOFT1_MODE);
	}
	if(cur_mode == LED_WORK_MODE_DIY)//add by fn in 20180620
	{
		stop_all_led_service();
		libDiyLightStart();
	}
	return 0;
	
}

void stop_all_led_service()
{
	libSceneLight_stop();
	libMicLight_stop();
	libDiyLightStop();
	libzled_setConstColor(0,0,0,SEND_SOFT1_MODE);
	libzled_stop();
}
u8 stop_led_service()
{
	u8 cur_mode=0;
	cur_mode=g_config_param.led_work_mode.mode;
	//stop
	ble_printf("ble:stop_led_service:%d\r\n",cur_mode);
	if(cur_mode==LED_WORK_MODE_SCENE){
		libSceneLight_stop();
	}
	if(cur_mode==LED_WORK_MODE_MUSIC){
		libMicLight_stop();
	}
	if(cur_mode == LED_WORK_MODE_DIY)//add by fn in 20180620
	{
		libDiyLightStop();
	}
	if(cur_mode==LED_WORK_MODE_COLOR){
		libzled_setConstColor(0,0,0,SEND_SOFT1_MODE);
	}
	libzled_stop();
	return 0;
}


void libzled_stop()
{
	libzled_setConstColor(0,0,0,SEND_SOFT1_MODE);
}

void libzled_start()
{
	libzled_setConstColor(g_config_param.led_work_mode.color_mode.color[0],\
						  g_config_param.led_work_mode.color_mode.color[1],\
						  g_config_param.led_work_mode.color_mode.color[2],
						  SEND_SOFT1_MODE);
}

void led_WorkMode_Init()
{
	gMaxLuminance = g_config_param.led_light;
	if(g_config_param.led_status == 1){//open the led strip
		start_led_service();
	}else{		//close the led stri
		stop_led_service(g_config_param.led_work_mode.mode);
	}
}

void libTime_Onoff_thread()
{
	if(g_config_param.switch_time.flag ==1&&g_time_cur.enable==1)
	{
		//on 
		if(g_time_cur.hour==g_config_param.switch_time.on_hour&&\
		  g_time_cur.min  ==g_config_param.switch_time.on_min&&\
		  g_time_cur.sec ==0){
			  g_time_cur.sec = 1;
			  if(g_config_param.led_status == 0){//open the led strip
					start_led_service();
			  }
			  g_config_param.led_status = 1;
		}
		//close
		if(g_time_cur.hour==g_config_param.switch_time.off_hour&&\
		  g_time_cur.min  ==g_config_param.switch_time.off_min&&\
		  g_time_cur.sec ==0){
		  g_time_cur.sec = 1;
		  if(g_config_param.led_status == 1){
					stop_led_service();
					WaitMs(5);
					stop_led_service();
		  }
		  g_config_param.led_status = 0;

		}
	}
}


const  static u8 rgbw_color[][4]=
{
	{0xff,0,0,0},\
	{0,0xff,0,0},\
	{0,0,0xff,0},\
	{0xff,0xff,0xff,0},\
	{0,0,0,0}
};

void libRGBW_testing(void)
{
	static u8 s_rgbw_cnt=0;
	static u32 taskTick=0;
	if(g_RGBW_test_enable == 1){

		if(clock_time_exceed(taskTick, 500000)){
			taskTick = clock_time();

			s_rgbw_cnt+=1;
			if(s_rgbw_cnt>4)s_rgbw_cnt=0;
			gm_printf("rgbw testing %d\r\n",s_rgbw_cnt);

			libzled_setColor(rgbw_color[s_rgbw_cnt][0],\
							 rgbw_color[s_rgbw_cnt][1],\
							 rgbw_color[s_rgbw_cnt][2],\
							0xff,1);

		}
	}

}




