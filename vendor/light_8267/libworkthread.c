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
#include "main_light.h"

#define OWN_FLASH_DATA_START 0X79500
#define MESH_NET_UID_OFFSET 0

#define MESH_NET_UID_LEN 8
#define BLE_Printf  U1_printf("ble: ");U1_printf

static u8 Status_station = DEFEAULT_MODE;
u8 notify_phone_flag=0;////1 mean that system has data to notify phone
static u8 notify_data[10];

config_parameter_t g_config_param;


enum _product_version_{
	SOFTWARE_VERSION=0,
	HARDWARE_VERSION=1,
};
void masterWorkThread()
{
	switch(Status_station)
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

void setWorkStatus(u8 cmd,u8 *params,u8 params_len)
{
	u8 get_buf[10];
	u8 i;
	BLE_Printf("cmd=%d\n",cmd);
	BLE_Printf("params=%d  %d  %d  %d  %d  %d  %d  %d  %d  %d\n",params[0],params[1],params[2],params[3],params[4],params[5],params[6],params[7],params[8],params[9]);
	if(cmd == LGT_CMD_SCENE_MODE)
	{
		Status_station = SCENE_MODE;
		
		g_config_param.cmd=cmd;
		g_config_param.param_len=params_len;
		memcpy(g_config_param.set_param_buf,params,params_len);
		
		libSceneLight_set(params[0]);
	}
	else if(cmd == LGT_CMD_DIY_MODE)
	{
		static u8 get_diy_Setting_Status=0;
		u8 serial_ID=params[0];
		
		if(serial_ID == 0x00)
		{
			get_diy_Setting_Status=1;
			memcpy(g_config_param.set_param_buf,params+1,9);
			g_config_param.param_len=params_len;
			BLE_Printf("get diy mode first buffer.\n");
		}
		else if(serial_ID == 0x01 && get_diy_Setting_Status == 1)
		{
			Status_station = DIY_MODE;
			g_config_param.cmd=cmd;
			g_config_param.param_len+=params_len;
			memcpy(g_config_param.set_param_buf+9,params+1,9);
			get_diy_Setting_Status=0;
			BLE_Printf("get diy mode second buffer and set diy mode.\n");
			BLE_Printf("g_config_param.param_len is %d\n",g_config_param.param_len);
			BLE_Printf("g_config_param.param is %d\n",g_config_param.param_len);

			for(i=0;i<g_config_param.param_len;i++)
			{
				BLE_Printf("%d = %d \n",i,g_config_param.set_param_buf[i]);
			}

			libDiyLightSetParams(g_config_param.set_param_buf);
		}
	}
	else if(cmd == LGT_CMD_SET_GET_UID)
	{
		u8 uid_status=0;//0 indicate not set,1 mean it has been set 
		flash_read_page(OWN_FLASH_DATA_START+MESH_NET_UID_OFFSET,MESH_NET_UID_LEN, get_buf);
		for(i=0;i<MESH_NET_UID_LEN;i++)
		{
			if(get_buf[i] != 0xff)
			{
				uid_status=1;
				break;
			}
		}
		if(uid_status == 0)//set new uid and notify to phone 
		{
			flash_write_page(OWN_FLASH_DATA_START+MESH_NET_UID_OFFSET,MESH_NET_UID_LEN, params);
			memcpy(notify_data,params,10);
		}
		else//get the existing uid and notify to phone 
		{
			memcpy(notify_data,get_buf,10);
		}
		notify_phone_flag=1;	
	}
	else if(cmd == LGT_CMD_GET_OWN_VERSION)
	{
		
	}
	else if(cmd == LGT_CMD_LIGHT_ONOFF  ||
			cmd == LGT_CMD_LIGHT_SET	||
			cmd == LGT_CMD_LIGHT_RC_SET_RGB ||
			cmd == LGT_CMD_SET_RGB_VALUE ||
			cmd == LGT_CMD_SET_SCENE)
	{
		Status_station = DEFEAULT_MODE;
	}
}
void getNotifyData(u8 *get_notify_data_buf)
{
	memcpy(get_notify_data_buf,notify_data,10);
}
