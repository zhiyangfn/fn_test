#include "../../proj/tl_common.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../../proj_lib/ble/ll/ll.h"
#include "../../proj_lib/ble/hci/hci.h"
#include "../../proj_lib/ble/blt_config.h"
#include "../../proj_lib/ble/trace.h"
#include "../../proj/mcu/pwm.h"
#include "../../proj_lib/ble/service/ble_ll_ota.h"
#include "../../proj/drivers/adc.h"
#include "../../proj/drivers/battery.h"
#include "../../proj_lib/ble/blt_config.h"
#include "../../proj_lib/ble/ble_smp.h"
#include "../common/tl_audio.h"
#include "../../proj/drivers/audio.h"
#include "../../proj/drivers/uart.h"
#include "../../proj/drivers/i2c.h"
#include "../common/blt_soft_timer.h"


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
#include "libflashdata.h"

extern void main_loop ();
static void Init_led(void);
extern int event_handler(u32 h, u8 *para, int n);
extern void my_att_init ();
extern int rx_from_uart_cb (void);
extern int tx_to_uart_cb (void);


MYFIFO_INIT(blt_rxfifo, 64, 8);
MYFIFO_INIT(blt_txfifo, 40, 8);
//////////////////////////////////////////////////////////////////////////////
//	Adv Packet, Response Packet
//////////////////////////////////////////////////////////////////////////////
 u8	tbl_advData[] = {
	 19, 0x09, 'i', 'h', 'o', 'm','e', 'n', 't', '_', 'H','6', '0', '0', '0', '_','a','b','c','d',//缂佹鍏涚粩瀛樼▔椤忓嫮鎽熼柤鍝勫�瑰Σ鎼佸触瀹ュ懐鎽熼柨娑樼墦閺嗚鲸鎯旈敓锟�-1闁挎冻鎷�
	 0x02, 0x01, 0x05, 							// BLE limited discoverable mode and BR/EDR not supported
};

 u8	tbl_scanRsp [] = {
	 19, 0x09, 'i', 'h', 'o', 'm','e', 'n', 't', '_', 'H','6', '0', '0', '0', '_','a','b','c','d',	//scan name " tmodul"//缂佹鍏涚粩瀛樼▔椤忓嫮鎽熼柤鍝勫�瑰Σ鎼佸触瀹ュ懐鎽熼柨娑樼墦閺嗚鲸鎯旈敓锟�-1闁挎冻鎷�
};
static u8 device_name[]={'i', 'h', 'o', 'm','e', 'n', 't', '_', 'H','6', '0', '0', '0', '_','a','b','c','d'};


#if SIG_PROC_ENABLE
/*------------------------------------------------------------------- l2cap data pkt(SIG) ---------------------------------------------------*
 | stamp_time(4B) |llid nesn sn md |  pdu-len   | l2cap_len(2B)| chanId(2B)| Code(1B)|Id(1B)|Data-Len(2B) |           Result(2B)             |
 |                |   type(1B)     | rf_len(1B) |       L2CAP header       |          SIG pkt Header      |  SIG_Connection_param_Update_Rsp |
 |                |                |            |     0x0006   |    0x05   |   0x13  | 0x01 |  0x0002     |             0x0000               |
 |                |          data_headr         |                                                       payload                              |
 *-------------------------------------------------------------------------------------------------------------------------------------------*/
u8 conn_update_cnt;//閺夆晝鍋炵敮鎾矗閸屾稒娈秚able闁汇劌瀚紞瀣礈瀹勵柎dex
int att_sig_proc_handler (u16 connHandle, u8 * p)
{
	rf_pkt_l2cap_sig_connParaUpRsp_t* pp = (rf_pkt_l2cap_sig_connParaUpRsp_t*)p;

#if 0//test debug
	foreach(i, 16){PrintHex(*((u8*)pp + i));}ble_printf(".\n");
#endif

	u8 sig_conn_param_update_rsp[9] = { 0x0A, 0x06, 0x00, 0x05, 0x00, 0x13, 0x01, 0x02, 0x00 };
	if(!memcmp(sig_conn_param_update_rsp, &pp->rf_len, 9) && ((pp->type&0b11) == 2)){//l2cap data pkt, start pkt
		if(pp->result == 0x0000){
			ble_printf("SIG: the LE master Host has accepted the connection parameters.\n");
			conn_update_cnt = 0;
		}
		else if(pp->result == 0x0001)
		{
			ble_printf("SIG: the LE master Host has rejected the connection parameters..\n");
			ble_printf("Current Connection interval:%dus.\n", bls_ll_getConnectionInterval() * 1250 );
			conn_update_cnt++;
            if(conn_update_cnt < 4){
            	ble_printf("Slave sent update connPara req!\n");
            }
			if(conn_update_cnt == 1){
				bls_l2cap_requestConnParamUpdate (8, 16, 0, 400);//18.75ms iOS
			}
			else if(conn_update_cnt == 2){
				bls_l2cap_requestConnParamUpdate (16,32, 0, 400);
			}
			else if(conn_update_cnt == 3){
				bls_l2cap_requestConnParamUpdate (32,60, 0, 400);
			}
			else{
				conn_update_cnt = 0;
				ble_printf("Slave Connection Parameters Update table all tested and failed!\n");
			}
		}
	}

}
#endif

/*
 * @Desription:
 * @param	:
 * @return 	:
 */
void entry_ota_mode(void)
{
	g_System_Status |= SET_OTA_ISWORKING;
	gm_printf("entry_ota_mode .\r\n");
	bls_ota_setTimeout(0xffffff00); //set OTA timeout  100 S
}

/*
 * @Desription:
 * @param	:
 * @return 	:
 */

void show_ota_result(int result)
{
	g_System_Status &= ~SET_OTA_ISWORKING;

	if(result == OTA_SUCCESS){
		ble_printf("OTA updata system success.code %d\r\n",result);

	}
	else{
		ble_printf("OTA updata system failure.code %d\r\n",result);
		send_response_pkg(BLE_MSG_SET_HEAD,BLE_MSG_TYPE_SYSTEM_OTA,(char)result);
		WaitMs(100);
	}
}

/*
 * @Desription:
 * @param	:
 * @return 	:
 */
#define MAX_INTERVAL_VAL		30
void	task_connect (void)
{
	//bls_l2cap_requestConnParamUpdate (12, 32, 0, 400);

	//update connParam
	if(bls_ll_getConnectionInterval() > MAX_INTERVAL_VAL)//
	{
		ble_printf("ConnInterval > 20ms.\nSlave sent update connPara req!\n");
	    bls_l2cap_requestConnParamUpdate(MAX_INTERVAL_VAL, MAX_INTERVAL_VAL, 0, 400);
	}
	else
	{
		ble_printf("ConnInterval < 20ms.\nSlave NOT need sent update connPara req!\n");
		ble_printf("Connection interval:%dus.\n", bls_ll_getConnectionInterval() * 1250 );
	}
	
	gpio_write(STATE_LED,ON);
}

/*
 * @Desription:
 * @param	:
 * @return 	:
 */
void app_switch_to_indirect_adv(u8 e, u8 *p, int n)
{
	ble_printf("BLT_EV_FLAG_ADV_DURATION_TIMEOUT \r\n");
	bls_ll_setAdvParam( ADV_INTERVAL_30MS, ADV_INTERVAL_30MS + 16,
						ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC,
						0,  NULL,
						MY_APP_ADV_CHANNEL,
						ADV_FP_NONE);

	bls_ll_setAdvEnable(1);  //must: set adv enable
}


/*
 * @Desription:
 * @param	:
 * @return 	:
 */
void task_connectEvent(u8 e,u8 *p,int n)//
{
//	bls_l2cap_requestConnParamUpdate (12, 32, 0, 400);

	ble_printf("conn event");
	g_System_Status |= SET_HEARTBEAT;
	//update connParam
	if(bls_ll_getConnectionInterval() > MAX_INTERVAL_VAL)//
	{
	    ble_printf("ConnInterval > 20ms.\nSlave sent update connPara req!\n");
	    bls_l2cap_requestConnParamUpdate(MAX_INTERVAL_VAL, MAX_INTERVAL_VAL, 0, 400);
	}
	else
	{
		ble_printf("ConnInterval < 20ms.\nSlave NOT need sent update connPara req!\n");
		ble_printf("Connection interval:%dus.\n", bls_ll_getConnectionInterval() * 1250 );
	}

	gpio_write(STATE_LED, ON);
}

/*
 * @Desription:
 * @param	:
 * @return 	:
 */
void task_TerminateEvent(u8 e,u8 *p,int n)
{
	ble_printf("--------Terminate event---------\r\n");
	if(*p == HCI_ERR_CONN_TIMEOUT){
		ble_printf("HCI_ERR_CONN_TIMEOUT\r\n");
	}
	else if(*p == HCI_ERR_REMOTE_USER_TERM_CONN){  //0x13
		ble_printf("HCI_ERR_REMOTE_USER_TERM_CONN\r\n");
	}
	else if(*p == HCI_ERR_CONN_TERM_MIC_FAILURE){
		ble_printf("HCI_ERR_CONN_TERM_MIC_FAILURE\r\n");
	}
	else{
		ble_printf("Don't know why Terminate\r\n");
	}

	gpio_write(STATE_LED, OFF);

}

/*
 * @Desription:
 * @param	:
 * @return 	:
 */
void task_CONN_PARA_REQ_Event(u8 e,u8 *p,int n)
{
	//Slave received Master's LL_Connect_Update_Req pkt.
	rf_packet_ll_updateConnPara_t pp;
	memcpy((u8*)&pp.winSize, p, 11);

	ble_printf("Receive Master's LL_Connect_Update_Req pkt.\n");
	ble_printf("Connection interval:%dus.\n", pp.interval*1250);
}


/*
 * @Desription:
 * @param	:
 * @return 	:
 */
void Init_led(void)
{
	gpio_set_func(STATE_LED, AS_GPIO);
	gpio_set_output_en(STATE_LED, 1);//enable output
	gpio_set_input_en(STATE_LED, 0); //disable input
	gpio_write(STATE_LED, OFF);

}

/*
 * @Desription:  Execute heartbeat mechanism and whether need to write flash
 * @param	:
 * @return 	:
 */
int task_ExecuteRegular_BySecond(void)
{
	u16 timeBrxREQ =0;
	u8 bleState =0;
#if 1
	//configuration save
	if( g_System_Status &NEED_WRITE_FLASH&&(g_System_Status &SET_OTA_ISWORKING)==0){
		timeBrxREQ = bls_ll_requestConnBrxEventDisable();//
		if(timeBrxREQ == 0xffff)
		{
			gm_printf("**ready to save config to flash. timeBrxREQ= %d\r\n",timeBrxREQ);
			bls_ll_disableConnBrxEvent();
			Config_Write_to_Flash();
			bls_ll_restoreConnBrxEvent();
			g_System_Status &= ~NEED_WRITE_FLASH;//clear
			gm_printf("end updata config to flash success.\r\n");
		}
	}
#endif
	//heartbeat
	if(g_System_Status &SET_HEARTBEAT){
		g_System_Status &=(~SET_HEARTBEAT);//clear
	}
	else{

		timeBrxREQ = bls_ll_requestConnBrxEventDisable();//
		bleState =blc_ll_getCurrentState();//

		if(((g_System_Status&SET_HEARTBEAT)==0) && (bleState == BLS_LINK_STATE_CONN) && (timeBrxREQ<10) && (g_System_Status &SET_OTA_ISWORKING)==0 ){
			bls_ll_terminateConnection(HCI_ERR_REMOTE_USER_TERM_CONN);//
			gm_printf("server initiate to terminate the connection .timeBrxREQ= %d\r\n",timeBrxREQ);
		}
	}
	return 0;
}

/*
 * @Desription: 闁烩偓鍔嶉崺娑㈠礆濠靛棭娼楅柛鏍垫嫹
 * @param	:
 * @return 	:
 */
 
void tl8267_Uart_Init()
{
	Init_uart();				//闂侇偄绻嬬槐鍫曞礆濠靛棭娼楅柛鏍垫嫹
}

void tl8267_timer_init(void)
{
	
	//timer0 10ms interval irq
	reg_irq_mask |= FLD_IRQ_TMR0_EN;
	reg_tmr0_tick = 0; //claer counter
	reg_tmr0_capt = 1 * CLOCK_SYS_CLOCK_1MS;
	reg_tmr_sta = FLD_TMR_STA_TMR0; //clear irq status
	reg_tmr_ctrl |= FLD_TMR0_EN;  //start timer

	//timer1 1s interval irq
	reg_irq_mask |= FLD_IRQ_TMR1_EN;
	reg_tmr1_tick = 0; //claer counter
	reg_tmr1_capt = 1000 * CLOCK_SYS_CLOCK_1MS;
	reg_tmr_sta = FLD_TMR_STA_TMR1; //clear irq status
	reg_tmr_ctrl |= FLD_TMR1_EN;  //start timer
	irq_enable();
	
}
extern u32 execute_step_time;
extern u32 execute_hold_time;

_attribute_ram_code_ void timer_irq_proc(void){
	static u8 ms10;
	if(reg_tmr_sta & FLD_TMR_STA_TMR0){
		reg_tmr_sta = FLD_TMR_STA_TMR0; //clear irq status
		ms10++;
		if(ms10 >=10){
			ms10 =0;
			Scan_KeyLow_Value();//10ms period to scan
		}
		if(execute_step_time!=0 && execute_step_time>=1)execute_step_time--;
		if(execute_hold_time!=0 && execute_hold_time>=1)execute_hold_time--;
	}
	if(reg_tmr_sta & FLD_TMR_STA_TMR1){		//1s
		reg_tmr_sta = FLD_TMR_STA_TMR1; //clear irq status
		g_time_cur.sec++;
		if(g_time_cur.sec==60){
			g_time_cur.sec = 0;
			g_time_cur.min++;	
			if(g_time_cur.min==60){
				g_time_cur.min=0;
				g_time_cur.hour++;
				if(g_time_cur.hour==24){
					g_time_cur.hour = 0;
				}
			}
		}
	}
}


void tl8267_Bsp_Init()
{
	////////////////// SPP initialization ///////////////////////////////////
	Init_amicAudio();//AUDIO initialization
	Init_led();//user Led Init
	Init_key();//Key up dowm middle
	zfft_init();//zfft
	libzled_init();
	tl8267_timer_init();
	blt_soft_timer_init();
	blt_soft_timer_add(task_ExecuteRegular_BySecond, 5000000);//5s
}

void tl8267_Ble_Init()
{
	blc_app_loadCustomizedParameters();  //load customized freq_offset cap value and tp value

	u8  tbl_mac [] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11};
	u32 *pmac = (u32 *) CFG_ADR_MAC;
	if (*pmac != 0xffffffff)//read chip mac
	{
	    memcpy (tbl_mac, pmac, 6);
	}
    else
    {
        //TODO : should write mac to flash after pair OK
        tbl_mac[0] = (u8)rand();
        flash_write_page (CFG_ADR_MAC, 6, tbl_mac);
    }
	gm_printf("mac:%x\r\n",*pmac);

	char name[5];
	sprintf(name,"%02X%02X",tbl_mac[1],tbl_mac[0]);
	for(u8 i=0;i<4;i++)
	{
		tbl_advData[16+i]=name[i];
		tbl_scanRsp[16+i]=name[i];
		device_name[14+i]=name[i];
	}
	for(u8 i = 0; i< 5;i++)
	{
		tbl_advData[10+i]=(u8)productName[i];
		tbl_scanRsp[10+i]=(u8)productName[i];
		device_name[8+i]=(u8)productName[i];
	}

///////////// BLE stack Initialization ////////////////
	////// Controller Initialization  ////////// Controller Layer
	blc_ll_initBasicMCU(tbl_mac);   //mandatory IDLE閻樿埖锟斤拷
	blc_ll_initAdvertising_module(tbl_mac); 	//adv module: 		 mandatory for BLE slave,闁跨喎顫曢幘锟�
	blc_ll_initSlaveRole_module();				//slave module: 	 mandatory for BLE slave,闁跨喐甯撮弬銈嗗閼癸拷
	blc_ll_initPowerManagement_module();        //pm module:      	 optional


////// Host Initialization  //////////
	my_att_init (); //gatt initialization
	blc_l2cap_register_handler (blc_l2cap_packet_receive);  	//l2cap initialization
#if SIG_PROC_ENABLE
	blc_l2cap_reg_att_sig_hander(att_sig_proc_handler);         //register sig process handler
#endif

	//smp initialization 闂佹澘绉撮锟�
#if ( SMP_DISABLE )
	bls_smp_enableParing (SMP_PARING_DISABLE_TRRIGER );
#elif ( SMP_JUST_WORK || SMP_PASSKEY_ENTRY )
	//Just work encryption: TK default is 0, that is, pin code defaults to 0, without setting
	//Passkey entry encryption: generate random numbers, or set the default pin code, processed in the event_handler function
	bls_smp_enableParing (SMP_PARING_CONN_TRRIGER );
	#if (SMP_PASSKEY_ENTRY )
		blc_smp_enableAuthMITM (1, 123456);
	#endif
#endif

///////////////////// USER application initialization ///////////////////
	bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));
	//change BLE connecting name
	bls_att_setDeviceName(device_name, sizeof(device_name));

////////////////// config adv packet /////////////////////
	u8 status = bls_ll_setAdvParam( ADV_INTERVAL_30MS, ADV_INTERVAL_30MS + 16,	//30ms妤犵偞瀵ч幐杈ㄧ▔閿熶粙鏌嗛敓锟�
								 ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC,//妤犵偞瀵ч幐閬嶆儍閸曨叀顫﹂柛顭掓嫹
								 0,  NULL,
								 MY_APP_ADV_CHANNEL,
								 ADV_FP_NONE);//閻犱礁澧介悿鍡涚嵁閹稿孩灏￠弶鈺佹处閹讹拷

	if(status != BLE_SUCCESS)
	{
		write_reg8(0x8000, 0x11);
		while(1);
	}  //debug: adv setting err


    gm_printf("Adv parameters setting success!\r\n");
	bls_ll_setAdvEnable(1);  //adv enable
	gm_printf("Enable ble adv!\r\n");
	rf_set_power_level_index (RF_POWER_8dBm);

	bls_pm_setSuspendMask (SUSPEND_DISABLE);//(SUSPEND_ADV | SUSPEND_CONN)

///////////////////濞存粌顑勫▎銏ゅ礆濠靛棭娼楅柛鏍垫嫹//////////////////////////////////////
	//todo by 8267_mod lib
//	blc_hci_registerControllerEventHandler(event_handler);		//register event callback
	//by 8267_32m lib
	bls_app_registerEventCallback(BLT_EV_FLAG_CONNECT,&task_connectEvent);//
	bls_app_registerEventCallback(BLT_EV_FLAG_TERMINATE,&task_TerminateEvent);//
	bls_app_registerEventCallback(BLT_EV_FLAG_CONN_PARA_REQ,&task_CONN_PARA_REQ_Event);//
	bls_app_registerEventCallback(BLT_EV_FLAG_ADV_DURATION_TIMEOUT,&app_switch_to_indirect_adv);//

	bls_hci_mod_setEventMask_cmd(0xffff);			//enable all 15 events,event list see ble_ll.h
	//HCI layer callback function
	blc_register_hci_handler(rx_from_uart_cb,tx_to_uart_cb);				//customized uart handler
///////////////////// OTA init//////////////////
	bls_ota_clearNewFwDataArea(); //must
	bls_ota_registerStartCmdCb(entry_ota_mode);
	bls_ota_registerResultIndicateCb(show_ota_result);

}


void tl8267_Ble_loop()
{
	////////////////////////////////////// BLE entry /////////////////////////////////
	blt_sdk_main_loop();
	////////////////////////////////////// UI entry /////////////////////////////////
	blt_soft_timer_process(MAINLOOP_ENTRY);
}

