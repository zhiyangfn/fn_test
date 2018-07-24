/*
 * libflashdata.c
 *
 *  Created on: 2018-7-23
 *      Author: FN
 */

#include "tl8267_uart.h"
#include "../../vendor/common/user_config.h"
#include "../../proj/tl_common.h"
#include "libworkthread.h"
#include "light.h"


#define OWN_FLASH_DATA_START FLASH_ADR_USER_DATA

#define DEVICE_DATA_OFESET 0
#define FLASH_DEVICE_ADDR (OWN_FLASH_DATA_START+DEVICE_DATA_OFESET)

#define CONFIG_DATA_OFFSET 0x500
#define FLASH_CONFIG_ADDR (OWN_FLASH_DATA_START+CONFIG_DATA_OFFSET)

#define MESH_NET_UID_OFFSET 0x1000
#define MESH_NET_UID_ADDR (OWN_FLASH_DATA_START+MESH_NET_UID_OFFSET)


#define UPDATECHECKDATE 0x20180727

const u32 flash_config_init_check=UPDATECHECKDATE;

#ifdef RELEASE_MODE
	#define FLASH_Printf  U1_printf("flash: ");U1_printf
#else
	#define FLASH_Printf
#endif
extern config_parameter_t g_config_param;
extern device_parameter_t g_device_param;

u8 g_test_save_data2flash=0;

void initFlashConfig()
{
	memcpy(g_config_param.flash_init, &flash_config_init_check,4);

	g_config_param.led_status=0;
	g_config_param.status_station=0;
	memset(g_config_param.uid,0xff,sizeof(g_config_param.uid));
	memset(g_config_param.param_buf,0,20);

	flash_erase_sector(FLASH_CONFIG_ADDR);
	FLASH_Printf("Init_flash_config len:%d\r\n",sizeof(g_config_param));
	flash_write_page ((u32)(FLASH_CONFIG_ADDR), sizeof(g_config_param), (u8*)(&g_config_param));
	u32 *pdata2= (u32*)(FLASH_CONFIG_ADDR);
	FLASH_Printf(" Init_flash head %x\r\n",*pdata2);
	FLASH_Printf("Save config data to flash");
}
void deviceReadFromFlash()
{
	u32 *pdata1 = (u32 *)(FLASH_DEVICE_ADDR);
	FLASH_Printf("device head is %x \r\n",*pdata1);
	flash_read_page((u32)(FLASH_DEVICE_ADDR) ,sizeof(g_device_param), (u8*)(&g_device_param));
}

void configReadFromFlash()
{
	u32 *pdata1 = (u32 *)(OWN_FLASH_DATA_START+CONFIG_DATA_OFFSET);
	FLASH_Printf("config head is %x \r\n",*pdata1);
	if(UPDATECHECKDATE != *pdata1)
	{
		FLASH_Printf("config is update and init");
		initFlashConfig();
	}
	else
	{
		FLASH_Printf("config is power read\r\n");
		flash_read_page((u32)(FLASH_CONFIG_ADDR) ,sizeof(g_config_param), (u8*)(&g_config_param) );
		FLASH_Printf("status_station = %d \n",g_config_param.status_station);
	}
	/*flash_erase_sector(FLASH_CONFIG_ADDR);
	FLASH_Printf("Init_flash_config:%d\r\n",sizeof(g_config_param));
	flash_write_page ((u32)(FLASH_CONFIG_ADDR), sizeof(g_config_param), (u8*)(&g_config_param));
	u32 *pdata2= (u32*)(FLASH_CONFIG_ADDR);
	FLASH_Printf(" Init_flash head %x\r\n",*pdata2);*/
}
void time2SaveConfig()
{
	flash_erase_sector(FLASH_CONFIG_ADDR);
	flash_write_page ((u32)(FLASH_CONFIG_ADDR), sizeof(g_config_param), (u8*)(&g_config_param));
	FLASH_Printf("Save config data to flash");
}
void systemFlashInit()
{
	deviceReadFromFlash();
	configReadFromFlash();
}

void updateConfigData2Flash()
{
	flash_erase_sector(FLASH_CONFIG_ADDR);
	flash_write_page ((u32)(FLASH_CONFIG_ADDR), sizeof(g_config_param), (u8*)(&g_config_param));
}
/*******************************************************
*return 1,mean config data changes and need to save them
*return 0,mean config data not changes
********************************************************/
u8 testIfDataChange()
{
	config_parameter_t g_config_param_in_flash;
	flash_read_page((u32)(FLASH_CONFIG_ADDR) ,sizeof(g_config_param_in_flash), (u8*)(&g_config_param_in_flash) );
	if(memcmp(&g_config_param_in_flash,&g_config_param,sizeof(g_config_param_in_flash)) == 0)
	{
		FLASH_Printf("config data not changes and no need to save\r\n");
		return 0;
	}
	else
	{
		FLASH_Printf("config data has changes and save them to flash\r\n");
		return 1;
	}
}
