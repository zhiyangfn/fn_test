#ifndef _PROTOCOL_H
#define _PROTOCOL_H

#define MAX_PKG_DATA_LEN 15
#define PKG_HEAD_LEN 4
#define BLE_SEND_PKG_LEN 20


#define BLE_MSG_SET_HEAD 0x33
#define BLE_MSG_GET_HEAD 0xAA

#define BLE_MSG_SET		"set"
#define BLE_MSG_GET		"get"

#define BLE_MSG_TYPE_LED_STATUS			0x1
#define BLE_MSG_TYPE_LED_NUMS			0x2
#define BLE_MSG_TYPE_BLE_NAME			0x3
#define BLE_MSG_TYPE_LED_LIGHT			0x4
#define BLE_MSG_TYPE_WORK_MODE			0x5
#define BLE_MSG_TYPE_GET_VERSION		0x6
#define BLE_MSG_TYPE_GET_DEV_INFO		0x7
#define BLE_MSG_TYPE_UPDATE_TIME		0x9
#define BLE_MSG_TYPE_ONOFF_TIME			0xa
#define BLE_MSG_TYPE_ALARM_NUM			0xc

#define BLE_MSG_TYPE_START_UPGRADE 		0xf0
#define BLE_MSG_TYPE_SYSTEM_UPGRADE 	0xf1

#define BLE_MSG_TYPE_SYSTEM_OTA			0xfd			//0x33 0xfd 0x0  
#define BLE_MSG_TYPE_PRODUCT_TEST		0Xff

//dev sub type
#define BLE_MSG_DEV_SUB_TYPE_NAME			0X1
#define BLE_MSG_DEV_SUB_TYPE_SN				0X2
#define BLE_MSG_DEV_SUB_TYPE_HW_VERISON		0X3
#define BLE_MSG_DEV_SUB_TYPE_SW_VERSION		0X4
#define BLE_MSG_DEV_SUB_TYPE_DATE			0X5
#define BLE_MSG_DEV_SUB_TYPE_MAC			0X6
#define BLE_MSG_DEV_SUB_TYPE_TV_INCH		0X7


#define BLE_MSG_DEV_SUB_TYPE_NAME		0X1
#define BLE_MSG_DEV_SUB_TYPE_NAME		0X1


#define BLE_MSG_NAME_MAX_LEN 			16
#define MAX_VERSION_STRING_LEN 	7//X.XX.XX


typedef struct _ble_msg_t
{
	char head;//0x33 or 0xaa
	char type;
	char data[17];
	char check_code;
}ble_msg_t;

typedef struct _rec_data_buf
{
	unsigned int len;
	char data[30];
}rec_data_buf;


u8 send_request_pkg(char head,char type,char *data);
char libble_get_bcc_check_code(char *datas,char len);

u8 ble_protocol_work(rec_data_buf* dataBuf);
u8 send_response_pkg(char head,char type,char errcode);



#endif



