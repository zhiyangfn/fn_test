/*
 * libdiylight.c
 *
 *  Created on: 2018-6-20
 *      Author: FN
 */
#include "../../proj/tl_common.h"
#include "libdiylight.h"
#include "tl8267_uart.h"
#include "clock_i.h"
#include "libzledsend.h"
#include "libMicLight.h"

static u8 diy_mode_switch = 0;//the switch  of diy mode
static u8 reset_diy_mode=0;//apply to skip the delay firstly,and avoid to set music mode repetitively

void setDiyGradualMode(u8 slave_mode);
void setDiyJumpingMode(u8 slave_mode);
void setDiyFilckerMode(u8 slave_mode);
void setDiyRuningMode(u8 slave_mode);
void setDiyMusicMode(u8 slave_mode);

u8 count_diy_color(u8 *color_cnt,u8 *color);

#define INTERVAL_NUM 2//apply to runing mode,must be >=1
#define BLOCK_LED_NUM 2//apply to runing mode,must be >=1

#define COLOR_BYTES 3
//#define LED_CHIP_NUM 50
#define LED_CHIP_MAX_NUM 60
#define COLOR_TYPE_NUM_LIMITE 4

enum _diy_master_mode
{
	Diy_Gradual=0,
	Diy_Jumping=1,
	Diy_Filcker=2,
	Diy_Runing=3,
	Diy_Music=4,
};

enum _Gradual_Jumping_Filcke_slave_mode
{
	GJF_Whole=0,
	GJF_Segmentation=1,
	GJF_Circultation=2,
};

enum _Runing_mode
{
	R_Whole=3,
	R_Go_In=4,
	R_Go_Out=5,
};
enum _Music_mode
{
	M_Rhythm=6,
	M_Rolling=7,
	
};

void libDiyLightStop()
{
	diy_mode_switch = 0;
}

void libDiyLightStart()
{
	diy_mode_switch = 1;
	reset_diy_mode=1;
	diyModeDetailPrintf();
	diyPrintf("get led num=== %d\n",libzled_get());
}
/*****************
 * count delay
 * if reach set-time return 1,else return 0
 * unit 1ms
**************** */
u8 libDiyModeDelay(u16 speed)
{
	static u32 diy_task_tick=0;
	static u32 diy_task_cnt;
	if(clock_time_exceed(diy_task_tick, 1000))//1ms
	{
		diy_task_tick = clock_time();
		diy_task_cnt++;
		if(diy_task_cnt >= speed)
		{
			diy_task_cnt=0;
		}
		else
			return 0;
	}
	else
	{
		return 0;
	}
	return 1;
}

void libDiyModeWorkThread()
{
	u8 master_mode,slave_mode;

	if(diy_mode_switch == 0)
		return;
		
	
	master_mode=g_config_param.led_work_mode.diy_mode.diy_master_mode;
	slave_mode=g_config_param.led_work_mode.diy_mode.diy_slave_mode;


	switch(master_mode)
	{
		case Diy_Gradual:
						setDiyGradualMode(slave_mode);
						break;
		case Diy_Jumping:
						setDiyJumpingMode(slave_mode);
						break;
		case Diy_Filcker:
						setDiyFilckerMode(slave_mode);
						break;
		case Diy_Runing:
						setDiyRuningMode(slave_mode);
						break;
		case Diy_Music:
						setDiyMusicMode(slave_mode);
						break;
		default:
						diyPrintf("master mode set false!\n");
						break;
	}
	
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
	u8 led_chip_num=libzled_get();

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
	libzled_send(set_color_buf,led_chip_num*COLOR_BYTES,0);

}
/**************************************************************
 * lightDiyGradualLed
 * set color cur_color+grad_step*step_cnt approaching to aim_color
 **************************************************************/
u8 lightDiyGradualLed(u8 *aim_color,u8 *cur_color,u8 color_num_cnt,u8 grad_step,u8 step_cnt)
{
	u8 light_set_color[COLOR_TYPE_NUM_LIMITE][COLOR_BYTES]={{0,0,0},{0,0,0},{0,0,0},{0,0,0}};
	u8 light_aim_color [COLOR_TYPE_NUM_LIMITE][COLOR_BYTES]={{0,0,0},{0,0,0},{0,0,0},{0,0,0}};
	u8 i,i1;
	u8 one_color_num,color_buf_offset=0;
	u8 set_color_buf[LED_CHIP_MAX_NUM*COLOR_BYTES];
	u8 color_reach_cnt=0;
	u8 led_chip_num=libzled_get();

	memcpy(light_aim_color,aim_color,COLOR_BYTES*color_num_cnt);
	memcpy(light_set_color,cur_color,COLOR_BYTES*color_num_cnt);

	for(i=0;i<color_num_cnt;i++)
	{
		for(i1=0;i1<COLOR_BYTES;i1++)
		{
			if(light_aim_color[i][i1] > light_set_color[i][i1])
			{
				if(light_aim_color[i][i1] > light_set_color[i][i1]+(step_cnt*grad_step))
					light_set_color[i][i1] +=(step_cnt*grad_step);
				else
				{
					light_set_color[i][i1] = light_aim_color[i][i1];
					color_reach_cnt++;
				}
			}
			else
			{
				if(light_aim_color[i][i1]+(step_cnt*grad_step) < light_set_color[i][i1] &&
						light_set_color[i][i1] > (step_cnt*grad_step))
					light_set_color[i][i1] -=(step_cnt*grad_step);
				else
				{
					light_set_color[i][i1] = light_aim_color[i][i1];
					color_reach_cnt++;
				}
			}
		}
	}
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

	libzled_send(set_color_buf,led_chip_num*COLOR_BYTES,0);
	if(color_reach_cnt >= COLOR_BYTES*color_num_cnt)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
void diyFilckerCircuitMode()
{

}
void diyJumpingSegmentationMode()
{

}
u8 count_diy_color(u8 *color_cnt,u8 *color)
{
	u8 i;
	u8 get_color[COLOR_TYPE_NUM_LIMITE][COLOR_BYTES]={{0,0,0},{0,0,0},{0,0,0},{0,0,0}};
	memcpy(get_color,g_config_param.led_work_mode.diy_mode.diy_color,COLOR_TYPE_NUM_LIMITE*COLOR_BYTES);
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
void setDiyGradualMode(u8 slave_mode)
{
	static u8 diy_gradual_step_cnt=0;
	static u8 gradual_circuit_color_cnt=0;
	u8 color_segm_num;
	u8 i;
	u8 speed;
	u8 color[COLOR_TYPE_NUM_LIMITE][COLOR_BYTES]={{0,0,0},{0,0,0},{0,0,0},{0,0,0}};
	u8 cur_color[COLOR_TYPE_NUM_LIMITE][COLOR_BYTES]={{0,0,0},{0,0,0},{0,0,0},{0,0,0}};
	u8 aim_color [COLOR_TYPE_NUM_LIMITE][COLOR_BYTES]={{0,0,0},{0,0,0},{0,0,0},{0,0,0}};;
	u8 color_cnt=0;
	u8 grad_step;

	speed=g_config_param.led_work_mode.diy_mode.diy_speed;
	if(speed > 100)
		speed=100;
	//if(speed > 90)
	//{
		//grad_step=speed-90;
	//}
	//else
	{
		grad_step=1;
		if(libDiyModeDelay((100-speed)/2) ==0 && (reset_diy_mode == 0))
			return;
	}
	reset_diy_mode=0;


	if(count_diy_color(&color_cnt,color) == 0)
	{
		diyPrintf("no valid color,please reset again\n");
		return;
	}

	if(slave_mode == GJF_Whole && color_cnt > 1)//GJF_Whole must be more than two color
	{
		memcpy(cur_color,color[gradual_circuit_color_cnt],COLOR_BYTES);
		if(gradual_circuit_color_cnt < color_cnt-1)
			memcpy(aim_color,color[gradual_circuit_color_cnt+1],COLOR_BYTES);
		else
			memcpy(aim_color,color[0],COLOR_BYTES);
		color_segm_num=1;
	}
	else if(slave_mode == GJF_Segmentation || color_cnt==1)//only one color,equal to GJF_Segmentation
	{
		static u8 cur_color_status=0;
		if(diy_gradual_step_cnt == 0)
		{
			cur_color_status=(cur_color_status+1)%2;
		}
		if(cur_color_status == 0)
		{
			memcpy(aim_color,color,color_cnt*COLOR_BYTES);
		}
		else
		{
			memcpy(cur_color,color,color_cnt*COLOR_BYTES);
		}
		color_segm_num=color_cnt;
	}
	else //if(slave_mode ==GJF_Circultation)
	{
		memcpy(cur_color+gradual_circuit_color_cnt,color,(color_cnt-gradual_circuit_color_cnt)*COLOR_BYTES);
		memcpy(cur_color,color+color_cnt-gradual_circuit_color_cnt,gradual_circuit_color_cnt*COLOR_BYTES);

		memcpy(aim_color+gradual_circuit_color_cnt+1,color,(color_cnt-gradual_circuit_color_cnt-1)*COLOR_BYTES);
		memcpy(aim_color,color+color_cnt-gradual_circuit_color_cnt-1,(gradual_circuit_color_cnt+1)*COLOR_BYTES);

		color_segm_num=color_cnt;
	}

	if(lightDiyGradualLed(aim_color,cur_color,color_segm_num,grad_step,diy_gradual_step_cnt) == 1)
	{
		diy_gradual_step_cnt=0;
		gradual_circuit_color_cnt++;
		if(gradual_circuit_color_cnt >= color_cnt)
			gradual_circuit_color_cnt=0;
		if(slave_mode == GJF_Whole)
		{
			diyPrintf("setDiyGradualMode!aim_color=%d,%d,%d\n",aim_color[0][0],aim_color[0][1],aim_color[0][2]);
			diyPrintf("setDiyGradualMode!cur_color=%d,%d,%d\n",cur_color[0][0],cur_color[0][1],cur_color[0][2]);
		}
		else
		{
			for(i=0;i<color_cnt;i++)
			{
				diyPrintf("setDiyGradualMode!aim_color%d=%d,%d,%d\n",i+1,aim_color[i][0],aim_color[i][1],aim_color[i][2]);
				diyPrintf("setDiyGradualMode!cur_color%d=%d,%d,%d\n",i+1,cur_color[i][0],cur_color[i][1],cur_color[i][2]);
			}
		}
	}
	else
		diy_gradual_step_cnt++;
}
/*****************************************
*
******************************************/

void setDiyJumpingMode(u8 slave_mode)
{
	static u8 color_jump_circult_cnt=0;
	u8 speed;
	u8 color[COLOR_TYPE_NUM_LIMITE][COLOR_BYTES]={{0},{0,0,0},{0,0,0},{0,0,0}};
	u8 set_color[COLOR_TYPE_NUM_LIMITE][COLOR_BYTES]={{0,0,0},{0,0,0},{0,0,0},{0,0,0}};
	u8 color_cnt=0;
	u8 i=0;

	//speed 100-0 equal to 500ms-10.5s
	speed=g_config_param.led_work_mode.diy_mode.diy_speed;
	if(speed >= 100)
		speed=100;
	if(libDiyModeDelay(100*(100-speed)+500) == 0 && reset_diy_mode == 0)
		return;

	reset_diy_mode=0;
	if(count_diy_color(&color_cnt,color) == 0)
	{
		diyPrintf("no valid color,please reset again\n");
		return;
	}

	if(slave_mode == GJF_Circultation)
	{
		memcpy(set_color,color+color_jump_circult_cnt,(color_cnt-color_jump_circult_cnt)*COLOR_BYTES);
		memcpy(set_color+color_cnt-color_jump_circult_cnt,color,color_jump_circult_cnt*COLOR_BYTES);
		lightDiySegmentation(set_color,color_cnt);
	}
	else if(slave_mode == GJF_Segmentation)
	{
		memcpy(set_color,color,color_cnt*COLOR_BYTES);
		lightDiySegmentation(set_color,color_cnt);
	}
	else //if(slave_mode == GJF_Whole)
	{
		memcpy(set_color,color+color_jump_circult_cnt,1*COLOR_BYTES);
		lightDiySegmentation(set_color,1);
	}
	if(++color_jump_circult_cnt >= color_cnt)
		color_jump_circult_cnt=0;
}
void setDiyFilckerMode(u8 slave_mode)
{
	static u8 color_filcker_circult_cnt=0;
	static u8 color_filcker_status_cnt=0;
	u8 speed;
	u8 color[COLOR_TYPE_NUM_LIMITE][COLOR_BYTES]={{0},{0,0,0},{0,0,0},{0,0,0}};
	u8 set_color[COLOR_TYPE_NUM_LIMITE][COLOR_BYTES]={{0,0,0},{0,0,0},{0,0,0},{0,0,0}};
	u8 color_cnt=0;

	if(libDiyModeDelay(100) == 0)
		return;

	reset_diy_mode = 0;
	if(count_diy_color(&color_cnt,color) == 0)
	{
		diyPrintf("no valid color,please reset again\n");
		return;
	}

	// filcker 10ms,close 2*10*(101-speed) ms
	speed=g_config_param.led_work_mode.diy_mode.diy_speed;
	if(speed >100)
		speed=100;
	if((color_filcker_status_cnt > (100-speed)/10) && reset_diy_mode == 0)//color_filcker_status_cnt reach speed,light led,else close the led
	{
		//color_filcker_status_cnt++;
		lightDiySegmentation(set_color,1);//close the led
		color_filcker_status_cnt=0;
		if(++color_filcker_circult_cnt >= color_cnt)
			color_filcker_circult_cnt=0;
		return;
	}
	else		
	{
		color_filcker_status_cnt++;
		//color_filcker_status_cnt=0;
	}

	diyPrintf("setDiyFilckerMode!\n");


	if(slave_mode == GJF_Circultation)
	{
		memcpy(set_color,color+color_filcker_circult_cnt,(color_cnt-color_filcker_circult_cnt)*COLOR_BYTES);
		memcpy(set_color+color_cnt-color_filcker_circult_cnt,color,color_filcker_circult_cnt*COLOR_BYTES);
		lightDiySegmentation(set_color,color_cnt);
	}
	else if(slave_mode == GJF_Segmentation)
	{
		memcpy(set_color,color,color_cnt*COLOR_BYTES);
		lightDiySegmentation(set_color,color_cnt);
	}
	else //if(slave_mode == GJF_Whole)
	{
		memcpy(set_color,color+color_filcker_circult_cnt,1*COLOR_BYTES);
		lightDiySegmentation(set_color,1);
	}


}
void setDiyRuningMode(u8 slave_mode)
{
	static u8 color_circult_cnt=0;
	u8 speed;
	u8 set_color_buf[LED_CHIP_MAX_NUM*COLOR_BYTES]={0};
	u8 color[COLOR_TYPE_NUM_LIMITE][COLOR_BYTES]={{0,0,0},{0,0,0},{0,0,0},{0,0,0}};
	u8 color_cnt=0;
	u8 i=0,i1,i2;
	int color_buf_offset;
	u8 led_chip_num=libzled_get();

	memset(color,COLOR_TYPE_NUM_LIMITE*COLOR_BYTES,0);
	
	speed=g_config_param.led_work_mode.diy_mode.diy_speed;
	if(speed >= 100)
		speed=100;
	//set_delay_time_cnt=50*(2^(101-speed));
	if(libDiyModeDelay(10*(100-speed)+50) == 0 && reset_diy_mode == 0)
		return;
	reset_diy_mode = 0;

	if(count_diy_color(&color_cnt,color) == 0)
	{
		diyPrintf("no valid color,please reset again\n");
		return;
	}
	memset(set_color_buf,led_chip_num*COLOR_BYTES,0);//clear buf
	if(slave_mode == R_Whole)
	{
		for(i=0;i<100;i++)
		{
			for(i1=0;i1<color_cnt;i1++)
			{
				for(i2=0;i2<BLOCK_LED_NUM;i2++)
				{
					color_buf_offset=(color_circult_cnt+(INTERVAL_NUM+BLOCK_LED_NUM)*i*color_cnt-(INTERVAL_NUM+BLOCK_LED_NUM)*i1-i2)*COLOR_BYTES;
					if(color_buf_offset < led_chip_num*COLOR_BYTES)
					{
						if((color_buf_offset>= 0) && (color_buf_offset < led_chip_num*COLOR_BYTES))
							memcpy(set_color_buf+color_buf_offset,color[i1],COLOR_BYTES);
					}
				}
			}
		}
	}
	else if(slave_mode == R_Go_In)
	{
		for(i=0;i<100;i++)
		{
			for(i1=0;i1<color_cnt;i1++)
			{
				for(i2=0;i2<BLOCK_LED_NUM;i2++)
				{
					color_buf_offset=(color_circult_cnt+(INTERVAL_NUM+BLOCK_LED_NUM)*i*color_cnt-(INTERVAL_NUM+BLOCK_LED_NUM)*i1-i2)*COLOR_BYTES;
					if(color_buf_offset < led_chip_num*COLOR_BYTES/2)
					{
						if((color_buf_offset) >= 0)
						{
							memcpy(set_color_buf+color_buf_offset,color[i1],COLOR_BYTES);
							memcpy(set_color_buf+(led_chip_num-1)*COLOR_BYTES-color_buf_offset,color[i1],COLOR_BYTES);
						}
					}
				}
			}
		}
	}
	else //if(slave_mode == R_Go_Out)
	{
		for(i=0;i<100;i++)
		{
			for(i1=0;i1<color_cnt;i1++)
			{
				for(i2=0;i2<BLOCK_LED_NUM;i2++)
				{
					color_buf_offset=(led_chip_num/2-color_circult_cnt-(INTERVAL_NUM+BLOCK_LED_NUM)*i*color_cnt+(INTERVAL_NUM+BLOCK_LED_NUM)*i1+i2)*COLOR_BYTES;
					if(color_buf_offset < led_chip_num*COLOR_BYTES/2)
					{
						if(color_buf_offset >= 0)
						{
							memcpy(set_color_buf+color_buf_offset,color[i1],COLOR_BYTES);
							memcpy(set_color_buf+(led_chip_num-1)*COLOR_BYTES-color_buf_offset,color[i1],COLOR_BYTES);
						}
					}
				}
			}
		}
	}

	color_circult_cnt++;
	if(color_circult_cnt >= (INTERVAL_NUM+BLOCK_LED_NUM)*color_cnt)
		color_circult_cnt=0;

	libzled_send(set_color_buf,led_chip_num*COLOR_BYTES,0);
}

void setDiyMusicMode(u8 slave_mode)
{
	static u8 set_diy_music_cnt=0;
	u8 color[COLOR_TYPE_NUM_LIMITE][COLOR_BYTES]={{0,0,0},{0,0,0},{0,0,0},{0,0,0}};
	u8 color_cnt=0;
	u8 speed;
	u8 led_chip_num=libzled_get();

	speed=g_config_param.led_work_mode.diy_mode.diy_speed;
	if(speed > 100)
		speed=100;
	if(libDiyModeDelay(50*(101-speed)) == 0 && reset_diy_mode == 0)
		return;
	count_diy_color(&color_cnt,color);
	reset_diy_mode=0;
	if(slave_mode == M_Rolling)
	{
		libMicLight_set(2,0,0,led_chip_num,color[set_diy_music_cnt]);
	}else //if(slave_mode == M_Rhythm)
	{
		libMicLight_set(1,0,0,led_chip_num,color[set_diy_music_cnt]);
	}
	libMicLight_start();
	if(++set_diy_music_cnt >= color_cnt)
		set_diy_music_cnt=0;
}


void diyModeDetailPrintf()
{
	u8 i;
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
	}
}
