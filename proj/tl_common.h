
#pragma once


#include "common/types.h"
#include "common/bit.h"
#include "common/utility.h"
#include "common/static_assert.h"
#include "common/assert.h"
#include "mcu/analog.h"
#include "common/compatibility.h"
#include "../vendor/common/user_config.h"
#include "mcu/config.h"
#include "mcu/compiler.h"
#include "mcu/register.h"
#include "mcu/gpio.h"

#include "mcu/cpu.h"
#include "mcu/dma.h"
#include "mcu/irq_i.h"
#include "mcu/clock.h"
#include "mcu/clock_i.h"
#include "mcu/random.h"
#include "common/breakpoint.h"
#include "common/log.h"

#include "drivers/flash.h"

#include "../vendor/common/myprintf.h"


//#include "../ble/ble_globals.h"

#ifdef WIN32
#include <stdio.h>
#include <string.h>
#else
#include "common/printf.h"
#include "common/string.h"
#endif

//ATTENTION: dont change bytes number other than 7 bytes on version
#define VersionNumber 	"1.01.16"
//ATTENTION: dont change bytes number other than 7 bytes on version

#define DEBUG_STOP()	{reg_tmr_ctrl = 0; reg_gpio_pb_ie = 0xff; while(1);}	// disable watchdog;  DP/DM en
#define DEBUG_SWS_EN()	{reg_tmr_ctrl = 0; reg_gpio_pb_ie = 0xff;}	// disable watchdog;  DP/DM en

#define REG_TIME32(a)			(*(volatile u32*)(REG_BASE_ADDR + (a)))

#define _DEBUG_PRINTF_		1
#if _DEBUG_PRINTF_
#define gm_printf	U1_printf("gm:  ");U1_printf
#else
#define gm_printf	//
#endif

#define _BLE_PRINTF_		1
#if _BLE_PRINTF_
#define ble_printf	U1_printf("ble:  ");U1_printf
#else
#define ble_printf	//
#endif

#define _KEY_PRINTF_		1
#if _KEY_PRINTF_
#define key_printf	U1_printf("key:  ");U1_printf
#else
#define key_printf	//
#endif


#define _DIY_PRINTF_		1
#if _DIY_PRINTF_
#define diyPrintf	U1_printf("diy:  ");U1_printf
#else
#define diyPrintf	//
#endif


typedef enum _flash_write{WRITE,READ=!WRITE}flash_write;
enum LED_WORK_MODE
{
	DEFAULT=-1,
	LED_WORK_MODE_VIDEO=0,
	LED_WORK_MODE_MUSIC=1,
	LED_WORK_MODE_COLOR=2,
	LED_WORK_MODE_SCENE=4,
	LED_WORK_MODE_DIY=7,//add by fn in 20180620
};

typedef enum _LED_STATUS_
{
	CLOSE=0,
	OPEN
}LED_STATUS;


typedef enum _LED_TYPE_
{
	LED_TYPE_RGB=0,
	LED_TYPE_WS2811
}LED_TYPE;

typedef enum _system_status{
	NEED_WRITE_FLASH =1<<1,
	SET_HEARTBEAT =1<<2,
	SET_OTA_ISWORKING =1<<3,
}system_status;

extern system_status g_System_Status;
extern char productName[6];



