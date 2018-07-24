/*
 * libscenes.c
 *
 *  Created on: 2018-3-6
 *      Author: iHoment-MQ
 */

#include "../../proj/tl_common.h"
//#include "../../proj_lib/ble/blt_config.h"

#include "tl8267_uart.h"
//#include "libflashdata.h"
//#include "libledsend.h"
#include "libScenes.h"
#include "main_light.h"
#include "libworkthread.h"
#include "light.h"
//#include "tl826x_led.h"

#define MAX(a,b,c)   (a>b?(a>c?a:c):(b>c?b:c))
#define ABS(a,b)	 ((a)>(b)?(a)-(b):(b)-(a))

#define COLOR_RED		255	,0	,0
#define COLOR_ORANGE	255	,127,0
#define COLOR_YELLOW 	255	,255,0
#define COLOR_GREEN 	0	,255,0
#define COLOR_BLUE 		0	,0	,255
#define COLOR_CYAN 		0	,255,255
#define COLOR_PURPLE	128	,0	,255
#define COLOR_COOL_WHITE	255 ,255 ,255
#define COLOR_WARM_WHITE	255 ,184 ,114

#ifdef RELEASE_MODE
	#define gm_printf U1_printf("gm: ");U1_printf
#else
	#define gm_printf
#endif


static u8 gSceneRunning = 0;
static u8 gSceneMode = 0;

void led_set_change_para(u16 r,u16 g, u16 b,u16 luminance, u32 reach_time,u32 static_time);//just execute one
char led_gradual_change();

u32 execute_step_time =0;
u32 execute_hold_time;
extern u32 execute_step_time;
extern u32 execute_hold_time;
static u32 step_time,hold_time;//evercher color change time

static u16 r_cur,g_cur,b_cur;
static u16 r_abs,g_abs,b_abs;
static u16 r_aim,g_aim,b_aim;
static u16 max_step =0;

static u8 color_cnt =0;
static u8 step =1;
//static u8 rgb_scale=0;
void led_set_change_para(u16 r,u16 g, u16 b,u16 luminance, u32 reach_time,u32 static_time)//just execute one
{
//	if(reach_time<1000){
		//rgb_scale =SEND_RGB255;
		r_aim =r*(u16)luminance/0xff;
		g_aim =g*(u16)luminance/0xff;
		b_aim =b*(u16)luminance/0xff;

//	}else{
//		rgb_scale =SEND_RGB1023;
//		r_aim =16*r*(u32)luminance/1023;//4*4 =16
//		g_aim =16*g*(u32)luminance/1023;
//		b_aim =16*b*(u32)luminance/1023;
//	}

	r_abs = ABS(  r_aim,r_cur);
	g_abs = ABS(  g_aim,g_cur);
	b_abs = ABS(  b_aim,b_cur);

	max_step  = MAX(r_abs,g_abs,b_abs);

	execute_step_time = reach_time / max_step;
	step_time =execute_step_time;

	execute_hold_time =static_time;
	hold_time = static_time;

//	gm_printf("max_step %d-execute_step_time %d\r\n",max_step,execute_step_time);
}

char led_gradual_change(void)
{
	static u16 step_cnt =0;
	if(step ==1){
		if(execute_step_time <= 0) {
			execute_step_time =step_time;
			step_cnt++;

			if(r_cur < r_aim) r_cur += 1;
			else if(r_cur > r_aim) r_cur -= 1;

			if(g_cur < g_aim) g_cur += 1;
			else if(g_cur > g_aim) g_cur -= 1;

			if(b_cur < b_aim) b_cur += 1;
			else if(b_cur > b_aim) b_cur -= 1;

			if(step_cnt >= max_step)
			{
				step_cnt=0;
				step =2;
				execute_hold_time = hold_time;
				r_cur =r_aim;
				g_cur =g_aim;
				b_cur =b_aim;
				//libzled_setRGB_Color_lum(r_cur,g_cur,b_cur,255,rgb_scale);
				setLedRGBValue(r_cur,g_cur,b_cur);
				return 1;
			}
			setLedRGBValue(r_cur,g_cur,b_cur);
			//libzled_setRGB_Color_lum(r_cur,g_cur,b_cur,255,rgb_scale);
			return 0;
		}
	}
	else {

		if(hold_time == 0)  return 3;//hold forever
		//libzled_setRGB_Color_lum(r_cur,g_cur,b_cur,255,rgb_scale);
		setLedRGBValue(r_cur,g_cur,b_cur);
		if(execute_hold_time <=0 ){
			step =1;
			return 2;//one cycle finish
		}
	}

	return -1;
}



void libSceneLight_start(void)
{
	gSceneRunning = 1;
	gm_printf("libSceneLight_start \r\n");
}

void libSceneLight_stop()
{
	gSceneRunning = 0;

	gm_printf("libSceneLight_stop \r\n");
}


void libSceneLight_set(u8 SceneMode)
{
	gSceneMode = SceneMode;
	execute_hold_time =10;
	step_time =10;
	hold_time =10;
	max_step =0;
	step =1;
	color_cnt=0;
	gm_printf("libSceneLight_set :%d\r\n",gSceneMode);
}


const u16 scene_sunrise[]={//
		3,
		255 , 30 , 0,  150,  1,120, //red..
		250, 120,   2, 200,  300,180,//orange..
		180, 255, 80,  200,  300,0,//white..
};
const u16 scene_sunset[]={//
		3,
		250, 120,  0,200,  1,120,
		255,  30, 0 ,100,  300,180,
		1,  1,  1   ,255,  300,0,
};

const u16 scene_romance[]={//
		1,
		80 ,0,  255 ,150, 1500,0,//purple
};

const u16 scene_breathe[]={
		7,
		COLOR_RED	, 200,1000,500,			//red
		COLOR_ORANGE, 200,1000,500,			//orange
		COLOR_YELLOW, 200,1000,500,			//yellow
		COLOR_GREEN , 200,1000,500,			//green
		COLOR_BLUE  , 200,1000,500,			//blue
		COLOR_CYAN  , 200,1000,500,			//cyan
		COLOR_PURPLE, 200,1000,500,			//purple
};

const u16 scene_colorful[]={
		7,
		COLOR_RED	, 200,10,10,			//red
		COLOR_ORANGE, 200,10,10,			//orange
		COLOR_YELLOW, 200,10,10,			//yellow
		COLOR_GREEN , 200,10,10,			//green
		COLOR_BLUE  , 200,100,10,			//blue
		COLOR_CYAN  , 200,10,10,			//cyan
		COLOR_PURPLE, 200,10,10,			//purple
};

const u16 scene_vivid[]={
		1,
		0,  100 ,  255, 200,1500,0,		//blue
};

#if 0
const u16 scene_reading[]={
		1,
		255,203 ,141  , 40,1500,0,			//3000k
};
#else

const u16 scene_reading[]={
		1,
		255,160 ,5  , 255,1500,0,			//3000k
};
#endif
const u16 scene_easiness[]={
		1,
		0, 255, 20,  200,1500,0,			//green
};


u8 libSceneLight_work_thread(void)
{
	char step_status;
	switch(gSceneMode)
	{
		case SCENE_SUNRISE:
			step_status = led_gradual_change();
			if(step_status ==2 ){
				if(color_cnt>=scene_sunrise[0]) color_cnt=0;
				led_set_change_para(scene_sunrise[6*color_cnt+1],scene_sunrise[6*color_cnt+2],scene_sunrise[6*color_cnt+3],\
									scene_sunrise[6*color_cnt+4],scene_sunrise[6*color_cnt+5]*1000,scene_sunrise[6*color_cnt+6]*1000);
				color_cnt++;
			}
			break;

		case SCENE_SUNSET:
			step_status = led_gradual_change();
			if(step_status ==2 ){
				if(color_cnt>=scene_sunset[0]) color_cnt=0;
				led_set_change_para(scene_sunset[6*color_cnt+1],scene_sunset[6*color_cnt+2],scene_sunset[6*color_cnt+3],\
									scene_sunset[6*color_cnt+4],scene_sunset[6*color_cnt+5]*1000,scene_sunset[6*color_cnt+6]*1000);
				color_cnt++;
			}else if(step_status ==3 ){
				setDefaultWorkStatus();
				closeAllLed();
			}

			break;

		case SCENE_VIVID:
			step_status = led_gradual_change();
			if(step_status ==2 ){
				if(color_cnt>=scene_vivid[0]) color_cnt=0;
				led_set_change_para(scene_vivid[6*color_cnt+1],scene_vivid[6*color_cnt+2],scene_vivid[6*color_cnt+3],\
									scene_vivid[6*color_cnt+4],scene_vivid[6*color_cnt+5],scene_vivid[6*color_cnt+6]);
				color_cnt++;
			}
			break;

		case SCENE_READING:
			step_status = led_gradual_change();
			//setWarmWhite_color_value(255,10,RGB_SCALE_255);

			if(step_status ==2 ){
				if(color_cnt>=scene_reading[0]) color_cnt=0;
				led_set_change_para(scene_reading[6*color_cnt+1],scene_reading[6*color_cnt+2],scene_reading[6*color_cnt+3],\
									scene_reading[6*color_cnt+4],scene_reading[6*color_cnt+5],scene_reading[6*color_cnt+6]);
				color_cnt++;
			}
			break;

		case SCENE_EASINESS:
			step_status = led_gradual_change();
			if(step_status ==2 ){
				if(color_cnt>=scene_easiness[0]) color_cnt=0;
				led_set_change_para(scene_easiness[6*color_cnt+1],scene_easiness[6*color_cnt+2],scene_easiness[6*color_cnt+3],\
									scene_easiness[6*color_cnt+4],scene_easiness[6*color_cnt+5],scene_easiness[6*color_cnt+6]);
				color_cnt++;
			}
			break;


		case SCENE_ROMANCE:
			step_status = led_gradual_change();
			if(step_status ==2 ){

				if(color_cnt>=scene_romance[0]) color_cnt=0;
				led_set_change_para(scene_romance[6*color_cnt+1],scene_romance[6*color_cnt+2],scene_romance[6*color_cnt+3],\
									scene_romance[6*color_cnt+4],scene_romance[6*color_cnt+5],scene_romance[6*color_cnt+6]);
				color_cnt++;
			}
			break;

		case SCENE_BREATHE:
			step_status = led_gradual_change();
			if(step_status ==2 ){

				if(color_cnt>=scene_breathe[0]) color_cnt=0;
				led_set_change_para(scene_breathe[6*color_cnt+1],scene_breathe[6*color_cnt+2],scene_breathe[6*color_cnt+3],\
									scene_breathe[6*color_cnt+4],scene_breathe[6*color_cnt+5],scene_breathe[6*color_cnt+6]);
				color_cnt++;
			}
			break;

		case SCENE_COLORFUL:
			step_status = led_gradual_change();
			if(step_status ==2 ){

				if(color_cnt>=scene_colorful[0]) color_cnt=0;
				led_set_change_para(scene_colorful[6*color_cnt+1],scene_colorful[6*color_cnt+2],scene_colorful[6*color_cnt+3],\
									scene_colorful[6*color_cnt+4],scene_colorful[6*color_cnt+5],scene_colorful[6*color_cnt+6]);
				color_cnt++;
			}
			break;

		default: color_cnt=0;
			break;
	}
	return 1;
}







