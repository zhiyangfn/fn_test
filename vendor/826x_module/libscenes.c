#include "../../proj/tl_common.h"
#include "../../proj_lib/ble/blt_config.h"

#include "tl8267_uart.h"
#include "libflashdata.h"
#include "libzledsend.h"
#include "libScenes.h"

#define MAX(a,b,c)   (a>b?(a>c?a:c):(b>c?b:c))
#define ABS(a,b)	 ((a)>(b)?(a)-(b):(b)-(a))

static u8 gSceneRunning = 0;
static u8 gSceneMode = 0;

void led_set_change_para(u8 r,u8 g, u8 b,u8 luminance, u32 reach_time,u32 static_time);//just execute one;
char led_gradual_change();


u32 execute_step_time =0;
u32 execute_hold_time;
extern u32 execute_step_time;
extern u32 execute_hold_time;
static u32 step_time,hold_time;//evercher color change time

static u8 r_cur,g_cur,b_cur;
static u8 r_abs,g_abs,b_abs;
static u8 r_aim,g_aim,b_aim;
static u8 max_step =0;
static u8 color_cnt =0;
static u8 step =1;

void led_set_change_para(u8 r,u8 g, u8 b,u8 luminance, u32 reach_time,u32 static_time)//just execute one
{
	r_aim =r*(u16)luminance/255;
	g_aim =g*(u16)luminance/255;;
	b_aim =b*(u16)luminance/255;;
	r_abs = ABS(  r_aim,r_cur);
	g_abs = ABS(  g_aim,g_cur);
	b_abs = ABS(  b_aim,b_cur);

	max_step  = MAX(r_abs,g_abs,b_abs);

	execute_step_time = reach_time / max_step;
	if(execute_step_time < 1)
		execute_step_time =1;
	step_time =execute_step_time;

	execute_hold_time =static_time;
	hold_time = static_time;

//	gm_printf("max_step %d-execute_step_time %d\r\n",max_step,execute_step_time);
}

char led_gradual_change()
{
	static u8 step_cnt =0;
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
				libzled_setColor(r_cur,g_cur,b_cur,255,1);
				return 1;
			}
			libzled_setColor(r_cur,g_cur,b_cur,255,1);
			return 0;
		}
	}
	else {
		libzled_setColor(r_aim,g_aim,b_aim,255,1);

		if(hold_time == 0)  return 0;//hold forever

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
	execute_hold_time =10;
	step_time =10;
	hold_time =10;
	step =1;
	max_step =0;
	color_cnt=0;
	gm_printf("libSceneLight_set :%d\r\n",SceneMode);
}


const u16 scene_morning[]={//??3?
		3,
		255,  0,   0, 200,  1,240,
		250, 80,   2, 200,  300,240,
		240, 100, 10,255,  120,0,
};
const u16 scene_dusk[]={//???ии
		3,
		250, 80,  2,200,  1,180,
		255,   0,  0,100,  180,120,
		0,  0,  0  ,255,   60,0,
};
const u16 scene_night_lamp[]={//и░1ж╠?
		1,
		252,50,0,50,500,0
};
const u16 scene_party[]={//???ив --D??б┴?беD?
		4,
		252, 50,  0,  200,500,6000,
		240, 100, 0,  200,500,200,
		252, 50,  0,  200,500,8000,
		240, 100, 0,  200,500,400
};
const u16 scene_movie[]={//ж╠?иобу
		1,
		200,200,200,150,500,0
};

const u16 scene_dating[]={//???ив
		1,
		252,50,0,200,500,0
};

const u16 scene_confort[]={//??и╣бу	?бебуб┴
		1,
		240,100,10,150,500,0
};
const u16 scene_romance[]={//ид??t
		4,
		252,  0,  0,150, 1500,5000,
		200,  0,252,150, 2000,100,
		252,  0,  0,150, 2000,7000,
		200,  0,252,150, 1500,200
};
const u16 scene_blink[]={//ижив??
		6,
		240,  100, 10, 40, 1000, 300,
		252,  160, 50, 200, 200, 200,
		240,  100, 10, 40, 500, 300,
		252,  160, 50, 200, 200, 200,
		240,  100, 10, 40, 500, 300,
		255,  255, 255, 200, 800, 0,
};
const u16 scene_candle[]={//?и░1a
		12,
		252, 80,  5, 200, 1000,200,
		252, 80,  5, 120, 500,500,
		252, 80,  5, 180, 1500,300,
		252, 80,  5, 100, 800,400,
		252, 80,  5, 150, 1200,200,
		252, 80,  5,  90, 600,1500,
		252, 80,  5, 150, 100,200,
		252, 80,  5, 180, 500,500,
		252, 80,  5, 120, 1000,400,
		252, 80,  5, 100, 600,900,
		252, 80,  5, 180, 400,700,
		252, 80,  5, 150, 600,1500,
};

u8 libSceneLight_work_thread(void)
{
	char step_status;
	if(0==gSceneRunning)		//
	{
		return 0;
	}
	
	switch(gSceneMode)
	{
		case SCENE_MORNING:
			step_status = led_gradual_change();
			if(step_status ==2 ){
				if(color_cnt>=scene_morning[0]) color_cnt=0;
				led_set_change_para(scene_morning[6*color_cnt+1],scene_morning[6*color_cnt+2],scene_morning[6*color_cnt+3],\
									scene_morning[6*color_cnt+4],scene_morning[6*color_cnt+5]*1000,scene_morning[6*color_cnt+6]*1000);
				color_cnt++;
			}
			break;

		case SCENE_DUSK:
			step_status = led_gradual_change();
			if(step_status ==2 ){
				if(color_cnt>=scene_dusk[0]) color_cnt=0;
				led_set_change_para(scene_dusk[6*color_cnt+1],scene_dusk[6*color_cnt+2],scene_dusk[6*color_cnt+3],\
									scene_dusk[6*color_cnt+4],scene_dusk[6*color_cnt+5]*1000,scene_dusk[6*color_cnt+6]*1000);
				color_cnt++;
			}
			break;

		case SCENE_NIGHT_LAMP:
			step_status = led_gradual_change();
			if(step_status ==2 ){
				if(color_cnt>=scene_night_lamp[0]) color_cnt=0;
				led_set_change_para(scene_night_lamp[6*color_cnt+1],scene_night_lamp[6*color_cnt+2],scene_night_lamp[6*color_cnt+3],\
									scene_night_lamp[6*color_cnt+4],scene_night_lamp[6*color_cnt+5],scene_night_lamp[6*color_cnt+6]);
				color_cnt++;
			}
			break;

		case SCENE_PARTY:
			step_status = led_gradual_change();
			if(step_status ==2 ){
				if(color_cnt>=scene_party[0]) color_cnt=0;
				led_set_change_para(scene_party[6*color_cnt+1],scene_party[6*color_cnt+2],scene_party[6*color_cnt+3],\
									scene_party[6*color_cnt+4],scene_party[6*color_cnt+5],scene_party[6*color_cnt+6]);
				color_cnt++;
			}
			break;

		case SCENE_MOVIE:
			step_status = led_gradual_change();
			if(step_status ==2 ){
				if(color_cnt>=scene_movie[0]) color_cnt=0;
				led_set_change_para(scene_movie[6*color_cnt+1],scene_movie[6*color_cnt+2],scene_movie[6*color_cnt+3],\
									scene_movie[6*color_cnt+4],scene_movie[6*color_cnt+5],scene_movie[6*color_cnt+6]);
				color_cnt++;
			}
			break;

		case SCENE_DATING:
			step_status = led_gradual_change();
			if(step_status ==2 ){
				if(color_cnt>=scene_dating[0]) color_cnt=0;
				led_set_change_para(scene_dating[6*color_cnt+1],scene_dating[6*color_cnt+2],scene_dating[6*color_cnt+3],\
									scene_dating[6*color_cnt+4],scene_dating[6*color_cnt+5],scene_dating[6*color_cnt+6]);
				color_cnt++;
			}
			break;

		case SCENE_COMFORT:
			step_status = led_gradual_change();
			if(step_status ==2 ){

				if(color_cnt>=scene_confort[0]) color_cnt=0;
				led_set_change_para(scene_confort[6*color_cnt+1],scene_confort[6*color_cnt+2],scene_confort[6*color_cnt+3],\
									scene_confort[6*color_cnt+4],scene_confort[6*color_cnt+5],scene_confort[6*color_cnt+6]);
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

		case SCENE_BLINK:
			step_status = led_gradual_change();
			if(step_status ==2 ){

				if(color_cnt>=scene_blink[0]) color_cnt=0;
				led_set_change_para(scene_blink[6*color_cnt+1],scene_blink[6*color_cnt+2],scene_blink[6*color_cnt+3],\
									scene_blink[6*color_cnt+4],scene_blink[6*color_cnt+5],scene_blink[6*color_cnt+6]);
				color_cnt++;
			}
			break;

		case SCENE_CANDLE:
			step_status = led_gradual_change();
			if(step_status ==2 ){

				if(color_cnt>=scene_candle[0]) color_cnt=0;
				led_set_change_para(scene_candle[6*color_cnt+1],scene_candle[6*color_cnt+2],scene_candle[6*color_cnt+3],\
									scene_candle[6*color_cnt+4],scene_candle[6*color_cnt+5],scene_candle[6*color_cnt+6]);
				color_cnt++;
			}
			break;

		default: color_cnt=0;
			break;
	}
	return 1;
}





