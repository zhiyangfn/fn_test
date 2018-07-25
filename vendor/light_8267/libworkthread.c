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

void replyAppSuccess();
void replyAppFail();


enum _product_version_{
	GET_HARDWARE_VERSION=0,
	GET_SOFTWARE_VERSION=1,
};
enum _get_information_flag_{
	GET_LUM=0,
	GET_MODE=1,
	GET_UID=2,
	GET_VERSION=3
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
	setLedRGBValue(0,0,0);
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
		
		g_config_param.led_work_mode.scene_mode.mode=params[0];
		libSceneLight_set(g_config_param.led_work_mode.scene_mode.mode);
		setRGBSaveValue(0,0,0);//clear rgb value system save
		replyAppSuccess(cmd);
	}
	else if(cmd == LGT_CMD_DIY_MODE)
	{
		static u8 get_diy_Setting_Status=0;
		static diy_mode_t get_diy_mode;

		u8 serial_ID=params[0];
		
		if(serial_ID == 0x00)
		{
			get_diy_Setting_Status=1;
			get_diy_mode.serial_id=params[1];
			get_diy_mode.diy_master_mode=params[2];
			get_diy_mode.diy_speed=params[3];
			memcpy(get_diy_mode.diy_color,params+4,6);//get two type RBG in front
			BLE_Printf("get diy mode first buffer.\n");
			replyAppSuccess();
		}
		else if(serial_ID == 0x01 && get_diy_Setting_Status == 1)
		{
			g_config_param.status_station = DIY_MODE;
			memcpy(&get_diy_mode.diy_color[2][0],params+1,6);//get two type RBG in back
			get_diy_Setting_Status=0;
			BLE_Printf("get diy mode second buffer and set diy mode.\n");
			memcpy(&g_config_param.led_work_mode.diy_mode,&get_diy_mode,sizeof(diy_mode_t));
			BLE_Printf("diy mode serial id is %d.\n",g_config_param.led_work_mode.diy_mode.serial_id);
			BLE_Printf("diy mode master mode is %d\n",g_config_param.led_work_mode.diy_mode.diy_master_mode);
			BLE_Printf("diy mode speed is %d\n",g_config_param.led_work_mode.diy_mode.diy_speed);
			for(i=0;i<4;i++)
			{
				BLE_Printf("diy mode color %d = %d,%d,%d \n",i,g_config_param.led_work_mode.diy_mode.diy_color[i][0]
				                                              ,g_config_param.led_work_mode.diy_mode.diy_color[i][1]
				                                              ,g_config_param.led_work_mode.diy_mode.diy_color[i][2]
				                                                                                                                                                                                    				                                                                                                                 );
			}

			//libDiyLightSetParams(&g_config_param.led_work_mode.diy_mode);
			setRGBSaveValue(0,0,0);//clear rgb value system save
			replyAppSuccess();
		}
		else
			replyAppFail();
	}
	else if(cmd == LGT_CMT_SET_UID)
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
			BLE_Printf("set uid success.\n");
			for(i=0;i<MESH_NET_UID_LEN;i++)
			{
				BLE_Printf("%d = %d \n",i,g_config_param.uid[i]);
			}
			replyAppSuccess();
		}
		else//get the existing uid and notify to phone
		{
			memcpy(notify_data+1,uid,MESH_NET_UID_LEN);
			BLE_Printf("set uid fail.\n");
			for(i=0;i<MESH_NET_UID_LEN;i++)
			{
				BLE_Printf("uid %d = %d \n",i,uid[i]);
			}
			replyAppFail();
		}
	}
	else if(cmd == LGT_CMD_GET_INFORMATION)
	{
		u8 get_type=params[0];

		notify_data[0]=get_type;
		BLE_Printf("get information.type = %d \n",get_type);
		if(get_type == GET_LUM)
		{
			notify_data[1]=getLumValue();
		}
		else if(get_type == GET_MODE)
		{
			u8 mode=g_config_param.status_station;
			if(mode == SCENE_MODE)
			{
				notify_data[1]=0x00;//scene mode
				notify_data[2]=g_config_param.led_work_mode.scene_mode.mode;
				BLE_Printf("work in scene mode and sub mode = %d \n",g_config_param.led_work_mode.scene_mode.mode);
			}
			else if(mode ==DIY_MODE)
			{
				u8 diy_serial_num=params[1];
				notify_data[1]=diy_serial_num+1;//diy serial frame
				if(diy_serial_num == 0)//first frame
				{
					BLE_Printf("work in diy mode first frame  \n");
					notify_data[2]=g_config_param.led_work_mode.diy_mode.serial_id;
					notify_data[3]=g_config_param.led_work_mode.diy_mode.diy_master_mode;
					memcpy(notify_data+4,g_config_param.led_work_mode.diy_mode.diy_color,6);
				}
				else //if(diy_serial_num == 1)//second frame
				{
					BLE_Printf("work in diy mode second frame  \n");
					notify_data[2]=g_config_param.led_work_mode.diy_mode.diy_speed;
					memcpy(notify_data+3,&g_config_param.led_work_mode.diy_mode.diy_color[2][0],6);
				}
			}
			else if(mode == WHITE_MODE)
			{
				BLE_Printf("work in ct white mode ,cool_white_lum=%d ,warm_white_lum=%d\n",g_config_param.led_work_mode.ct_white_mode.cool_white_lum,
																							g_config_param.led_work_mode.ct_white_mode.warm_white_lum);
				notify_data[1]=4;//CT white mode
				notify_data[2]=g_config_param.led_work_mode.ct_white_mode.cool_white_lum;
				notify_data[3]=g_config_param.led_work_mode.ct_white_mode.warm_white_lum;
			}
			else//default color mode ,send rgb value
			{
				BLE_Printf("work in default mode ,get RGB Value  \n");
				notify_data[1]=3;//default mode ,color mode ,send RGB value
				getRGBValue(notify_data+2);
			}
		}
		else if(get_type == GET_UID)
		{
			memcpy(notify_data+1,g_config_param.uid,MESH_NET_UID_LEN);
		}
		else if(get_type ==GET_VERSION)
		{
			u8 version_type=params[1];
			notify_data[1]=version_type;
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
				replyAppFail();
				return;
			}

			notify_phone_flag=1;
		}
		else
		{
			BLE_Printf("get information param error\n");
			replyAppFail();
			return;
		}
		notify_phone_flag=1;
	}
	/*else if(cmd == LGT_CMD_GET_OWN_VERSION)
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
			replyAppFail();
			return;
		}
		notify_data[0]=cmd;
		notify_data[1]=version_type;
		notify_phone_flag=1;
	}*/
	else if((cmd == LGT_CMD_SET_RGB_VALUE) && (params[0] == 5))//CT mode
	{
		u8 color_tem=params[1];
		BLE_Printf("ct mode and set ct value\n");
		g_config_param.status_station = WHITE_MODE;
		setCTValue(color_tem);
		if(color_tem <=100)
		{
			g_config_param.led_work_mode.ct_white_mode.cool_white_lum=color_tem;
			g_config_param.led_work_mode.ct_white_mode.warm_white_lum=100-color_tem;
		}
		else
		{
			g_config_param.led_work_mode.ct_white_mode.cool_white_lum=100;
			g_config_param.led_work_mode.ct_white_mode.warm_white_lum=100;
		}
		//memcpy(g_config_param.param_buf,params+1,9);
	}
	else if((cmd == LGT_CMD_LIGHT_SET) && (g_config_param.status_station == WHITE_MODE))//change lum at ct mode,and change lum
	{
		u8 color_tem=g_config_param.led_work_mode.ct_white_mode.cool_white_lum;
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
void replyAppSuccess()
{
	notify_data[0]=0xAA;
	notify_data[1]=0x01;
	notify_phone_flag=1;
}
void replyAppFail()
{
	notify_data[0]=0xAA;
	notify_data[1]=0x00;
	notify_phone_flag=1;
}
void modeSetInit()
{
	u8 status_station=g_config_param.status_station;
	switch(status_station)
	{
		case SCENE_MODE:
			libSceneLight_set(g_config_param.led_work_mode.scene_mode.mode);
			setLedRGBValue(0,0,0);
			break;

		case DIY_MODE:
			setLedRGBValue(0,0,0);
			break;

		case WHITE_MODE:
			setCTValue(g_config_param.led_work_mode.ct_white_mode.cool_white_lum);
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
