#pragma once

/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
extern "C" {
#endif
#if		(__PROJECT_8261_MODULE__)
	#define CHIP_TYPE				CHIP_TYPE_8261
#elif   (__PROJECT_8267_MODULE__)
	#define CHIP_TYPE				CHIP_TYPE_8267
#elif   (__PROJECT_8269_MODULE__)
	#define CHIP_TYPE				CHIP_TYPE_8269
#elif   (__PROJECT_8266_MODULE__)
	#define CHIP_TYPE				CHIP_TYPE_8266
#endif


/////////////////// MODULE /////////////////////////////////
#define TELIK_SPP_SERVICE_ENABLE			1

#define SIG_PROC_ENABLE 					0   //To known if the Master accepted or rejected Connection_Parameters_Update or not!


//////////////// SMP SETTING  //////////////////////////////
#define SMP_DISABLE			   				1
#define SMP_JUST_WORK       				0
#define SMP_PASSKEY_ENTRY   				0
#if SMP_PASSKEY_ENTRY
#define PINCODE_RANDOM_ENABLE               0//0:default: 123456
#if PINCODE_RANDOM_ENABLE
#define PRINT_DEBUG_INFO                    1
#endif
#endif

/////////////////////////////////////////////////////////////
#define     MY_APP_ADV_CHANNEL					BLT_ENABLE_ADV_ALL
#define 	MY_DIRECT_ADV_TMIE					2000000  //us


/////////////////// led pin /////////////////////////////////
#define STATE_LED							GPIO_PE1
#define ON            						1
#define OFF           						0


////////////////////////// AUDIO CONFIG /////////////////////////////
//#define	ADPCM_PACKET_LEN				128 //±àÂëÓÃµ½µÄ»º´æ
//#define TL_MIC_ADPCM_UNIT_SIZE			248

#define	TL_MIC_32K_FIR_16K				1

#define	TL_MIC_BUFFER_SIZE				512

/////////////////// DEBUG  /////////////////////////////////
//////////////////// PRINT DEBUG INFO ///////////////////////

//826x module's pin simulate as a uart tx, Just for debugging
#define PRINT_DEBUG_INFO               		1	//open/close myprintf

#if PRINT_DEBUG_INFO
	//defination debug printf pin
	#define PRINT_BAUD_RATE             		115200 //1M baud rate,should Not bigger than 1M, when system clock is 16M.
	#define DEBUG_INFO_TX_PIN           		GPIO_PA0//G0 for 8267/8269 EVK board(C1T80A30_V1.0)
	//#define PC6_OUTPUT_ENABLE	        		1       //mini_printf function contain this
	#define PULL_WAKEUP_SRC_PC6         		PM_PIN_PULLUP_1M
#endif



///////////// avoid ADC module current leakage (when module on suspend status) //////////////////////////////
#define ADC_MODULE_CLOSED               BM_CLR(reg_adc_mod, FLD_ADC_CLK_EN)  // adc clk disable
#define ADC_MODULE_ENABLE               BM_SET(reg_adc_mod, FLD_ADC_CLK_EN) // adc clk open



/////////////////// Clock  /////////////////////////////////
#define CLOCK_SYS_TYPE  		CLOCK_TYPE_PLL	//  one of the following:  CLOCK_TYPE_PLL, CLOCK_TYPE_OSC, CLOCK_TYPE_PAD, CLOCK_TYPE_ADC
#define CLOCK_SYS_CLOCK_HZ  	32000000



//////////////////Extern Crystal Type///////////////////////
#define CRYSTAL_TYPE			XTAL_12M		//  extern 12M crystal



/////////////////// watchdog  //////////////////////////////
#define MODULE_WATCHDOG_ENABLE		1
#define WATCHDOG_INIT_TIMEOUT		500  //ms



/////////////////// set default   ////////////////

#include "../common/default_config.h"

/* Disable C linkage for C++ Compilers: */
#if defined(__cplusplus)
}
#endif

