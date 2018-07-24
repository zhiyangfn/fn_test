/*
 * libdiylight.c
 *
 *  Created on: 2018-6-20
 *      Author: FN
 */
#include "../../proj/tl_common.h"
#include "libdiylight.h"
#include "tl8267_uart.h"
#include "libworkthread.h"
#include "main_light.h"
#include "light.h"
static u8 reset_diy_mode=0;//apply to skip the delay firstly,and avoid to set music mode repetitively

extern config_parameter_t g_config_param;


void setDiyGradualMode();
void setDiyJumpingMode();
void setDiyClickMode();
void lightDiySegmentation(u8 *color,u8 color_segm_num);


u8 count_diy_color(u8 *color_cnt,u8 *color);

#define INTERVAL_NUM 1//apply to runing mode,must be >=1
#define BLOCK_LED_NUM 2//apply to runing mode,must be >=1

#ifdef RELEASE_MODE
	#define diyPrintf U1_printf("Diy : ");U1_printf
#else
	#define diyPrintf
#endif

#define COLOR_BYTES 3
#define LED_CHIP_MAX_NUM 60
#define COLOR_TYPE_NUM_LIMITE 4

typedef struct{
	u8 diy_mode_ID;
	u8 diy_mode;
    u8 speed;
    u8 color[5][3];
}_diy_param_;
_diy_param_ diy_param;

enum _diy_mode
{
	Diy_Gradual=0,
	Diy_Jumping=1,
	Diy_Click=2,
};

/*****************
 * count delay
 * if reach set-time return 1,else return 0
 * unit ms
**************** */
u8 libDelayms(u32 time_ms)
{
	static u32 time_task_tick=0;
	u8 ret=0;
	if(clock_time_exceed(time_task_tick, 1000*time_ms))
	{
		time_task_tick=clock_time();
		ret=1;
	}

	return ret;
}
void libDiyModeWorkThread()
{
	u8 diy_mode;
	
	diy_mode=diy_param.diy_mode;
	
	switch(diy_mode)
	{
		case Diy_Gradual:
						setDiyGradualMode();
						break;
		case Diy_Jumping:
						setDiyJumpingMode();
						break;
		case Diy_Click:
						setDiyClickMode();
						break;
		default:
						diyPrintf("master mode set false!\n");
						break;
	}
	
}
/**************************************************************
 * lightDiyGradualLed
 * set color cur_color+grad_step*step_cnt approaching to aim_color
 **************************************************************/
u8 lightDiyGradualLed(u8 *aim_color,u8 *cur_color,u8 color_num_cnt,u8 grad_step,u8 step_cnt)
{
	u8 light_set_color[COLOR_BYTES]={0,0,0};
	u8 light_aim_color[COLOR_BYTES]={0,0,0};
	u8 i;
	//u8 one_color_num,color_buf_offset=0;
	//u8 set_color_buf[LED_CHIP_MAX_NUM*COLOR_BYTES];
	u8 color_reach_cnt=0;
	//u8 led_chip_num=0;//libzled_get();

	memcpy(light_aim_color,aim_color,COLOR_BYTES*color_num_cnt);
	memcpy(light_set_color,cur_color,COLOR_BYTES*color_num_cnt);

	for(i=0;i<COLOR_BYTES;i++)
	{
		if(light_aim_color[i] > light_set_color[i])
		{
			if(light_aim_color[i] > light_set_color[i]+(step_cnt*grad_step))
				light_set_color[i] +=(step_cnt*grad_step);
			else
			{
				light_set_color[i] = light_aim_color[i];
				color_reach_cnt++;
			}
		}
		else
		{
			if(light_aim_color[i]+(step_cnt*grad_step) < light_set_color[i] &&
					light_set_color[i] > (step_cnt*grad_step))
				light_set_color[i] -=(step_cnt*grad_step);
			else
			{
				light_set_color[i] = light_aim_color[i];
				color_reach_cnt++;
			}
		}
	}
	/*
	one_color_num=led_chip_num/color_num_cnt;
	if(led_chip_num%color_num_cnt !=0)
			one_color_num++;
	for(i=0;i<color_num_cnt;i++)
	{
		for(i1=0;i1<one_color_num;i1++)
		{
			if(color_buf_offset < led_chip_num*COLOR_BYTES)
			{
				memcpy(set_color_buf+color_buf_offset,light_set_color[i],COLOR_BYTES);
			}
			else
				break;
			color_buf_offset+=COLOR_BYTES;
		}
		if(color_buf_offset >= led_chip_num*COLOR_BYTES)
			break;
	}
*/
	setLedRGBValue(light_set_color[0],light_set_color[1],light_set_color[2]);
	//libzled_send(set_color_buf,led_chip_num*COLOR_BYTES,0);
	if(color_reach_cnt >= COLOR_BYTES*color_num_cnt)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

u8 count_diy_color(u8 *color_cnt,u8 *color)
{
	u8 i;
	u8 get_color[COLOR_TYPE_NUM_LIMITE][COLOR_BYTES]={{0,0,0},{0,0,0},{0,0,0},{0,0,0}};
	memcpy(get_color,diy_param.color,COLOR_TYPE_NUM_LIMITE*COLOR_BYTES);
	for(i=0;i<COLOR_TYPE_NUM_LIMITE;i++)//count the vaild color numbers
	{
		if(get_color[i][0] == 0 && get_color[i][1] == 0 && get_color[i][2] == 0)
			break;
		else
			(*color_cnt)++;
	}
	memcpy(color,get_color,COLOR_TYPE_NUM_LIMITE*COLOR_BYTES);
	if(color_cnt == 0)
		return 0;
	else
		return 1;
}
void setDiyGradualMode()
{
	static u8 diy_gradual_step_cnt=0;
	static u8 gradual_circuit_color_cnt=0;
	u8 color_segm_num;
	u8 speed;
	u8 color[COLOR_TYPE_NUM_LIMITE][COLOR_BYTES]={{0,0,0},{0,0,0},{0,0,0},{0,0,0}};
	u8 cur_color[COLOR_BYTES]={0,0,0};
	u8 aim_color[COLOR_BYTES]={0,0,0};
	u8 color_cnt=0;
	u8 grad_step;

	speed=diy_param.speed;
	if(speed > 100)
		speed=100;
	grad_step=1;
	if(libDelayms(100-speed) ==0 && (reset_diy_mode == 0))
		return;
		
	reset_diy_mode=0;
	if(count_diy_color(&color_cnt,color) == 0)
	{
		diyPrintf("no valid color,please reset again\n");
		return;
	}

	if(color_cnt > 1)
	{
		memcpy(cur_color,color[gradual_circuit_color_cnt],COLOR_BYTES);
		if(gradual_circuit_color_cnt < color_cnt-1)
			memcpy(aim_color,color[gradual_circuit_color_cnt+1],COLOR_BYTES);
		else
			memcpy(aim_color,color[0],COLOR_BYTES);
	}
	else if(color_cnt==1)//only one color,equal to GJF_Segmentation
	{
		static u8 cur_color_status=0;
		if(diy_gradual_step_cnt == 0)
		{
			cur_color_status=(cur_color_status+1)%2;
		}
		if(cur_color_status == 0)
		{
			memcpy(aim_color,color,COLOR_BYTES);
		}
		else
		{
			memcpy(cur_color,color,COLOR_BYTES);
		}
	}
	color_segm_num=1;
	if(lightDiyGradualLed(aim_color,cur_color,color_segm_num,grad_step,diy_gradual_step_cnt) == 1)
	{
		diy_gradual_step_cnt=0;
		gradual_circuit_color_cnt++;
		if(gradual_circuit_color_cnt >= color_cnt)
			gradual_circuit_color_cnt=0;
		//if(slave_mode == GJF_Whole)
		{
			diyPrintf("setDiyGradualMode!aim_color=%d,%d,%d\n",aim_color[0],aim_color[1],aim_color[2]);
			diyPrintf("setDiyGradualMode!cur_color=%d,%d,%d\n",cur_color[0],cur_color[1],cur_color[2]);
			//diyPrintf("setDiyGradualMode!color=%d,%d,%d\n",color[0][0],color[0][1],color[0][2]);
		}
		/*else
		{
			for(i=0;i<color_cnt;i++)
			{
				diyPrintf("setDiyGradualMode!aim_color%d=%d,%d,%d\n",i+1,aim_color[i][0],aim_color[i][1],aim_color[i][2]);
				diyPrintf("setDiyGradualMode!cur_color%d=%d,%d,%d\n",i+1,cur_color[i][0],cur_color[i][1],cur_color[i][2]);
			}
		}*/
	}
	else
		diy_gradual_step_cnt++;
}
/*****************************************
*
******************************************/

void setDiyJumpingMode()
{
	static u8 color_jump_circult_cnt=0;
	u8 speed;
	u8 color[COLOR_TYPE_NUM_LIMITE][COLOR_BYTES]={{0,0,0},{0,0,0},{0,0,0},{0,0,0}};
	u8 set_color[COLOR_BYTES]={0,0,0};
	u8 color_cnt=0;

	//speed 100-0 equal to 500ms-10.5s
	//speed=g_config_param.led_work_mode.diy_mode.diy_speed;
	speed =diy_param.speed;
	if(speed >= 100)
		speed=100;
	if(libDelayms(100*(100-speed)+500) == 0 && reset_diy_mode == 0)
		return;

	reset_diy_mode=0;
	if(count_diy_color(&color_cnt,color) == 0)
	{
		diyPrintf("no valid color,please reset again\n");
		return;
	}

	//if(slave_mode == GJF_Whole)
	{
		memcpy(set_color,color+color_jump_circult_cnt,COLOR_BYTES);
		//lightDiySegmentation(set_color,1);
		setLedRGBValue(set_color[0],set_color[1],set_color[2]);
	}
	if(++color_jump_circult_cnt >= color_cnt)
		color_jump_circult_cnt=0;
}
void setDiyClickMode()
{
	static u8 color_filcker_circult_cnt=0;
	static u8 color_filcker_status_cnt=0;
	u8 speed;
	u8 color[COLOR_TYPE_NUM_LIMITE][COLOR_BYTES]={{0,0,0},{0,0,0},{0,0,0},{0,0,0}};
	u8 set_color[COLOR_BYTES]={0,0,0};
	u8 color_cnt=0;

	if(libDelayms(100) == 0)
		return;

	reset_diy_mode = 0;
	if(count_diy_color(&color_cnt,color) == 0)
	{
		diyPrintf("no valid color,please reset again\n");
		return;
	}

	//speed=g_config_param.led_work_mode.diy_mode.diy_speed;
	speed=diy_param.speed;
	if(speed >100)
		speed=100;
	if((color_filcker_status_cnt > (100-speed)/5) && reset_diy_mode == 0)//color_filcker_status_cnt reach speed,light led,else close the led
	{
		//color_filcker_status_cnt++;
		memcpy(set_color,color+color_filcker_circult_cnt,COLOR_BYTES);
		setLedRGBValue(set_color[0],set_color[1],set_color[2]);//light the led
		color_filcker_status_cnt=0;
		if(++color_filcker_circult_cnt >= color_cnt)
			color_filcker_circult_cnt=0;
		return;
	}
	else		
	{
		color_filcker_status_cnt++;
		setLedRGBValue(0,0,0);;//close the led
	}
}

void diyModeDetailPrintf()
{
	/*u8 i;
	diyPrintf("diy ID = %d\n",g_config_param.led_work_mode.diy_mode.serial_id);
	diyPrintf("diy master mode = %d\n",g_config_param.led_work_mode.diy_mode.diy_master_mode);
	diyPrintf("diy slave mode = %d\n",g_config_param.led_work_mode.diy_mode.diy_slave_mode);
	diyPrintf("diy speed = %d\n",g_config_param.led_work_mode.diy_mode.diy_speed);
	for(i=0;i<4;i++)
	{
		diyPrintf("diy color %d = %d  %d  %d",i,
		g_config_param.led_work_mode.diy_mode.diy_color[i][0],
		g_config_param.led_work_mode.diy_mode.diy_color[i][1],
		g_config_param.led_work_mode.diy_mode.diy_color[i][2]);
	}*/
}

void libDiyLightSetParams(u8 *params)
{
	u8 i;
	memcpy(&diy_param,params,19);
	diyPrintf("diy mode ID=  %d \n",diy_param.diy_mode_ID);
	diyPrintf("diy mode =  %d \n",diy_param.diy_mode);
	diyPrintf("diy mode speed =  %d \n",diy_param.speed);
	for(i=0;i<4;i++)
	{
		diyPrintf("color %d = %d  %d  %d \n",i,diy_param.color[i][0],diy_param.color[i][1],diy_param.color[i][2]);
	}
	/*typedef struct{
		u8 diy_mode;
	    u8 speed;
	    u8 color[8][3];
	    u8 color_lum;
	}_diy_param_;
	diy_param.diy_mode=;
	diy_param.speed=;*/

}

/**************************************************************
 * lightDiySegmentation
 * set some different color Segmentation
 *************************************************************/
void lightDiySegmentation(u8 *color,u8 color_segm_num)
{
	u8 light_set_color[COLOR_TYPE_NUM_LIMITE][COLOR_BYTES]={{0,0,0},{0,0,0},{0,0,0},{0,0,0}};
	u8 one_color_num,color_buf_offset=0;
	u8 set_color_buf[LED_CHIP_MAX_NUM*COLOR_BYTES];
	u8 i,i1;
	u8 led_chip_num=0;//=libzled_get();

	memcpy(light_set_color,color,COLOR_BYTES*color_segm_num);

	if(color_segm_num > COLOR_TYPE_NUM_LIMITE)
	{
		diyPrintf("too more segmentation numbers,please reset color_segm_num less than %d\n",COLOR_TYPE_NUM_LIMITE+1);
		return;
	}

	one_color_num=led_chip_num/color_segm_num;
	if(led_chip_num%color_segm_num !=0)
		one_color_num++;

	for(i=0;i<color_segm_num;i++)
	{
		for(i1=0;i1<one_color_num;i1++)
		{
			if(color_buf_offset < led_chip_num*COLOR_BYTES)
			{
				memcpy(set_color_buf+color_buf_offset,light_set_color[i],COLOR_BYTES);
			}
			else
				break;
			color_buf_offset+=COLOR_BYTES;
		}
		if(color_buf_offset >= led_chip_num*COLOR_BYTES)
			break;
	}
	//libzled_send(set_color_buf,led_chip_num*COLOR_BYTES,0);

}

