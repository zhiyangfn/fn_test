
#pragma once

#define	CHIP_PACKAGE_QFN64  1
#define	CHIP_PACKAGE_QFN48  2
#define	CHIP_PACKAGE_QFN40  3
#define	CHIP_PACKAGE_QFN32  4

#ifndef CHIP_PACKAGE
#define	CHIP_PACKAGE 		CHIP_PACKAGE_QFN64
#endif

#define	CHIP_TYPE_8266  	1
#define	CHIP_TYPE_8366  	2
#define	CHIP_TYPE_8368  	3
#define	CHIP_TYPE_8267  	4
#define	CHIP_TYPE_8263  	5
#define CHIP_TYPE_8261		6
#define CHIP_TYPE_8269		7

#ifndef CHIP_TYPE
#define	CHIP_TYPE 			1000
#endif

#ifndef CHIP_8366_A1
#define CHIP_8366_A1    1
#endif

#define	MCU_CORE_8266 		1
#define	MCU_CORE_8366 		2
#define MCU_CORE_8368		3
#define	MCU_CORE_8267 		4
#define	MCU_CORE_8263 		5
#define MCU_CORE_8261 		6
#define MCU_CORE_8269 		7

#if(CHIP_TYPE == CHIP_TYPE_8266)
	#define MCU_CORE_TYPE	MCU_CORE_8266
#elif(CHIP_TYPE == CHIP_TYPE_8267)
	#define MCU_CORE_TYPE	MCU_CORE_8267
#elif(CHIP_TYPE == CHIP_TYPE_8269)
	#define MCU_CORE_TYPE	MCU_CORE_8269
#elif(CHIP_TYPE == CHIP_TYPE_8366)
	#define MCU_CORE_TYPE	MCU_CORE_8366
#elif(CHIP_TYPE == CHIP_TYPE_8368)
	#define MCU_CORE_TYPE	MCU_CORE_8368
#elif(CHIP_TYPE == CHIP_TYPE_8263)
	#define MCU_CORE_TYPE	MCU_CORE_8263
#else
	#define MCU_CORE_TYPE	1000
#endif


