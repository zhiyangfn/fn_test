/*
 * libworkthread.c
 *
 *  Created on: 2018-7-17
 *      Author: FN
 */


#include "libscenes.h"
#include "libdiylight.h"
#include "libworkthread.h"
#include "../../proj_lib/light_ll/light_ll.h"
#include "tl8267_uart.h"
#include "../../vendor/common/user_config.h"
#include "main_light.h"
#include "light.h"

#ifdef RELEASE_MODE
	#define BLE_Printf  U1_printf("ble: ");U1_printf
#else
	#define BLE_Printf
#endif
#define MESH_NET_UID_LEN 8
//static u8 Status_station = DEFEAULT_MODE;
u8 notify_phone_flag=0;////1 mean that system has data to notify phone
#define SOFTWARE_VERSION "V1.01.00"

config_parameter_t g_config_param;
device_parameter_t g_device_param;

u8 notify_data[10];

void replyAppSuccess(u8 cmd);
void replyAppFail(u8 cmd);


enum _product_version_{
	GET_SOFTWARE_VERSION=0,
	GET_HARDWARE_VERSION=1,
};
void masterWorkThread()
{

	u8 status_station=g_config_param.status_station;
	if(getLedStatus() == 0)
		return;
#ifdef TEST_PWM
	static u32 i;
	if(i++ > 10000)
	{
		i=0;
		//pwm_set_lum(5,100,0);
		//pwm_set_lum (2, get_pwm_cmp(125,50), 1);
		//pwm_set_lum (4, get_pwm_cmp(125,50), 0);
		//pwm_set_lum (1, get_pwm_cmp(255,100), 0);
	}
	return;
#endif
	switch(status_station)
	{
		case SCENE_MODE:
			libSceneLight_work_thread();
			break;

		case DIY_MODE:
			libDiyModeWorkThread();
			break;

		default:
			break;
	}
}
void setDefaultWorkStatus()
{
	g_config_param.status_station=DEFAULT_MODE;
}
void closeAllLed()
{
	light_adjust_RGB_hw(0,0,0,100);
	pwm_set_lum (PWMID_C1,0, 0);//¿‰π‚
	pwm_set_lum (PWMID_W,0, 0);

}
void setWorkStatus(u8 cmd,u8 *params,u8 params_len)
{
	u8 i;
	if(getLedStatus() == 0)
	{
		BLE_Printf("led is light off,please turn on led.\n");
		return;
	}
	BLE_Printf("cmd=%d\n",cmd);
	BLE_Printf("params=%d  %d  %d  %d  %d  %d  %d  %d  %d  %d\n",params[0],params[1],params[2],params[3],params[4],params[5],params[6],params[7],params[8],params[9]);
	if(cmd == LGT_CMD_SCENE_MODE)
	{
		BLE_Printf("set work status scene mode \r\n");
		g_config_param.status_station = SCENE_MODE;
		
		memcpy(g_config_param.param_buf,params,10);

		libSceneLight_set(params[0]);
		setRGBSaveValue(0,0,0);
		replyAppSuccess(cmd);
	}
	else if(cmd == LGT_CMD_DIY_MODE)
	{
		static u8 get_diy_Setting_Status=0;


		u8 serial_ID=params[0];
		
		if(serial_ID == 0x00)
		{
			get_diy_Setting_Status=1;
			memcpy(g_config_param.param_buf,params+1,9);
			//g_config_param.param_len=params_len;
			BLE_Printf("get diy mode first buffer.\n");
			replyAppSuccess(cmd);
		}
		else if(serial_ID == 0x01 && get_diy_Setting_Status == 1)
		{
			g_config_param.status_station = DIY_MODE;
			memcpy(g_config_param.param_buf+9,params+1,9);
			get_diy_Setting_Status=0;
			BLE_Printf("get diy mode second buffer and set diy mode.\n");

			for(i=0;i<10;i++)
			{
				BLE_Printf("%d = %d \n",i,g_config_param.param_buf[i]);
			}

			libDiyLightSetParams(g_config_param.param_buf);
			setRGBSaveValue(0,0,0);
			replyAppSuccess(cmd);
		}
		else
			replyAppFail(cmd);
	}
	else if(cmd == LGT_CMD_SET_GET_UID)
	{
		u8 uid_status=0;//0 indicate not set,1 mean it has been set
		u8 uid[MESH_NET_UID_LEN]={0};
		memcpy(uid,g_config_param.uid,MESH_NET_UID_LEN);
		for(i=0;i<MESH_NET_UID_LEN;i++)
		{
			if(uid[i] != 0xff)
			{
				uid_status=1;
				break;
			}
		}
		if(uid_status == 0)//set new uid and notify to phone 
		{
			memcpy(g_config_param.uid,params,MESH_NET_UID_LEN);
			memcpy(notify_data+1,g_config_param.uid,MESH_NET_UID_LEN);
			BLE_Printf("set uid.\n");
			for(i=0;i<MESH_NET_UID_LEN;i++)
			{
				BLE_Printf("%d = %d \n",i,g_config_param.uid[i]);
			}
		}
		else//get the existing uid and notify to phone 
		{
			memcpy(notify_data+1,uid,MESH_NET_UID_LEN);
			BLE_Printf("get uid.\n");
			for(i=0;i<MESH_NET_UID_LEN;i++)
			{
				BLE_Printf("uid %d = %d \n",i,uid[i]);
			}
		}
		notify_data[0]=cmd;
		notify_phone_flag=1;	
	}
	else if(cmd == LGT_CMD_GET_OWN_VERSION)
	{
		u8 version_type=params[0];
		if(version_type == GET_SOFTWARE_VERSION)
		{
			memcpy(notify_data+2,SOFTWARE_VERSION,strlen(SOFTWARE_VERSION));
			BLE_Printf("get software version = %s \n",SOFTWARE_VERSION);
		}
		else if(version_type == GET_HARDWARE_VERSION)
		{
			for(u8 i=0;i<4;i++)
			{
				if(g_device_param.hw_version[i] > 100)
				{
					g_device_param.hw_version[0]=0;
					g_device_param.hw_version[1]=1;
					g_device_param.hw_version[2]=1;
					g_device_param.hw_version[3]=0;
					break;
				}
			}
			sprintf(notify_data+2,"V%d.%d.%d",g_device_param.hw_version[2],g_device_param.hw_version[1],g_device_param.hw_version[0]);
			BLE_Printf("get hardware version = %s \n",notify_data+2);
		}
		else
		{
			BLE_Printf("get version param error\n");
			replyAppFail(cmd);
			return;
		}
		notify_data[0]=cmd;
		notify_data[1]=version_type;
		notify_phone_flag=1;
	}
	else if((cmd == LGT_CMD_SET_RGB_VALUE) && (params[0] == 5))//CT mode
	{
		u8 color_tem=params[1];
		BLE_Printf("ct mode and set ct value\n");
		g_config_param.status_station = WHITE_MODE;
		setCTValue(color_tem);
		memcpy(g_config_param.param_buf,params+1,9);
	}
	else if((cmd == LGT_CMD_LIGHT_SET) && (g_config_param.status_station == WHITE_MODE))//change lum at ct mode,and change lum
	{
		u8 color_tem=g_config_param.param_buf[0];
		setCTValue(color_tem);
	}
	else if(cmd == LGT_CMD_LIGHT_RC_SET_RGB ||
			cmd == LGT_CMD_SET_RGB_VALUE ||
			cmd == LGT_CMD_SET_SCENE)
	{
		g_config_param.status_station = DEFAULT_MODE;
		BLE_Printf("set work status default mode \r\n");
	}
	else
	{
		replyAppFail(cmd);
		BLE_Printf("cmd param error\r\n");
	}
}
void getNotifyData(u8 *get_notify_data_buf)
{
	memcpy(get_notify_data_buf,notify_data,10);
	memset(notify_data,0,10);
}
void replyAppSuccess(u8 cmd)
{
	notify_data[0]=0xAA;
	notify_data[1]=cmd;
	notify_data[2]=0x01;
	notify_phone_flag=1;
}
void replyAppFail(u8 cmd)
{
	notify_data[0]=0xAA;
	notify_data[1]=cmd;
	notify_data[2]=0x00;
	notify_phone_flag=1;
}
void modeSetInit()
{
	u8 status_station=g_config_param.status_station;
	switch(status_station)
	{
		case SCENE_MODE:
			libSceneLight_set(g_config_param.param_buf[0]);
			setLedRGBValue(0,0,0);
			break;

		case DIY_MODE:
			setLedRGBValue(0,0,0);
			break;

		case WHITE_MODE:
			setCTValue(g_config_param.param_buf[0]);
			break;


		default:
			break;
	}
}
//1=on
//0=off
void setLedStatus(u8 on_off)
{
	g_config_param.led_status=on_off;
}
u8 getLedStatus()
{
	return g_config_param.led_status;
}
