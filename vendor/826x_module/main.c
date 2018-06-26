#include "../../proj/tl_common.h"
#include "../../proj/mcu/watchdog_i.h"
#include "../../vendor/common/user_config.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../../proj_lib/ble/blt_config.h"
#include "../../proj_lib/ble/ll/ll.h"

#include "tl8267_ble.h"
#include "tl8267_audio.h"
#include "tl8267_uart.h"
#include "libkeyhandle.h"
#include "tl8267_spp.h"
#include "libzledsend.h"
#include "tl8267_led.h"
#include "libflashdata.h"
#include "libzfft_int.h"
#include "libMicLight.h"
#include "libScenes.h"
#include "libbleprotocol.h"
#include "libdiylight.h"


extern my_fifo_t hci_rx_fifo;
extern void timer_irq_proc(void);

#define CompanyName 	"ihoment "

char productName[6] = "H6100";

void Board_Message_Output(void)
{
    gm_printf("\r[Company Name] :"CompanyName" %s\r\n",productName);
    gm_printf("\rV[version] :%s %s %s\r\n",VersionNumber,__DATE__,__TIME__);
}

_attribute_ram_code_ void irq_handler(void)
{
	irq_blt_sdk_handler ();
	timer_irq_proc();
	unsigned char irqS = reg_dma_rx_rdy0;
    if(irqS & FLD_DMA_UART_RX)	//rx
    {
    	reg_dma_rx_rdy0 = FLD_DMA_UART_RX;
    	u8* w = hci_rx_fifo.p + (hci_rx_fifo.wptr & (hci_rx_fifo.num-1)) * hci_rx_fifo.size;
    	if(w[0]!=0)
    	{
    		my_fifo_next(&hci_rx_fifo);
    		u8* p = hci_rx_fifo.p + (hci_rx_fifo.wptr & (hci_rx_fifo.num-1)) * hci_rx_fifo.size;
    		reg_dma0_addr = (u16)((u32)p);
    	}
    }

    if(irqS & FLD_DMA_UART_TX)	//tx
    {
    	reg_dma_rx_rdy0 = FLD_DMA_UART_TX;
    }
}

void tl8267_System_Init()
{
	blc_pm_select_internal_32k_crystal();
	cpu_wakeup_init(CRYSTAL_TYPE);
	set_tick_per_us (CLOCK_SYS_CLOCK_HZ/1000000);
	clock_init();
	gpio_init();
	rf_drv_init(CRYSTAL_TYPE);

}


system_status g_System_Status = 0x00;

//short auBuf[100];
extern u16 keys_Status;
extern u8 gGain;

int main(void) 
{
	u32 taskTick =0;
	tl8267_System_Init();
	tl8267_Uart_Init();
	System_Flash_Init();
	
	tl8267_Ble_Init();
	tl8267_Bsp_Init();		
    irq_enable();
	Board_Message_Output();
	setAMIC_gain_level(8);//这里必须跟tl8267_audio.c里的gGain相同
	while (1) 
	{
		wd_clear(); 					//clear watch dog
		tl8267_Ble_loop();
		if(g_System_Status &SET_OTA_ISWORKING)continue;
		libMicLight_work_thread();
		libSceneLight_work_thread();
		libDiyModeWorkThread();//diy mode,add by fn in 20180620
		libTime_Onoff_thread();
		libRGBW_testing();
		
		if(clock_time_exceed(taskTick, 20000)){
			taskTick = clock_time();

			//testWithPython(auBuf);
			Key_Handle();
			#if 0
			if(keys_Status!=NOTHING){
				if(keys_Status == COLOR_TAP){
					gGain++;
					if(gGain == 9)gGain = 1;
					setAMIC_gain_level(gGain);
					//U1_printf("gain=%d\r\n",gGain);
				}
				keys_Status = NOTHING;
			}
			#endif
		}
	}
	return 0;
}


