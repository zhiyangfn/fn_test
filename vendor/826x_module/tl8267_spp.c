#include "../../proj/tl_common.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../../proj_lib/ble/ll/ll.h"
#include "../../proj_lib/ble/blt_config.h"
#include "../../proj_lib/ble/ble_smp.h"
#include "../../proj/drivers/uart.h"

#include "libbleprotocol.h"
#include "tl8267_spp.h"
#include "tl8267_uart.h"

extern my_fifo_t hci_rx_fifo;
extern my_fifo_t hci_tx_fifo;
/////////////////////////////////////////////
extern void app_suspend_exit ();
extern int rx_from_uart_cb (void);
extern int tx_to_uart_cb (void);
extern int event_handler(u32 h, u8 *para, int n);
int bls_uart_handler (u8 *p, int n);
int hci_send_data (u32 h, u8 *para, int n);

#if 0
///////////the code below is just for demonstration of the event callback only////////////
int event_handler(u32 h, u8 *para, int n)
{
	if((h&HCI_FLAG_EVENT_TLK_MODULE)!= 0)			//module event
	{
		Debug_printf("event2\r\n");
		switch((u8)(h&0xff))
		{
			case BLT_EV_FLAG_SCAN_RSP:
				break;
			case BLT_EV_FLAG_CONNECT:
			{
				u32 header;
				header = 0x0780 + BLT_EV_FLAG_CONNECT;		//state change event
				header |= HCI_FLAG_EVENT_TLK_MODULE;
				hci_send_data(header, NULL, 0);		//HCI_FLAG_EVENT_TLK_MODULE
				Debug_printf("\n\r************** Connection event occured! **************\n\r");
				task_connect();
				gpio_write(STATE_LED,ON);
			}
				break;
			case BLT_EV_FLAG_TERMINATE://终止事件
			{
				u32 header;
				header = 0x0780 + BLT_EV_FLAG_TERMINATE;		//state change event
				header |= HCI_FLAG_EVENT_TLK_MODULE;
				hci_send_data(header, NULL, 0);		//HCI_FLAG_EVENT_TLK_MODULE
				Debug_printf("\n\r************** Terminate event occured! **************\n\r");

				gpio_write(STATE_LED,OFF);
			}
				break;
			case BLT_EV_FLAG_PAIRING_BEGIN:
			{
#if(SMP_PASSKEY_ENTRY)
	#if (PINCODE_RANDOM_ENABLE)//Randomly generated PINCODE, need to have the ability to print out it //Debug_printf, here
				u32 pinCode_random;
				u8 pc[7] = { '0','0','0','0','0','0', '\0'};
				generateRandomNum(4, (u8*)&pinCode_random);
				pinCode_random &= 999999;//0~999999
				pc[0] = (pinCode_random/100000) + '0';
				pc[1] = (pinCode_random%100000)/10000 + '0';
				pc[2] = ((pinCode_random%100000)%10000)/1000 + '0';
				pc[3] = (((pinCode_random%100000)%10000)%1000)/100 + '0';
				pc[4] = ((((pinCode_random%100000)%10000)%1000)%100)/10 + '0';
				pc[5] = pinCode_random%10 + '0';
				Debug_printf("PIN Code Number : %s\n", pc);

				blc_smp_enableAuthMITM (1, pinCode_random);//pincode
				blc_smp_setIoCapability (IO_CAPABLITY_DISPLAY_ONLY);
	#else//Popup dialog box on your phone , you need to enter the pincode:123456 to match
				blc_smp_enableAuthMITM (1, 123456);//pincode
				blc_smp_setIoCapability (IO_CAPABLITY_DISPLAY_ONLY);
	#endif
#endif
			}
				break;
			case BLT_EV_FLAG_PAIRING_END:
				break;
			case BLT_EV_FLAG_ENCRYPTION_CONN_DONE:
				break;
			case BLT_EV_FLAG_GPIO_EARLY_WAKEUP:
				break;
			case BLT_EV_FLAG_CHN_MAP_REQ:
				break;
			case BLT_EV_FLAG_CONN_PARA_REQ:
			{
				//Slave received Master's LL_Connect_Update_Req pkt.
				rf_packet_ll_updateConnPara_t p;
				memcpy((u8*)&p.winSize, para, 11);

				Debug_printf("Receive Master's LL_Connect_Update_Req pkt.\n");
				Debug_printf("Connection interval:%dus.\n", p.interval*1250);
			}
				break;

			case BLT_EV_FLAG_CONN_PARA_UPDATE:
			{
				u32 header;
				header = 0x0780 + BLT_EV_FLAG_CONN_PARA_UPDATE;		//state change event
				header |= HCI_FLAG_EVENT_TLK_MODULE;
				hci_send_data(header, NULL, 0);		//HCI_FLAG_EVENT_TLK_MODULE

				//Master send SIG_Connection_Param_Update_Rsp pkt,and the reply result is 0x0000. When connection event counter value is equal
				//to the instant, a callback event BLT_EV_FLAG_CONN_PARA_UPDATE will generate. The connection interval at this time should be the
				//currently updated and valid connection interval!
				Debug_printf("Update param event occur.\n");
				Debug_printf("Current Connection interval:%dus.\n", bls_ll_getConnectionInterval() * 1250);
			}
				break;

			case BLT_EV_FLAG_CHN_MAP_UPDATE:
			{
				u32 header;
				header = 0x0780 + BLT_EV_FLAG_CHN_MAP_UPDATE;		//state change event
				header |= HCI_FLAG_EVENT_TLK_MODULE;
				hci_send_data(header, NULL, 0);		//HCI_FLAG_EVENT_TLK_MODULE
			}
				break;

			case BLT_EV_FLAG_ADV_DURATION_TIMEOUT:
			{
				app_switch_to_indirect_adv();
			}

				break;

			case BLT_EV_FLAG_SUSPEND_ENTER:
				ADC_MODULE_CLOSED;
				break;
			case BLT_EV_FLAG_SUSPEND_EXIT:
				//app_suspend_exit ();
				break;
			default:
				break;
		}
	}
	return 0;
}
#endif

/*
 * @Desription:
 * @param	:
 * @return 	:
 */
u8 Send_Data_to_Phone(char *dataBuff , u16 len)
{
	u8 data_to_phone[50];
	u8 _cnt =0;

	len+=2;
	data_to_phone[0] = 0x1c;
	data_to_phone[1] = 0xff;
	data_to_phone[2] = (u8)len;
	data_to_phone[3] = (u8)( len>>8 );
	data_to_phone[4] = 0x11;
	data_to_phone[5] = 0;

	for(_cnt=0 ;_cnt < len ; _cnt++){
		data_to_phone[_cnt+6] = *dataBuff++;
	}

	bls_uart_handler(data_to_phone,len+6);
//	for(u8 i=0;i<len+6;i++)
//		U1_printf("%x ",data_to_phone[i]);
	return 0;
}


///////////////////////////////////////////the default bls_uart_handler///////////////////////////////
int bls_uart_handler (u8 *p, int n)
{
	u8  status = BLE_SUCCESS;
	int  cmdLen;
	u8 *cmdPara;

	u16 cmd = p[0]|p[1]<<8;
	u32	header = 0;
	u8	para[16] = {0};
	u8 para_len = 1;
	cmdPara = p + 4;
	cmdLen = p[2] | p[3]<<8;//低位在前 高位在后
	header = ((p[0] + p[1] * 256) & 0x3ff) | 0x400;		//event complete
	header |= (3 << 16) | HCI_FLAG_EVENT_TLK_MODULE;
#if 0
	// set advertising interval: 01 ff 02 00 50 00: 80 *0.625ms
	if (cmd == SPP_CMD_SET_ADV_INTV)
	{
		u8 interval = cmdPara[0] ;
		status = bls_ll_setAdvInterval(interval, interval);
	}
	// set advertising data: 02 ff 06 00 01 02 03 04 05 06
	else if (cmd == SPP_CMD_SET_ADV_DATA)
	{
		status = (u8)bls_ll_setAdvData(cmdPara, p[2]);
	}
	// enable/disable advertising: 0a ff 01 00  01
	else if (cmd == SPP_CMD_SET_ADV_ENABLE)
	{
		status = (u8)bls_ll_setAdvEnable(cmdPara[0]);
		para[0] = status;
	}
	// send data: 0b ff 05 00  01 02 03 04 05
	//change format to 0b ff 07 handle(2bytes) 00 01 02 03 04 05
	else if (cmd == 0xFF0B)
	{

	}
	// get module available data buffer: 0c ff 00  00
	else if (cmd == SPP_CMD_GET_BUF_SIZE)
	{
		u8 r[4];
		para[0] = (u8)blc_hci_le_readBufferSize_cmd( (u8 *)(r) );
		para[1] = r[2];
		para_len = 2;
	}
	// set advertising type: 0d ff 01 00  00
	else if (cmd == SPP_CMD_SET_ADV_TYPE)
	{
		status = bls_ll_setAdvType(cmdPara[0]);
	}
	// set advertising addr type: 0e ff 01 00  00
	else if (cmd == SPP_CMD_SET_ADV_ADDR_TYPE)
	{
		status = blt_set_adv_addrtype(cmdPara[0]);
	}
	// set advertising direct initiator address & addr type: 0e ff 07 00  00(public; 1 for random) 01 02 03 04 05 06
	else if (cmd == SPP_CMD_SET_ADV_DIRECT_ADDR)
	{
		status = blt_set_adv_direct_init_addrtype(cmdPara);
	}
	// add white list entry: 0f ff 07 00 01 02 03 04 05 06
	else if (cmd == SPP_CMD_ADD_WHITE_LST_ENTRY)
	{
		status = (u8)ll_whiteList_add(cmdPara[0], cmdPara + 1);
	}
	// delete white list entry: 10  ff 07 00 01 02 03 04 05 06
	else if (cmd == SPP_CMD_DEL_WHITE_LST_ENTRY)
	{
		status = (u8)ll_whiteList_delete(cmdPara[0], cmdPara + 1);
	}
	// reset white list entry: 11 ff 00 00
	else if (cmd == SPP_CMD_RST_WHITE_LST)
	{
		status = (u8)ll_whiteList_reset();
	}
	// set filter policy: 12 ff 10 00 00(bit0: scan WL enable; bit1: connect WL enable)
	else if (cmd == SPP_CMD_SET_FLT_POLICY)
	{
		status = bls_ll_setAdvFilterPolicy(cmdPara[0]);
	}
	// set device name: 13 ff 0a 00  01 02 03 04 05 06 07 08 09 0a
	else if (cmd == SPP_CMD_SET_DEV_NAME)
	{
		status = bls_att_setDeviceName(cmdPara,p[2]);
	}
	// get connection parameter: 14 ff 00 00
	else if (cmd == SPP_CMD_GET_CONN_PARA)
	{
		blt_get_conn_para(para+1);
		para_len = 11;
	}
	// set connection parameter: 15 ff 08 00 a0 00 a2 00 00 00 2c 01 (min, max, latency, timeout)
	else if (cmd == SPP_CMD_SET_CONN_PARA)
	{
		bls_l2cap_requestConnParamUpdate(cmdPara[0]|cmdPara[1]<<8,cmdPara[2]|cmdPara[3]<<8,cmdPara[4]|cmdPara[5]<<8,cmdPara[6]|cmdPara[7]<<8);
	}
	// get module current work state: 16 ff 00 00
	else if (cmd == SPP_CMD_GET_CUR_STATE)
	{
		para[1] = blc_ll_getCurrentState();
		para_len = 2;
	}
	// terminate connection: 17 ff 00 00
	else if (cmd == SPP_CMD_TERMINATE)
	{
		bls_ll_terminateConnection(HCI_ERR_REMOTE_USER_TERM_CONN);
	}
	// restart module: 18 ff 00 00
	else if (cmd == SPP_CMD_RESTART_MOD)
	{
		cpu_sleep_wakeup(DEEPSLEEP_MODE, PM_WAKEUP_TIMER, clock_time() + 10000 * sys_tick_per_us);
	}
	// enable/disable MAC binding function: 19 ff 01 00 00(disable, 01 enable)
	else if (cmd == 0x19)
	{

	}
	// add MAC address to binding table: 1a ff 06 00 01 02 03 04 05 06
	else if (cmd == 0x1a)
	{

	}
	// delete MAC address from binding table: 1b ff 06 00 01 02 03 04 05 06
	else if (cmd == 0x1b)
	{

	}
#endif
	//change format to 1c ff 07 00 11 00 01 02 03 04 05
	 if (cmd == SPP_CMD_SEND_NOTIFY_DATA)
	{
		if (cmdLen > 42)
		{
			status = 2;			//data too long
		}
		else
		{
			status = bls_att_pushNotifyData( cmdPara[0] | (cmdPara[1]<<8), cmdPara + 2,  cmdLen - 2);
		}
	}
	para[0] = status;
	hci_send_data (header, para, para_len);
	return 0;
}


int hci_send_data (u32 h, u8 *para, int n)
{

	u8 *p = my_fifo_wptr (&hci_tx_fifo);
	if (!p || n >= hci_tx_fifo.size)
	{
		return -1;
	}

	int nl = n + 4;
	if (h & HCI_FLAG_EVENT_TLK_MODULE)
	{
		*p++ = nl;
		*p++ = nl >> 8;
		*p++ = 0xff;
		*p++ = n + 2;
		*p++ = h;
		*p++ = h>>8;
		memcpy (p, para, n);
		p += n;
	}
	my_fifo_next (&hci_tx_fifo);
	return 0;
}


/////////////////////////////////////blc_register_hci_handler for spp////////////////////////////
int rx_from_uart_cb (void)//UART data send to Master,we will handler the data as CMD or DATA
{
	if(my_fifo_get(&hci_rx_fifo) == 0)//r==w,mean no data
	{
		return 0;
	}

	u8* p = my_fifo_get(&hci_rx_fifo);
	u32 rx_len = p[0]; //usually <= 255 so 1 byte should be sufficient

	if (rx_len)
	{
		bls_uart_handler(&p[4], rx_len - 4);
		my_fifo_pop(&hci_rx_fifo);//读指针加一
	}

	return 0;
}


int tx_to_uart_cb (void)
{
	rec_data_buf rx_buf_f_hci;//{len, data}
	u8 *p = my_fifo_get (&hci_tx_fifo);//从 hci层获取数据
	u16 datalen;
	if(p != NULL)
	{
		memset(rx_buf_f_hci.data,0,25);

		datalen=(p[1]<<8|p[0]);//rec length

		if(datalen>7)
		{
			rx_buf_f_hci.len=datalen-7;
			memcpy(rx_buf_f_hci.data , p + 9, rx_buf_f_hci.len);//p0 p1 is data lenght
			ble_protocol_work(&rx_buf_f_hci);
		}

//		for(u8 i=0;i<datalen +2 ;i++)
//			U1_printf("%x ",*(p+i));
//		U1_printf("len %d\r\n",datalen);

		my_fifo_pop (&hci_tx_fifo);//弹出一个fifo

	}
	return 0;
}

