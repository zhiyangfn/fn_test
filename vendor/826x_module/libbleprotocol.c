#include "../../proj/tl_common.h"

#include "libbleprotocol.h"
#include "tl8267_uart.h"
#include "libzledsend.h"
#include "tl8267_spp.h"
#include "libflashdata.h"
#include "libMicLight.h"
#include "libScenes.h"


#define MT_ERR -1
#define MT_OK 0


char libble_get_bcc_check_code(char *datas,char len){

	unsigned char bcc=0,i;
	for(i=0;i<len;i++)
	{
		bcc ^=datas[i];
	}
    return bcc;
}

u8 send_response_pkg(char head,char type,char errcode)
{
	char sbuf[BLE_SEND_PKG_LEN]={0},ret=0;
	sbuf[0]=head;
	sbuf[1]=type;
	sbuf[2]=errcode;
	sbuf[BLE_SEND_PKG_LEN-1]=libble_get_bcc_check_code(sbuf,BLE_SEND_PKG_LEN-1);
	ret =Send_Data_to_Phone(sbuf,BLE_SEND_PKG_LEN);
	return ret;
}

u8 send_request_pkg(char head,char type,char *data)
{
	char sbuf[BLE_SEND_PKG_LEN]={0},ret=0;
	sbuf[0]=head;
	sbuf[1]=type;
	if(type == BLE_MSG_TYPE_GET_VERSION){
		memcpy(&sbuf[2],data,7);
	}else{
		memcpy(&sbuf[2],data,BLE_MSG_NAME_MAX_LEN);
	}
	sbuf[BLE_SEND_PKG_LEN-1]=libble_get_bcc_check_code(sbuf,BLE_SEND_PKG_LEN-1);
	ret =Send_Data_to_Phone(sbuf,BLE_SEND_PKG_LEN);

//	for(u8 i=0;i<BLE_SEND_PKG_LEN ;i++)
//		ble_printf("%x ",*(sbuf+i));
//	ble_printf("len %d\r\n",BLE_SEND_PKG_LEN);

	return ret;
}

u8 libble_check_pkg(char *data)
{
	char bcc;
	bcc=libble_get_bcc_check_code(data,BLE_SEND_PKG_LEN-1);
	if(bcc!=data[BLE_SEND_PKG_LEN-1])
	{
		return MT_ERR;
	}
	return MT_OK;
}

static u8 tran_data_to_msg_pkg(char *data,ble_msg_t *msg)
{
	if(data==NULL||msg==NULL){
		ble_printf("data error (%x,%x)\n",data,msg);
		return MT_ERR;
	}
	msg->head=data[0];
	msg->type=data[1];
	memcpy(msg->data,&data[2],17);
	msg->check_code=data[19];
	return MT_OK;
}

u8 libble_msg_ble_name(char head,char *data)
{
//	int ret=0,len;
//	char error_code=0;
//	ble_printf("app %s ble name:%s\n",((head==BLE_MSG_SET_HEAD)?BLE_MSG_SET:BLE_MSG_GET),data);
//	if(head==BLE_MSG_SET_HEAD)
//	{
//		len=strlen(data);
//		if(len>BLE_MSG_NAME_MAX_LEN)
//			len=BLE_MSG_NAME_MAX_LEN;
//		memset(g_device_param.dev_name,0,MAX_STRING_LENGTH);
//		memcpy(g_device_param.dev_name,data,len);
//		ret = libini_set_string_value(DEVICE_FILE_NAME, "dev", "dev_name", g_device_param.dev_name);
//		if ( ret !=MT_OK) {
//			ble_printf("set ble name error\n");
//			error_code=0x1;
//
//		}
//		libble_set_name(g_device_param.dev_name);
//		send_response_pkg(BLE_MSG_SET_HEAD,BLE_MSG_TYPE_BLE_NAME,error_code);
//		sleep(1);
//		libble_reboot();
//
//	}else
//	{
//		memset(g_device_param.dev_name,0,MAX_STRING_LENGTH);
//		ret = libini_get_string_value(CONFIG_FILE_NAME, "dev", "dev_name", g_device_param.dev_name,"");
//		if ( ret !=MT_OK) {
//			ble_printf("get ble name error\n");
//		}
//		memset(data,0,BLE_MSG_NAME_MAX_LEN+1);
//		len=strlen(g_device_param.dev_name);
//		if(len>BLE_MSG_NAME_MAX_LEN)
//			len=BLE_MSG_NAME_MAX_LEN;
//		memcpy(data,g_device_param.dev_name,len);
//		send_request_pkg(BLE_MSG_GET_HEAD,BLE_MSG_TYPE_BLE_NAME,data);
//
//	}
	return 0;
}

u8 libble_msg_led_status(char head,char *data)
{
	static u8 time = 0;
	g_System_Status |= SET_HEARTBEAT;
	if(head==BLE_MSG_SET_HEAD)
	{
		ble_printf("ble_set:led_status:%d\r\n",data[0]);
		g_config_param.led_status=data[0];
		if(g_config_param.led_status == 1){//open the led strip
			start_led_service();
		}else{		//close the led stri
			stop_led_service(g_config_param.led_work_mode.mode);
		}
		send_response_pkg(BLE_MSG_SET_HEAD,BLE_MSG_TYPE_LED_STATUS,0);

	}else
	{
		time++;
		if(time==30){		// 1min to printf
			time = 0;
			ble_printf("ble_get:led_status:%d,time %d-%d-%d\r\n",g_config_param.led_status,\
							g_time_cur.hour,g_time_cur.min,g_time_cur.sec);
		}
		memset(data,0,BLE_MSG_NAME_MAX_LEN);
		data[0]=g_config_param.led_status;
		send_request_pkg(BLE_MSG_GET_HEAD,BLE_MSG_TYPE_LED_STATUS,data);
	}
	return 0;
}

u8 libble_msg_led_nums(char head,char *data)
{
	ble_printf("app %s led nums:%d\n",((head==BLE_MSG_SET_HEAD)?BLE_MSG_SET:BLE_MSG_GET),data[0]);
	if(head==BLE_MSG_SET_HEAD)
	{
		g_config_param.led_nums=data[0];
		send_response_pkg(BLE_MSG_SET_HEAD,BLE_MSG_TYPE_LED_NUMS,0);

	}else
	{
		memset(data,0,BLE_MSG_NAME_MAX_LEN);
		data[0]=g_config_param.led_nums;
		send_request_pkg(BLE_MSG_GET_HEAD,BLE_MSG_TYPE_LED_NUMS,data);
	}
	return 0;
}

u8 libble_msg_led_light(char head,char *data)
{
	if(head==BLE_MSG_SET_HEAD)
	{
		ble_printf("ble_set:mode %d led_light:%d\r\n",g_config_param.led_work_mode.mode,data[0]);
		g_config_param.led_light=data[0];
		libzled_setMaxLuminance(g_config_param.led_light);
		if(g_config_param.led_work_mode.mode==LED_WORK_MODE_COLOR){
       			libzled_setConstColor(g_config_param.led_work_mode.color_mode.color[0],g_config_param.led_work_mode.color_mode.color[1],g_config_param.led_work_mode.color_mode.color[2],0);
      	}
		if(g_config_param.led_work_mode.mode==LED_WORK_MODE_SCENE){
			
		}
		send_response_pkg(BLE_MSG_SET_HEAD,BLE_MSG_TYPE_LED_LIGHT,0);

	}else
	{
		ble_printf("ble_get:led_light:%d\r\n",g_config_param.led_light);
		memset(data,0,BLE_MSG_NAME_MAX_LEN+1);
		data[0]=g_config_param.led_light;
		send_request_pkg(BLE_MSG_GET_HEAD,BLE_MSG_TYPE_LED_LIGHT,data);
	}
	return 0;
}

u8 libble_msg_work_mode(char head,char *data)
{
	u8 mode=(u8)data[0];
	u8 i,i1,i2;
	ble_printf("app %s led work mode:%d\r\n",((head==BLE_MSG_SET_HEAD)?BLE_MSG_SET:BLE_MSG_GET),data[0]);
	if(head==BLE_MSG_SET_HEAD)
	{
		
		ble_printf("ble_set:work_mode:%d\r\n",data[0]);
		if(g_config_param.led_work_mode.mode!=data[0])
		{
			if(LED_WORK_MODE_COLOR != g_config_param.led_work_mode.mode){				// add to for led light light
				stop_led_service(g_config_param.led_work_mode.mode);
			}
		}

		g_config_param.led_work_mode.mode=data[0];
		if(mode==1){//music
			g_config_param.led_work_mode.music_mode.mode=data[1];
			g_config_param.led_work_mode.music_mode.sensitivity=data[2];
			if(g_config_param.led_work_mode.music_mode.mode ==1 ||g_config_param.led_work_mode.music_mode.mode ==2){
				g_config_param.led_work_mode.music_mode.color[0]=data[3];
				g_config_param.led_work_mode.music_mode.color[1]=data[4];
				g_config_param.led_work_mode.music_mode.color[2]=data[5];
			}
		}
		if(mode==2){//color
			g_config_param.led_work_mode.color_mode.color[0]=data[1];
			g_config_param.led_work_mode.color_mode.color[1]=data[2];
			g_config_param.led_work_mode.color_mode.color[2]=data[3];
		}
		if(mode==3)// no ic music	
		{
			g_config_param.led_work_mode.mode=1;
			g_config_param.led_work_mode.music_mode.mode=data[1]+100;
			g_config_param.led_work_mode.music_mode.sensitivity=data[2];
			if(g_config_param.led_work_mode.music_mode.mode ==101){
				g_config_param.led_work_mode.music_mode.color[0]=data[3];
				g_config_param.led_work_mode.music_mode.color[1]=data[4];
				g_config_param.led_work_mode.music_mode.color[2]=data[5];
			}
			ble_printf("no ic music set %d\r\n",g_config_param.led_work_mode.music_mode.mode);
			ble_printf("no ic music set R:%d,G:%d,B:%d\r\n",g_config_param.led_work_mode.music_mode.color[0],\
				g_config_param.led_work_mode.music_mode.color[1],g_config_param.led_work_mode.music_mode.color[2]);
		}
		if(mode==4)	//scenes
		{
			gm_printf("-%d-%d-%d\r\n",data[0],data[1],data[2]);
			g_config_param.led_work_mode.scene_mode.mode = data[1];
		}
//************************add by fangnan in 20180620*************
//******************************diy mode************
		if(mode == 7)
		{
			diyPrintf("enter diy mode!!!\n");
			g_config_param.led_work_mode.diy_mode.serial_id=data[++i];
			g_config_param.led_work_mode.diy_mode.diy_master_mode=data[++i];
			g_config_param.led_work_mode.diy_mode.diy_slave_mode=data[++i];
			g_config_param.led_work_mode.diy_mode.diy_speed=data[++i];
			for(i1=0;i1<4;i1++)
				for(i2=0;i2<3;i2++)
					g_config_param.led_work_mode.diy_mode.diy_color[i1][i2] = (u8)data[++i];
			diyModeDetailPrintf();//diyPrintf("serial_id == %d\n",g_config_param.led_work_mode.diy_mode.serial_id);	
		}
//*********************************end diy mode******************	
		if(g_config_param.led_status==1)
			start_led_service();
		send_response_pkg(BLE_MSG_SET_HEAD,BLE_MSG_TYPE_WORK_MODE,0);
	}
	else
	{
	
		ble_printf("ble_get:led_light:%d\r\n",g_config_param.led_work_mode.mode);
		data[0]=g_config_param.led_work_mode.mode;
		mode=(u8)data[0];
		if(mode==0){		//video
			data[1]=g_config_param.led_work_mode.video_mode.mode;
			data[2]=g_config_param.led_work_mode.video_mode.part;
		}
		if(mode==1){		//music
			data[1]=g_config_param.led_work_mode.music_mode.mode;
			if(data[1]>99){
				data[1]=g_config_param.led_work_mode.music_mode.mode-100;
				data[0]=3;
				ble_printf("no ic music set %d-%d\r\n",data[0],data[1]);
			}
			data[2]=g_config_param.led_work_mode.music_mode.sensitivity;
			data[3]=g_config_param.led_work_mode.music_mode.color[0];
			data[4]=g_config_param.led_work_mode.music_mode.color[1];
			data[5]=g_config_param.led_work_mode.music_mode.color[2];
		}
		if(mode==2){		//color
			data[1]=g_config_param.led_work_mode.color_mode.color[0];
			data[2]=g_config_param.led_work_mode.color_mode.color[1];
			data[3]=g_config_param.led_work_mode.color_mode.color[2];
		}
		if(mode==4){		//scene
			data[1]=g_config_param.led_work_mode.scene_mode.mode;
			gm_printf("-%d-%d-%d\r\n",data[0],data[1],data[2]);
		}
		//diy mode,add by fn in 20180623
		if(mode ==7 ){
			data[1]=g_config_param.led_work_mode.diy_mode.serial_id;
			data[2]=g_config_param.led_work_mode.diy_mode.diy_master_mode;
			data[3]=g_config_param.led_work_mode.diy_mode.diy_slave_mode;
			data[4]=g_config_param.led_work_mode.diy_mode.diy_speed;
			for(i=0;i<12;i++)
				data[5+i]=g_config_param.led_work_mode.diy_mode.diy_color[i/3][i%3];
		}
		send_request_pkg(BLE_MSG_GET_HEAD,BLE_MSG_TYPE_WORK_MODE,data);
	}
	return 0;
}

u8 libble_msg_get_version(char head,char *data)
{
	if(head==BLE_MSG_GET_HEAD)
	{
		ble_printf("ble:app get version:%s\n",VersionNumber);
		send_request_pkg(BLE_MSG_GET_HEAD,BLE_MSG_TYPE_GET_VERSION,VersionNumber);
	}
	return 0;
}

u8 libble_msg_get_dev_info(char head,char *data)
{
	char send_data[20]={0};
	if(head==BLE_MSG_SET_HEAD){
		ble_printf("ble:dev info not support set\n");
		return MT_ERR;
	}
	ble_printf("ble:get dev info:sub_type=%d\n",data[0]);
	char sub_type=data[0];
	memset(send_data,0,20);
	send_data[0]=data[0];
	switch(sub_type)
	{

		case BLE_MSG_DEV_SUB_TYPE_NAME:
		{
			memcpy(&send_data[1],productName,strlen(productName));
			send_request_pkg(BLE_MSG_GET_HEAD,BLE_MSG_TYPE_GET_DEV_INFO,send_data);
		}
		break;
		case BLE_MSG_DEV_SUB_TYPE_HW_VERISON:		
		{			
			memcpy(&send_data[1],g_device_param.hw_version,strlen(g_device_param.hw_version));	
			send_request_pkg(BLE_MSG_GET_HEAD,BLE_MSG_TYPE_GET_DEV_INFO,send_data);		
		}	
		break;
		case BLE_MSG_DEV_SUB_TYPE_SW_VERSION:		
		{			
			send_request_pkg(BLE_MSG_GET_HEAD,BLE_MSG_TYPE_GET_DEV_INFO,send_data);		
		}		
		break;	

		case BLE_MSG_DEV_SUB_TYPE_MAC:		
		{			
			send_request_pkg(BLE_MSG_GET_HEAD,BLE_MSG_TYPE_GET_DEV_INFO,send_data);		
		}		
		break;
		case BLE_MSG_DEV_SUB_TYPE_TV_INCH:		
		{			
			send_data[1]=100;			
			send_request_pkg(BLE_MSG_GET_HEAD,BLE_MSG_TYPE_GET_DEV_INFO,send_data);		
		}	
		break;
		case BLE_MSG_DEV_SUB_TYPE_SN:		
		{			
			send_request_pkg(BLE_MSG_GET_HEAD,BLE_MSG_TYPE_GET_DEV_INFO,send_data);		
		}		
		break;		
		case BLE_MSG_DEV_SUB_TYPE_DATE:		
		{			
				memcpy(&send_data[1],g_device_param.production_date,strlen(g_device_param.production_date));		
				send_request_pkg(BLE_MSG_GET_HEAD,BLE_MSG_TYPE_GET_DEV_INFO,send_data);	
		}	
		break;
		
		default:
			ble_printf("msg type error :%x\n",sub_type);
		break;
		
	}
	return 0;
}

u8 libble_msg_update_time(char head,char *data)
{
	ble_printf("libble_msg_update_time\r\n");
	if(head==BLE_MSG_SET_HEAD)
	{
		ble_printf("libble set update_time\r\n");
		g_time_cur.enable = 1;
		g_time_cur.hour = data[0];
		g_time_cur.min  = data[1];
		g_time_cur.sec  = data[2];
		ble_printf("libble time_cur %d-%d-%d\r\n",g_time_cur.hour,g_time_cur.min,g_time_cur.sec);
		send_response_pkg(BLE_MSG_SET_HEAD,BLE_MSG_TYPE_UPDATE_TIME,0);
	}
	return 0;
}

u8 libble_msg_onoff_time(char head,char *data)
{
	if(head==BLE_MSG_SET_HEAD)
	{
		ble_printf("libble set onoff_time\r\n");
		g_config_param.switch_time.flag  = data[0];
		if(g_config_param.switch_time.flag==1){
			g_config_param.switch_time.on_hour = data[1];
			g_config_param.switch_time.on_min  = data[2];
			g_config_param.switch_time.off_hour = data[3];
			g_config_param.switch_time.off_min	= data[4];
		}
		send_response_pkg(BLE_MSG_SET_HEAD,BLE_MSG_TYPE_ONOFF_TIME,0);
	}
	else
	{
		ble_printf("libble get onoff_time\r\n");
		data[0] = g_config_param.switch_time.flag  ;
		data[1] = g_config_param.switch_time.on_hour ;
		data[2] = g_config_param.switch_time.on_min  ;
		data[3] = g_config_param.switch_time.off_hour;
		data[4] = g_config_param.switch_time.off_min ;
		send_request_pkg(BLE_MSG_GET_HEAD,BLE_MSG_TYPE_ONOFF_TIME,data);
	}
	return 0;
}

u8 libble_msg_alarm_num(char head,char *data)
{
	if(head==BLE_MSG_SET_HEAD)
	{
		send_response_pkg(BLE_MSG_SET_HEAD,BLE_MSG_TYPE_ALARM_NUM,0);
		Reset_flash_config();
		ble_printf("Reset flash config done!\r\n");
		while(1);
	}
	else
	{
		data[0] =2;
		ble_printf("get alarm num %d\r\n",data[0]);
		send_request_pkg(BLE_MSG_GET_HEAD,BLE_MSG_TYPE_ALARM_NUM,data);
	}
	return 0;
}


u8 libble_msg_product_test(char head,char *data)
{
	if(head==BLE_MSG_SET_HEAD)
	{
		g_RGBW_test_enable =1;
		send_response_pkg(BLE_MSG_SET_HEAD,BLE_MSG_TYPE_PRODUCT_TEST,0);
		stop_led_service();
		Reset_flash_config();
		ble_printf("entry product rgbw test mode \r\n");
	}
	else
	{
		send_request_pkg(BLE_MSG_GET_HEAD,BLE_MSG_TYPE_PRODUCT_TEST,data);
	}
	return 0;
}



u8 ble_protocol_work(rec_data_buf *dataBuf)
{
	char recv_buf[BLE_SEND_PKG_LEN]={0};
	char ret = 0;
	ble_msg_t msg;

	if( (dataBuf->len) != BLE_SEND_PKG_LEN)
	{
		ble_printf("len!=BLE_SEND_PKG_LEN \n");
		return -1;
	}

	memcpy(recv_buf,dataBuf->data,BLE_SEND_PKG_LEN);
	ret=libble_check_pkg(recv_buf);
	if(ret!=MT_OK){
		ble_printf("check pkg error\n");
		return -1;
	}
	tran_data_to_msg_pkg(recv_buf,&msg);

	if(msg.head==BLE_MSG_SET_HEAD||msg.head==BLE_MSG_GET_HEAD){
		if(msg.head==BLE_MSG_SET_HEAD)g_System_Status |= NEED_WRITE_FLASH;
		switch(msg.type)
			{
				case BLE_MSG_TYPE_BLE_NAME:					// *
				{
					libble_msg_ble_name(msg.head,msg.data);
				}
				break;

				case BLE_MSG_TYPE_LED_STATUS:				//
				{
					libble_msg_led_status(msg.head,msg.data);
				}
				break;

				case BLE_MSG_TYPE_LED_NUMS:					//*
				{
					libble_msg_led_nums(msg.head,msg.data);
				}
				break;
				case BLE_MSG_TYPE_LED_LIGHT:
				{
					libble_msg_led_light(msg.head,msg.data);
				}
				break;
				case BLE_MSG_TYPE_WORK_MODE:
				{
					libble_msg_work_mode(msg.head,msg.data);		
				}
				break;
				case BLE_MSG_TYPE_GET_VERSION:
				{
					libble_msg_get_version(msg.head,msg.data);
				}
				break;
				case BLE_MSG_TYPE_GET_DEV_INFO:
				{
					libble_msg_get_dev_info(msg.head,msg.data);
				}
				break;
				case BLE_MSG_TYPE_UPDATE_TIME:
				{
					libble_msg_update_time(msg.head,msg.data);

				}
				break;
				case BLE_MSG_TYPE_ONOFF_TIME:
				{
					libble_msg_onoff_time(msg.head,msg.data);
				}
				break;		

				case BLE_MSG_TYPE_ALARM_NUM:
				{
					libble_msg_alarm_num(msg.head,msg.data);
				}
				break;

				case BLE_MSG_TYPE_PRODUCT_TEST:
				{
					libble_msg_product_test(msg.head,msg.data);
				}
				break;

				default:
					ble_printf("msg type error :%x\n",msg.type);
				break;
			}
	}
	else{
		ble_printf("msg head error %x\n",msg.head);
	}
	memset(recv_buf,0,BLE_SEND_PKG_LEN);
	return 0;
}


