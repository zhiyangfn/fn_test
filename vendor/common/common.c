#include "../../proj/tl_common.h"
#include "../../proj_lib/light_ll/light_frame.h"
#include "../../proj_lib/ble_ll/blueLight.h"
#include "../../proj_lib/light_ll/light_ll.h"
#include "../../proj/mcu/watchdog_i.h"
#include "../../proj_lib/pm.h"
#include "common.h"
#include "../../proj/drivers/uart.h"
#include "../../proj_lib/ble_ll/att.h"
#include "../../proj_lib/rf_drv.h"

FLASH_ADDRESS_EXTERN;
/////////////// password encode sk initial  ///////////////////////////////////////////////////
u8	pair_config_pwd_encode_sk[17] = {MESH_PWD_ENCODE_SK};
u8  pair_config_pwd_encode_enable = 1;

u8 tx_packet_bridge_random_en = 0;
/////////////// adv par define ///////////////////////////////////////////////////
u16 adv_interval2listen_interval = 4;           // unit: default is 40ms, setting by 40000 from rf_link_slave_init (40000);
u16 online_status_interval2listen_interval = 8; // unit: default is 40ms, setting by 40000 from rf_link_slave_init (40000);

#if PASSIVE_EN
/////////////// for passive switch ///////////////////////////////////////////////
u8  separate_ADVpkt = 1;					//if 1 send one adv packet in each interrupt
u8  mesh_chn_amount = 2;				//amount of sys_chn_listen
/////////////// listen chanel define ///////////////////////////////////////////////////

u8 sys_chn_listen[4] = {2, 12, 2, 12};	//8, 30, 52, 74
#else
/////////////// for passive switch ///////////////////////////////////////////////
u8  separate_ADVpkt = 0;					//if 1 send one adv packet in each interrupt
u8  mesh_chn_amount = 4;				//amount of sys_chn_listen
/////////////// listen chanel define ///////////////////////////////////////////////////

u8 sys_chn_listen[4] = {2, 12, 23, 34};	//8, 30, 52, 74
#endif
/////////////// mesh node define ////////////////////////////////////////////////////
mesh_node_st_t mesh_node_st[MESH_NODE_MAX_NUM];
status_record_t slave_status_record[MESH_NODE_MAX_NUM];
u16 slave_status_record_size = sizeof(slave_status_record);

u32	mesh_node_mask[MESH_NODE_MAX_NUM>>5];
u16 mesh_node_max_num = MESH_NODE_MAX_NUM;
u8 mesh_node_st_val_len = MESH_NODE_ST_VAL_LEN;
u8 mesh_node_st_par_len = MESH_NODE_ST_PAR_LEN;
u8 mesh_node_st_len = sizeof(mesh_node_st_t);

void mesh_node_buf_init ()
{
	for (int i=0; i<mesh_node_max_num; i++)
	{
	    memset(&mesh_node_st[i], 0, sizeof(mesh_node_st_t));
	}
	device_status_update();
}

u8	SW_Low_Power = 0;

STATIC_ASSERT((MESH_NODE_MAX_NUM <= 256) && ((MESH_NODE_MAX_NUM % 32) == 0));
STATIC_ASSERT((MESH_NODE_ST_VAL_LEN >= 4) && ((MESH_NODE_ST_VAL_LEN <= 10)));

///////////////////////////////////////////////////////////////////////////////////
#define TYPE_FIX_STEP               1
#define TYPE_FIX_TIME               2

#define STEP_TYPE                   TYPE_FIX_TIME // TYPE_FIX_STEP // 

#if(STEP_TYPE == TYPE_FIX_STEP)
#define LIGHT_ADJUST_STEP           (2)   //unit: lum step 1--100
#define LIGHT_ADJUST_INTERVAL       (2)   // unit :10ms;     min:20ms
#else
#define LIGHT_ADJUST_TIME           (100)   //unit: 10ms
#define LIGHT_ADJUST_INTERVAL       (2)   // unit :10ms;     min:20ms
#endif

extern void light_adjust_RGB_hw(u8 val_R, u8 val_G, u8 val_B, u8 lum);
void light_onoff_normal(u8 on);

extern u16 rgb_lumen_map[101];
extern u8 light_off;
extern u8 led_lum;
extern u8 led_val[6];

typedef struct{
    u32 time;
    s16 lum_temp;
    s16 lum_dst;
    u8 step;
    u8 step_mod;
    u8 remainder;
    u8 adjusting_flag;
}light_step_t;

static light_step_t light_step = {};

enum{
    LUM_UP = 0,
    LUM_DOWN,
};

void get_next_lum(u8 direction){    
    u32 temp = light_step.remainder + light_step.step_mod;
    light_step.remainder = (u8)temp;
    
    if(LUM_UP == direction){
        light_step.lum_temp += light_step.step;
        if(temp >= 0x100){
            light_step.lum_temp += 1;
        }
        if(light_step.lum_temp >= light_step.lum_dst){
            light_step.lum_temp = light_step.lum_dst;
            light_step.remainder = 0;
        }
    }else{
        light_step.lum_temp -= light_step.step;
        if(temp >= 0x100){
            light_step.lum_temp -= 1;
        }
        if(light_step.lum_temp <= light_step.lum_dst){
            light_step.lum_temp = light_step.lum_dst;
            light_step.remainder = 0;
        }
    }
}

void get_step(u8 direction){
    light_step.remainder = 0;       // reset
    #if(STEP_TYPE == TYPE_FIX_STEP)
    light_step.step = LIGHT_ADJUST_STEP;
    light_step.step_mod = 0;
    #else   // fix time
    if(LUM_UP == direction){
        light_step.step = (light_step.lum_dst - light_step.lum_temp)/(LIGHT_ADJUST_TIME / LIGHT_ADJUST_INTERVAL);
        light_step.step_mod = (((light_step.lum_dst - light_step.lum_temp)%(LIGHT_ADJUST_TIME / LIGHT_ADJUST_INTERVAL))*256)/(LIGHT_ADJUST_TIME / LIGHT_ADJUST_INTERVAL);
    }else{
        light_step.step = (light_step.lum_temp - light_step.lum_dst)/(LIGHT_ADJUST_TIME / LIGHT_ADJUST_INTERVAL);
        light_step.step_mod = (((light_step.lum_temp - light_step.lum_dst)%(LIGHT_ADJUST_TIME / LIGHT_ADJUST_INTERVAL))*256)/(LIGHT_ADJUST_TIME / LIGHT_ADJUST_INTERVAL);
    }
    #endif
}

void light_step_correct_mod(u16 *pwm_val, u8 lum){
    #if(STEP_TYPE == TYPE_FIX_TIME)
    int temp_pwm = light_step.remainder;
    
    if(light_step.adjusting_flag && (light_step.lum_dst != light_step.lum_temp)
   && ((lum > 0) && (lum < ARRAY_SIZE(rgb_lumen_map) -1))
   && (light_step.remainder)){
        if(light_step.lum_dst > light_step.lum_temp){
            temp_pwm = *pwm_val + (temp_pwm * (rgb_lumen_map[lum+1] - rgb_lumen_map[lum])) / 256;

            if(temp_pwm > U16_MAX){
                temp_pwm = U16_MAX;
            }
        }else{
            temp_pwm = *pwm_val - (temp_pwm * (rgb_lumen_map[lum] - rgb_lumen_map[lum-1])) / 256;
            if(temp_pwm < 0){
                temp_pwm = 0;
            }
        }

        *pwm_val = temp_pwm;
    }
    #endif
}

void light_onoff_step_init()
{
    //light_step.adjusting_flag = 0;
    memset((u8 *)(&light_step), 0, sizeof(light_step));
}

void light_onoff_step(u8 on){
    if(light_step.adjusting_flag){
        //return ;
    }

    u8 set_flag= 1;
    
    if(on){
        if(light_off){
            if(0 == light_step.adjusting_flag){
                light_step.lum_temp = 0;
            }
            light_step.lum_dst = led_lum;
            get_step(LUM_UP);
    	}else{
    	    set_flag = 0;
    	    light_onoff_normal(1); // make sure on. unnecessary.
    	}
        light_off = 0;
	}else{
        if(light_off){
    	    set_flag = 0;
    	    light_onoff_normal(0); // make sure off. unnecessary.
    	}else{
            if(0 == light_step.adjusting_flag){
                light_step.lum_temp = led_lum;
            }
            light_step.lum_dst = 0;
            get_step(LUM_DOWN);
    	}
        light_off = 1;    
	}
	
    light_step.adjusting_flag = set_flag;
    light_step.time = 0;
}
extern void	pwm_set_lum (int id, u16 y, int pol);
extern u32 get_pwm_cmp(u8 val, u8 lum);
extern u8 CURRENT_MODE;//
extern u8 CT_VAL;//
void light_onoff_step_timer(){
    if(light_step.adjusting_flag){
        if(0 == light_step.time){
            if(light_step.lum_dst != light_step.lum_temp){
                if(light_step.lum_temp < light_step.lum_dst){
                    get_next_lum(LUM_UP);
                }else{
                    get_next_lum(LUM_DOWN);
                }
               // light_adjust_RGB_hw(led_val[0], led_val[1], led_val[2], light_step.lum_temp);
                if(CURRENT_MODE==1)
				{
					light_adjust_RGB_hw(led_val[0], led_val[1], led_val[2], light_step.lum_temp);
				}
				else
					if(CURRENT_MODE==2)
					{
					 pwm_set_lum (PWMID_C1, get_pwm_cmp((100-CT_VAL)*2.5,light_step.lum_temp), 0);//Àä¹â
					 pwm_set_lum (PWMID_W, get_pwm_cmp((CT_VAL)*2.5,light_step.lum_temp), 0);     //Å¯¹â

					}


            }else{
                light_step.adjusting_flag = 0;
                memset((u8 *)(&light_step), 0, sizeof(light_step));
            }
        }
        
        light_step.time++;
        if(light_step.time >= LIGHT_ADJUST_INTERVAL){
            light_step.time = 0;
        }
    }
}

u8 is_lum_invalid(u8 lum){
    #define LED_LUM_MIN         5
    if(lum < LED_LUM_MIN){
        return LED_LUM_MIN;
    }else{
        return 0;
    }
}

extern u8 rf_slave_ota_busy;
void pa_init(u8 tx_pin_level, u8 rx_pin_level)
{
#if(PA_ENABLE)
    rf_set_power_level_index (RF_POWER_0dBm);
    gpio_set_func(PA_TXEN_PIN, AS_GPIO);
    gpio_set_input_en(PA_TXEN_PIN, 0);
    gpio_set_output_en(PA_TXEN_PIN, 1);
    gpio_write(PA_TXEN_PIN, tx_pin_level);
    
    gpio_set_func(PA_RXEN_PIN, AS_GPIO);
    gpio_set_input_en(PA_RXEN_PIN, 0);
    gpio_set_output_en(PA_RXEN_PIN, 1);
    gpio_write(PA_RXEN_PIN, tx_pin_level);
#endif    
}
void pa_txrx(u8 val)
{
#if(PA_ENABLE)
    if(val == PA_OFF/* || rf_slave_ota_busy*/){
        gpio_write(PA_TXEN_PIN, 0);
        gpio_write(PA_RXEN_PIN, 0);
    }else if(val == PA_TX){
        gpio_write(PA_RXEN_PIN, 0);
        gpio_write(PA_TXEN_PIN, 1);
    }else if(val == PA_RX){
        gpio_write(PA_TXEN_PIN, 0);
        gpio_write(PA_RXEN_PIN, 1);
    }
#endif
}

u8 iBeaconInterval = 0;
u8 beacon_with_mesh_adv = 0;// 0 means only send beacon adv pkt;  1 means send both of beacon pkt and mesh adv pkt
#if(IBEACON_ENABLE)
//apple ibeacon
ibeacon_adv_t ibeacon_customer_data = {
	0x02, 0x01, 0x06,		// not connect
	0x1A, 
	0xFF, 
	0x004C, 
	0x1502,
	{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F},
	0x3327,
	0x1FBA,
	(7 - 57),			//7-8dbm not 0dbm, Measured power one meter distance, link loss -41dbm
};

u8 eddystone_uid[31] = {
                    0x02, 0x01, 0x06,       // not connect
                    0x03, 0x03, 0xAA, 0xFE, // uuid
                    0x17, 0x16, 0xAA, 0xFE, // UID type's len is 0x17
                        0x00,               // UID type
                        0x08,               // tx power
                        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, // NID
                        0x01, 0x02, 0x03, 0x04, 0x05, 0x06,                         // BID
                        0x00, 0x00          // RFU
                    };
u8 eddystone_url[31] = {
                    0x02, 0x01, 0x06,       // not connect
                    0x03, 0x03, 0xAA, 0xFE, // uuid
                    0x12, 0x16, 0xAA, 0xFE, // URL type's len is variable
                        0x10,               // URL type
                        0x08,               // tx power
                        0x00,               // URL Scheme 0x00-http://www.  0x01-https://www.  0x02-http://  0x03-https://
                        0x74, 0x65, 0x6c, 0x69, 0x6e, 0x6b, 0x2d, 0x73, 0x65, 0x6d, 0x69,// telink-semi
                        0x07,               // 0x07-.com  0x08-.org 0x09-.edu  0x0a-.net............
                    };

u8 eddystone_tlm[31] = {
                    0x02, 0x01, 0x06,       // not connect
                    0x03, 0x03, 0xAA, 0xFE, // uuid
                    0x11, 0x16, 0xAA, 0xFE, // TLM type's len is 0x11
                        0x20,               // TLM type
                        0x00,               // TLM version
                        0x00, 0x00,         // Battery voltage 1mV/bit
                        0x00, 0x80,         // Temperature
                        0x00, 0x00, 0x00, 0x00, // ADV_CNT
                        0x00, 0x00, 0x00, 0x00, // SEC_CNT unit:0.1s
                    };

u8 iBeaconData[31] = {0};
u8 beacon_len = 0;
extern rf_packet_adv_ind_module_t pkt_ibeacon;
extern u8* slave_p_mac;
void set_ibeacon_data(u8 *val, int n){
    pkt_ibeacon.dma_len = n + 8;
    pkt_ibeacon.type = FLG_BLE_ADV_NONCONN_IND;//Set ADV type to non-connectable FLG_BLE_ADV_NONCONN_IND
    pkt_ibeacon.rf_len = n + 6;
	memcpy (pkt_ibeacon.advA, slave_p_mac, 6);
    memcpy (pkt_ibeacon.data, val, n);
}
#endif

u8 flash_user_data[16] = {0};
u8 user_data_idx;					//always pointing to the next block address to be written
#define max_data_block 256			//256*16= 4096
u8 get_user_data_idx(void)
{
	int i;
	u8 data[16];
	u8 data1[16] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
					0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
	user_data_idx = 0;
	for (i=0; i<max_data_block; i++)
	{
		flash_read_page((flash_adr_user_data + user_data_idx*16),16,(u8 *)(&data));
		if(!memcmp(data,data1,16))
			break;
		user_data_idx++;
	}
	if(max_data_block == i)
	{
		memcpy(flash_user_data,data,16);
		flash_erase_sector(flash_adr_user_data);
		user_data_idx = 0;
	}
	if(0 == user_data_idx)
	{
		flash_write_page(flash_adr_user_data,16,(u8 *)(&flash_user_data));	//reset
		user_data_idx ++;
	}
	return user_data_idx;
}

void get_flash_user_data(void)		//get data from flash
{
	get_user_data_idx();
	flash_read_page((flash_adr_user_data + (user_data_idx-1)*16),16,(u8 *)(&flash_user_data));
}



void store_user_data_2flash(void)
{
	get_user_data_idx();
	flash_write_page((flash_adr_user_data + user_data_idx*16),16,(u8 *)(&flash_user_data));
	user_data_idx ++;
}

int compare_user_data(void)
{
	u8 temp[16];
	flash_read_page((flash_adr_user_data + (user_data_idx-1)*16),16,(u8 *)(&temp));
	return memcmp(temp,flash_user_data,16);
}


void check_store_user_data(void)	//if flash_user_data changed store it
{
	get_user_data_idx();
	if(!compare_user_data())
		return;							//data not changed
	store_user_data_2flash();
}

#if ((__PROJECT_LIGHT_SWITCH__)         \
    ||(__PROJECT_LIGHT_8266__)          \
    ||(__PROJECT_8266_MESH_CONFIG__)    \
    ||(__PROJECT_LIGHT_8267__)          \
    ||(__PROJECT_LIGHT_8269__)          \
    || (__PROJECT_LIGHT_8267_UART__)	\
    || (__PROJECT_LIGHT_GATEWAY__)	\
    ||(__PROJECT_LIGHT_NO_MESH__)          \
    ||(__PROJECT_MASTER_LIGHT_8266__)          \
    ||(__PROJECT_MASTER_LIGHT_8267__)          \
    ||(__PROJECT_MOTIONSENSOR_8267__)   \
    ||(__PROJECT_LIGHT_SWITCH__)        \
    ||(__PROJECT_BLE_MASTER__)          \
    ||(__PROJECT_MONITOR_8266__))

// for ota
extern fp_rf_led_ota_ok 			p_vendor_rf_led_ota_ok;
extern fp_rf_led_ota_error			p_vendor_rf_led_ota_error;
#if (__PROJECT_LIGHT_SWITCH__ || __PROJECT_MOTIONSENSOR_8267__ || __PROJECT_LIGHT_NO_MESH__)
#define 	OTA_LED				GPIO_LED
#else
#define 	OTA_LED				PWM_R
#endif

void rf_led_ota_ok(void){
	if(p_vendor_rf_led_ota_ok){
		p_vendor_rf_led_ota_ok();
		return;
	}
	gpio_set_func(OTA_LED, AS_GPIO);
	gpio_set_output_en(OTA_LED, 1);
	static u8 led_onoff = 1;
	foreach(i, 6){
		gpio_write(OTA_LED, led_onoff);
		led_onoff = !led_onoff;
#if(MODULE_WATCHDOG_ENABLE)
        wd_clear();
#endif
		sleep_us(1000*1000);
	}
}
void rf_led_ota_error(void){
	if(p_vendor_rf_led_ota_error){
		p_vendor_rf_led_ota_error();
		return;
	}
	gpio_set_func(OTA_LED, AS_GPIO);
	gpio_set_output_en(OTA_LED, 1);
	static u8 led_onoff = 1;
	foreach(i, 60){
		gpio_write(OTA_LED, led_onoff);
		led_onoff = !led_onoff;
#if(MODULE_WATCHDOG_ENABLE)
        wd_clear();
#endif
		sleep_us(100*1000);
	}
}

void mesh_ota_led_cb(u32 type)
{
    if(MESH_OTA_LED_OK == type){
        rf_led_ota_ok();
    }else if(MESH_OTA_LED_ERROR == type){
        rf_led_ota_error();
    }else if(MESH_OTA_LED_STOP == type){
        rf_led_ota_error();
    }
}
#endif

///////////////  light_slave_tx_command call back /////////////////////////////////
void rf_link_data_callback (u8 *);
void light_slave_tx_command_callback (u8 *p){
    rf_link_data_callback(p);
}

///////////////  set BLE interval and timeout parameter /////////////////////////////////
/************
 *
 * int setup_ble_parameter_start(u16 delay, u16 interval_min, u16 interval_max, u16 timeout);
 *
 * delay   :  unit: one ble interval 
 * interval_min,interval_max:  if all 0,will keep the system parameter for android but not ios.   unit: 1.25ms; must longer than 20ms.
 * timeout:  if 0,will keep the system parameter.   unit: 10ms; must longer than 3second for steady connect.
 *
 * return 0 means setup parameters is valid.
 * return -1 means parameter of interval is invalid.
 * return -2 means parameter of timeout is invalid.
 *
 *
 * void rf_link_slave_connect_callback()
 * system will call this function when receive command of BLE connect request.
 */
void update_ble_parameter_cb(){
    #if 1
    #if 0 // for android to set a fixed interval
    setup_ble_parameter_start(1, 32, 32, 200);  // interval 32: means 40ms;   timeout 200: means 2000ms
    #else   // for ios
    /*IOS Note: 
          20 ms <= interval_min
          interval_min + 20 ms <= interval_max <= 2second
          timeout <= 6second
       */
    setup_ble_parameter_start(1, 16, 16+16, 200);  // interval 32: means 40ms;   timeout 200: means 2000ms
    #endif
    #endif
}

void rf_link_slave_connect_callback(){
}

void cb_ble_slave_disconnect(){
}

void cb_pair_failed(){
}

// system will call p_cb_ble_slave_disconnect() when BLE disconnect.
cb_func_void_t	p_cb_ble_slave_disconnect = 0;  // cb_ble_slave_disconnect  //

// system will call p_cb_pair_failed() when login failed or set mesh name/password/ltk failed.
cb_func_void_t	p_cb_pair_failed = 0;   // cb_pair_failed   //

/************
 * u8 get_setup_ble_parameter_result()
 *
 * return 0 means setup parameters fail.
 * return 1 means setup parameters success.
 */
 
u8 get_setup_ble_parameter_result(){
    extern u8 update_ble_par_success_flag;
    return update_ble_par_success_flag;
}

////////////////// gate way /////////////////////////////////////
int rf_link_slave_data (rf_packet_ll_data_t *p, u32 t);

extern u8 	slave_link_connected;
extern u8   pair_login_ok;
extern u8   not_need_login;

typedef struct{
    u8 sno[3];
    u8 src[2];
    u8 dst[2];
    u8 op;
    u16 vendor_id;
    u8 par[10];
}app_cmd_value_t;

typedef struct{
	u32 dma_len;
	u8	type;
	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  opcode;
	u8 handle;
	u8 handle1;
	app_cmd_value_t app_cmd_v;
	u8 rsv[10];
}rf_packet_ll_app_t;

#if GATEWAY_EN
u32 gateway_cmd_sno;

u8          mode_master = 0;

#if (HCI_ACCESS == HCI_USE_UART)
#define GATEWAY_REPORT_INTERVAL_MS      (10)
#else   // usb
#define GATEWAY_REPORT_INTERVAL_MS      (20)    // min: 18ms
#endif

void rf_link_slave_data_app(rf_packet_ll_app_t *pkt_app)
{
    u8 r = irq_disable();
    if(gateway_en && !gateway_security){
        not_need_login = pair_login_ok = 1;
    }
    rf_link_slave_data((rf_packet_ll_data_t *)(pkt_app), 0);
    irq_restore(r);
}
/*///////////////////////////////////////////////////
int	ble_gateway_ll_data (u8 *p, int n);

p[0] : rf_packet_att_cmd_t.type
n : length of input buffer, max is 29
///////////////////////////////////////////////////*/
int	ble_gateway_ll_data (u8 *p, int n)
{
	if (1) {
	    rf_packet_ll_app_t  pkt_app_data = {};
	    memset (&pkt_app_data, 0, sizeof(pkt_app_data));
		memcpy (&pkt_app_data.type, p, n);
		pkt_app_data.dma_len = n;			// recalculate to make sure length is ok
		pkt_app_data.rf_len = n - 2;        // recalculate to make sure length is ok
		if (pkt_app_data.type < 3)
		{
			pkt_app_data.l2capLen = n - 6;  // recalculate to make sure length is ok
		}

        rf_link_slave_data_app(&pkt_app_data);
		return 1;
	}
	return 0;
}

#if 1    // for test
int gatway_tx_command(u8 cmd, u16 dst_adr, u8 *par, u8 par_len, u8 op){ // should call in fp_gateway_rx_proc();
    if(par_len > (is_cmd_long_par(cmd) ? 15 : 10)){
        return -1;
    }

    // packet 
	rf_packet_ll_app_t  pkt_app_data = {};
    memset(&pkt_app_data, 0, sizeof(pkt_app_data));
    pkt_app_data.type = 0x02;
    pkt_app_data.rf_len = 17 + par_len;
    pkt_app_data.dma_len = pkt_app_data.rf_len + 2;
    pkt_app_data.l2capLen = pkt_app_data.rf_len - 4;
    pkt_app_data.chanId = 0x04;
    pkt_app_data.opcode = op;
    pkt_app_data.handle= 0x15;
    pkt_app_data.handle1 = 0x00;
    
    gateway_cmd_sno++;
    memcpy(pkt_app_data.app_cmd_v.sno, &gateway_cmd_sno, 3);
    //memcpy(pkt_app_data.app_cmd_v.src, &device_address, 2);
    memcpy(pkt_app_data.app_cmd_v.dst, &dst_adr, 2);
    pkt_app_data.app_cmd_v.op = (cmd & 0x3F) | 0xC0;
    pkt_app_data.app_cmd_v.vendor_id = VENDOR_ID;
    memcpy(pkt_app_data.app_cmd_v.par, par, par_len);
    
    // send command
    rf_link_slave_data_app(&pkt_app_data);
    
    return 0;
}

// update firmware for itself
int gatway_local_OTA(u8 *par, u8 par_len){ // should call in fp_gateway_rx_proc();
    if(par_len > 20){
        return -1;
    }

    // packet 
	rf_packet_ll_app_t  pkt_app_data = {};
    memset(&pkt_app_data, 0, sizeof(pkt_app_data));
    pkt_app_data.type = 0x02;
    pkt_app_data.rf_len = 7 + par_len;
    pkt_app_data.dma_len = pkt_app_data.rf_len + 2;
    pkt_app_data.l2capLen = pkt_app_data.rf_len - 4;
    pkt_app_data.chanId = 0x04;
    pkt_app_data.opcode = ATT_OP_WRITE_CMD;
    pkt_app_data.handle= 0x18;
    pkt_app_data.handle1 = 0x00;
    
    memcpy(&pkt_app_data.app_cmd_v, par, par_len);
    
    // send command
    rf_link_slave_data_app(&pkt_app_data);
    
    return 0;
}

void gatway_tx_command_test(){
    u8 par[10] = {0};   // must init to zero;  max size of par is 10
    u16 dst_adr = 0xFFFF;
    u8 cmd = LGT_CMD_LIGHT_ONOFF;
    
    static u32 tx_cmd_cnt;
    tx_cmd_cnt++;
    if(tx_cmd_cnt & 1){
        par[0] = LIGHT_ON_PARAM;
    }else{
        par[0] = LIGHT_OFF_PARAM;
    }
    
    gatway_tx_command(cmd, dst_adr, par, sizeof(par), ATT_OP_WRITE_REQ);
}

///////////////  sample function of MASTER UART handle received data  /////////////////////////////////
/************
 * void    gateway_uart_host_handle(void *pkt_uart);
 *
 * pkt_uart: pointer to UART data
 *
 * MASTER UART will call this function when receive data from gateway.
 */
void    gateway_uart_host_handle(void *pkt_uart)
{
    rf_packet_ll_app_t *pkt_rsp = CONTAINER_OF(pkt_uart, rf_packet_ll_app_t, l2capLen);
    if(0x04 == pkt_rsp->chanId){   // LLID : 2
        if(ATT_OP_WRITE_RSP == pkt_rsp->opcode){
            //
        }else if(ATT_OP_READ_RSP == pkt_rsp->opcode){
            //
        }else if(ATT_OP_HANDLE_VALUE_NOTI == pkt_rsp->opcode){  // notify pkt
            //
        }
    }
}
#endif

void gateway_hci_busy_clr()
{
#if (HCI_ACCESS == HCI_USE_UART)
    uart_tx_busy_clear();
#else   // usb
    reg_usb_ep_ctrl(8) = FLD_USB_EP_BUSY;
#endif
}

int gateway_hci_busy()
{
    int busy = 0;
#if (HCI_ACCESS == HCI_USE_UART)
    busy = uart_tx_busy_check();
#else   // usb
    busy = (reg_usb_ep_ctrl(8) & FLD_USB_EP_BUSY);
#endif
    static u8 hci_busy_start;
    static u32 hci_busy_tick;
    if(busy){
        if(0 == hci_busy_start){
            hci_busy_start = 1;
            hci_busy_tick = clock_time();
        }
        if(clock_time_exceed(hci_busy_tick, 1000*1000)){
            hci_busy_start = 0;
            gateway_hci_busy_clr();
            return 0;
        }
    }else{
        hci_busy_start = 0;
    }
    
    return busy;
}

// Note: check differrence IO of button for differrence PCB
void	proc_ui_gateway ()
{
    if(gateway_en && rf_slave_ota_busy){
        u8 r = irq_disable();   // must
        fp_gateway_rx_proc();   // handle ota data as soon as possible
        irq_restore(r);
    }
    
    if(gateway_en){             // report notify as soon as possible
        static u32 gateway_report_tick;
        if((0 == gateway_hci_busy()) 
        && clock_time_exceed(gateway_report_tick, GATEWAY_REPORT_INTERVAL_MS*1000)
        ){
            if(gateway_report_poll()){
                gateway_report_tick = clock_time();
            }
        }
    }

	static u32 tick;
	if (!clock_time_exceed (tick, 40000))
	{
		return;
	}
	tick = clock_time();
	
    if(!gateway_en){
        // when in gateway mode, proccess in irq_st_ble_rx(), so it just receive BLE command in irq_st_ble_rx().
        fp_gateway_rx_proc();
    }

    #if (!(__PROJECT_MASTER_LIGHT_8266__ || __PROJECT_MASTER_LIGHT_8267__))	// because button function use for switch master/slave mode in master light
	static u8 st = 0;
	u8 s = !gpio_read (SWITCH_MODE_BUTTON1) || !gpio_read(SWITCH_MODE_BUTTON2);
	if ((!st) & s)
	{
	    #if 0
	    gatway_tx_command_test();      // for test
	    #else
	    if(0 == gateway_mode_onoff(!gateway_en)){
            rf_link_light_event_callback(LGT_CMD_DEL_PAIR);
	    }
	    #endif
	}
	st = s;
	#endif
}

const rf_packet_ll_init_t	pkt_gateway_init = {
		sizeof (rf_packet_ll_init_t) - 4,		// dma_len
		FLG_BLE_LIGHT_CONNECT_REQ,				// type
		sizeof (rf_packet_ll_init_t) - 6,		// rf_len
		{0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5},	// scanA
		{0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5},	// advA
		{0xd6, 0xbe, 0x89, 0x8e},				// access code
		{0x8d, 0x26, 0xd8},						// crcinit[3]
		0x02,									// wsize
		0x0005,									// woffset
		0x0020,									// interval: 32 * 1.25 ms = 40 ms
		0x0000,									// latency
		0x0032,									// timeout: 50*10 ms = 500ms
		{0xff, 0xff, 0xff, 0xff, 0x1f},			// chm[5]
		0xac,									// hop: initial channel - 12
};

#define     EP_BO           5

void	ble_master_data_callback (u8 *p)
{
	static	u32 bdbg_data;
	bdbg_data++;
	/*static */u8 buf[48];
	memset4(buf, 0, sizeof(buf));

	rf_packet_ll_app_t *pkt_rsp;
	if(mode_master){
	    pkt_rsp = (rf_packet_ll_app_t *)(p+4);
	}else{
	    pkt_rsp = (rf_packet_ll_app_t *)p;
	}

	u8 rf_len = pkt_rsp->rf_len;
	buf[0] = rf_len;                      // buf[0-3]: length of UART
	memcpy(buf+4, &pkt_rsp->l2capLen, rf_len);
	
#if (HCI_ACCESS == HCI_USE_UART)
	uart_Send((u8*)(buf));          // UART MASTER would call gateway_uart_host_handle() to handle this data
#else
	reg_usb_ep8_dat = rf_len + 8;   // length of USB data
	reg_usb_ep8_dat = 0;
	u8 *pdata = buf+4;              // same witch (&pkt_rsp->l2capLen)
	for (int i=0; i < rf_len + 5; i++) // 5 = DC offset (2 BYTE) + CRC (3BYTE)
	{
		reg_usb_ep8_dat = pdata[i];
	}
	if(mode_master){
	    reg_usb_ep8_dat = p[0];     //rssi
	}else{
	    reg_usb_ep8_dat = 0;        //rssi
	}
	reg_usb_ep8_ctrl = BIT(7);
#endif
}

/*
ble_event_callback() is only needed if it is  security mode between master and gateway.
*/
void ble_event_callback (u8 status, u8 *p, u8 rssi)
{
	static u32 bdbg_event;
	bdbg_event++;

#if (__PROJECT_MASTER_LIGHT_8266__ || __PROJECT_MASTER_LIGHT_8267__)
	if(FLG_SYS_LINK_LOST == status){
		extern int host_ota_start; 
		host_ota_start = 0;			// must, if not, it can not reconnect, when host_ota_start not zero. 
	}
#endif

#if (__PROJECT_LIGHT_8267_UART__ || (HCI_ACCESS == HCI_USE_UART))
	#if 0	// for sample
	/*static */u8 buf[48];
	memset4(buf, 0, sizeof(buf));
	rf_packet_adv_t *p_adv = CONTAINER_OF(p,rf_packet_adv_t,rf_len);
	buf[0] = p_adv->rf_len + 2; 					 // buf[0-3]: length of UART
	memcpy(buf+4, &p_adv->type, buf[0]);
	uart_Send((u8*)(buf));
	#endif
#else
	//////////////////////////////////////////////
	int n = 0;
	if (p)
	{
		n = *p++;
	}
	reg_usb_ep8_dat = n + 8;
	reg_usb_ep8_dat = status;
	for (int i=0; i<n+5; i++)
	{
		reg_usb_ep8_dat = *p++;
	}
	reg_usb_ep8_dat = rssi;
	reg_usb_ep8_ctrl = BIT(7);
#endif
	///////////////////////////////////////////////
}


//////////////////////////////////////////////////////////
//	USB interfuace BI/BO
//////////////////////////////////////////////////////////
u8	buff_command[64];
int host_write_gateway (int n)
{
	static u32 no_cmd;
	no_cmd++;

	u8 cmd = buff_command[0];

	if (cmd == 0 && n > 0)		// data
	{
	    if(gateway_en){
		    ble_gateway_ll_data (buff_command + 1, n - 1);
		}
	}
	else if (cmd == 2)	// start master
	{
	    extern rf_packet_adv_ind_module_t	pkt_adv;
	    // report adv packet to PC, because master need MAC of gate way in security mode
	    ble_event_callback (FLG_SYS_DEVICE_FOUND, (u8 *)(&(pkt_adv.rf_len)), 0);
	}
	else if (cmd == 3)	// stop master
	{
	}
	return 0;
}

void host_init ()
{
	/////////// ID initialization for host control software //////////
	REG_ADDR8(0x74) = 0x53;
	REG_ADDR16(0x7e) = 0x82bd;
	REG_ADDR8(0x74) = 0x00;

	/////////// enable USB device /////////////////////////////////////
	#if (MCU_CORE_TYPE == MCU_CORE_8267 || MCU_CORE_TYPE == MCU_CORE_8269)    // 
	gpio_setup_up_down_resistor(GPIO_DP, PM_PIN_PULLUP_10K);
	usb_dp_pullup_en (0);    // fix 8267 A0 error:1.5K pull up
    #else
	usb_dp_pullup_en (1);
	#endif
	reg_usb_ep_ctrl(EP_BO) = BIT(0);
}

void proc_host ()
{
	//////////// host interface change  //////////////////////////////////
#if (HCI_ACCESS == HCI_USE_UART)
	extern unsigned char uart_rx_true;
	u8 rc_cmd_flag = uart_rx_true;
#else
	u8 rc_cmd_flag = (reg_usb_irq & BIT(EP_BO));
#endif

	if(rc_cmd_flag){
		memset4(buff_command, 0, sizeof(buff_command));
		#if (HCI_ACCESS == HCI_USE_UART)
		u32 gateway_proc(void);
		int n = gateway_proc();
		#else
		int n = reg_usb_ep_ptr (EP_BO);
		reg_usb_ep_ptr(EP_BO) = 0;
		for (int i=0; i<n; i++)
		{
			buff_command[i] = reg_usb_ep_dat(EP_BO);
		}
		#endif
		
	    if(!mode_master){
	        host_write_gateway (n);
	    }
		#if MODE_MASTER_SLAVE
	    else{
		    host_write_master (n);
		}
		#endif

		#if (HCI_ACCESS == HCI_USE_USB)
		reg_usb_irq = BIT(EP_BO);
		reg_usb_ep_ctrl(EP_BO) = BIT(0);
		#endif
	}
	#if (HCI_ACCESS == HCI_USE_USB)
	else{
		static u32	tick_bulk_out;
		if (reg_usb_ep_ctrl(EP_BO) & FLD_USB_EP_BUSY)
		{
			tick_bulk_out = clock_time ();
		}
		else if (clock_time_exceed (tick_bulk_out, 1000000))
		{
			reg_usb_ep_ctrl(EP_BO) = BIT(0);
		}
	}
	#endif
}

void gateway_init(){
    gateway_cmd_sno = ((clock_time() + device_address) & 0xff) << 16;
    fp_gateway_tx_proc = &ble_master_data_callback;
    fp_gateway_rx_proc = &proc_host;
	host_init();
}

void gateway_set_login_flag(){
    if(gateway_en && (!gateway_security)){
        not_need_login = pair_login_ok = 1;
    }else{
        not_need_login = pair_login_ok = 0;
    }
}

int gateway_mode_onoff(u8 on){
    u8 r = irq_disable();
    int ret = 0;
    //mesh_send_online_status_flag = 1;
    if(on){
        if(0 == gateway_en){
            if(0 == slave_link_connected){  // if connected by other phone
                gateway_en = 1;
                int rf_link_slave_connect (rf_packet_ll_init_t *p, u32 t);
                rf_link_slave_connect((rf_packet_ll_init_t *)&pkt_gateway_init, clock_time());
                slave_link_connected = 1;
                gateway_set_login_flag();
                ble_event_callback (FLG_SYS_LINK_CONNECTED, 0, 0);
            }else{
                extern u8 ble_conn_terminate;
                ble_conn_terminate = 1;
                ret = -1;  // wait for timeout
            }
        }
    }else{
        if(gateway_en){
            gateway_set_login_flag();
        }
        gateway_en = 0;     // will timeout and into adv mode
        ble_event_callback (FLG_SYS_LINK_LOST, 0, 0);
    }

    irq_restore(r);
    return ret;
}
#endif

void mesh_ota_set_start_par_user(mesh_ota_pkt_start_command_t *p)
{
    #if 0
    // redefine parameter by user if needed.
    // max size of start command par is 8
    memset(p, 0, 8);
    #endif
}

int mesh_ota_slave_need_ota(u8 *params)
{
    int ret = 1;
    mesh_ota_pkt_start_command_t *p =  (mesh_ota_pkt_start_command_t *)(params+2);
    if(LIGHT_MODE == p->dev_mode){
        void get_fw_version(u8 *ver);
        u8 ver_myself[4];
        get_fw_version(ver_myself);
        if(p->version[1] < ver_myself[1]){
            ret = 0;
        }else if(p->version[1] == ver_myself[1]){
            if(p->version[3] <= ver_myself[3]){
                ret = 0;
            }
        }
    }else{
        ret = 0;
    }
    
    return ret;
}

/* 
for start mesh ota, user should call 
mesh_ota_master_start_firmware_from_20000() or mesh_ota_master_start_firmware_from_own();

Note: only gate way or some node in BLE connected can start mesh ota now.
*/
void mesh_ota_master_start_firmware_from_20000(u8 dev_mode)
{
    mesh_ota_master_start((u8 *)flash_adr_ota_master, *(u32 *)(flash_adr_ota_master+0x18), dev_mode);
}

void mesh_ota_master_start_firmware_from_own()
{
#if (MCU_CORE_TYPE == MCU_CORE_8267 || MCU_CORE_TYPE == MCU_CORE_8269)
    u32 adr_fw = ota_program_offset ? 0 : flash_adr_light_new_fw;  // 8267/8269 should use "ota_program_offset"
#else
    u32 adr_fw = 0;
#endif

    mesh_ota_master_start((u8 *)adr_fw, *(u32 *)(adr_fw+0x18), LIGHT_MODE);
}

////////////////// check stack /////////////////////////////////////
#if(STACK_CHECK_ENABLE)
extern u32 _start_bss_, _end_bss_;
#define STACK_CHECK_FLAG        (0x1324adbc)    // use a random data

void stack_check_init(){
    _start_bss_ = _end_bss_ = STACK_CHECK_FLAG;
}

void stack_check(){
	static u32 stack_check_tick;
	if (!clock_time_exceed (stack_check_tick, 1000*1000))
	{
		return;
	}
	stack_check_tick = clock_time();
	
    if((_start_bss_ != STACK_CHECK_FLAG)    // irq_stack
    || (_end_bss_ != STACK_CHECK_FLAG)){    // system_stack
        irq_disable();
        while(1){
            #if(MODULE_WATCHDOG_ENABLE)
    		wd_clear();
            #endif
            static u8 stack_overflow;
            stack_overflow++;
        }
    }
}
#endif

////////////////// ALARM and SCENE /////////////////////////////////////
#include "rtc.h"
#include "scene.h"

int is_bridge_task_busy(){
    return ((ALARM_EN && (is_alarm_poll_notify_busy()))
          || (SCENE_EN && (is_scene_poll_notify_busy())));
}

int is_tx_cmd_busy(){   // it must be used, if want enough bridges(default is BRIDGE_MAX_CNT 8).
    return (0 != mesh_user_cmd_idx);
}

////////////////// OTA  /////////////////////////////////////
extern void ota_boot_check();
extern int rf_ota_save_data(u8 * data);
extern void rf_ota_set_flag();

u32  rf_slave_ota_finished_time = 0;
extern u8  rf_slave_ota_finished_flag;
extern u8  rf_slave_ota_terminate_flag;
extern u32 cur_ota_flash_addr;
extern u8 flash_protect_en;
extern u32 tick_per_us;
extern u16 rf_slave_ota_timeout_def_s;
extern u16  rf_slave_ota_timeout_s;

const u8 pkt_terminate[8] = {0x04, 0x00, 0x00, 0x00, 0x03, 0x02, 0x02, 0x13};

int	rf_link_slave_data_ota(void *ph)
{
	extern u16 ota_pkt_cnt;
	static u16 dbg_ota;
	rf_packet_att_data_t *p = (rf_packet_att_data_t *)ph;
	int nDataLen = p->l2cap - 7;
	static u16 dbg_ota_data0, dbg_ota_dat_crc_ok, dbg_ota_dat_crc_err, dbg_ota_cpy0_cnt, dbg_ota_cpy_cnt, dbg_ota_lost_cnt = 0;
	static u16 ota_rcv_last_idx = -1;
	u8 reset_flag=0;

    if(!pair_login_ok || rf_slave_ota_finished_flag){
        return 1;
    }
	rf_slave_ota_busy = 1;

	if(crc16(p->dat, 2+nDataLen) == (p->dat[nDataLen+2] | p->dat[nDataLen+3]<<8)){
		rf_slave_ota_timeout_s = rf_slave_ota_timeout_def_s;	// refresh timeout
		
		u16 cur_idx = (*(p->dat) | ((*(p->dat+1)) << 8));
		if(nDataLen == 0){
		    u32 ota_firmware_len = 0;
		    u16 ota_pkt_total = 0;
		    flash_read_page(flash_adr_ota_offset + 0x18, 4,(u8 *)&ota_firmware_len);
		    ota_pkt_total = (ota_firmware_len + 15)/16;
			if((cur_idx == ota_rcv_last_idx+1) && (cur_idx == ota_pkt_total)){
				// ota ok, save, reboot
				++dbg_ota_data0;
				reset_flag = OTA_STATE_OK;
			}else{
				// ota err
				cur_ota_flash_addr = ota_pkt_cnt = ota_rcv_last_idx = 0;
				reset_flag = OTA_STATE_ERROR;
			}
		}else{
			++dbg_ota_dat_crc_ok;
			if(cur_idx == 0){
				// start ota
				if(cur_ota_flash_addr){
				    // 0x10000 should be 0x00
					cur_ota_flash_addr = ota_pkt_cnt = ota_rcv_last_idx = 0;
                    reset_flag = OTA_STATE_ERROR;
				}else{
                    #if(__TL_LIB_8266__ || (MCU_CORE_TYPE && MCU_CORE_TYPE == MCU_CORE_8266))
                    ota_boot_check();
                    #endif
                    
                    if(flash_protect_en){
				        flash_unprotect_OTA_start();
				    }
    				reset_flag = rf_ota_save_data(p->dat+2);
    				++dbg_ota_cpy0_cnt;
				}
			}else if(cur_idx == ota_rcv_last_idx + 1){
				reset_flag = rf_ota_save_data(p->dat+2);
				++dbg_ota_cpy_cnt;
			}else if(cur_idx == ota_rcv_last_idx){
				// for feibi
			}else{
				// error, ota failed
				++dbg_ota_lost_cnt;
				cur_ota_flash_addr = ota_pkt_cnt = ota_rcv_last_idx = 0;
				reset_flag = OTA_STATE_ERROR;
			}

			ota_rcv_last_idx = cur_idx;
		}
	}else{
		#if 0//  crc error before, shoule #if 1 for feibi
		if(nDataLen == 0){
			// ota  ok, save , reboot
			++dbg_ota_dat_crc_err1;
			flash_erase_sector (FLASH_ADR_OTA_READY_FLAG);
			u32 flag = 0xa5;
			flash_write_page (FLASH_ADR_OTA_READY_FLAG, 4, &flag);
			rf_led_ota_ok();
			reset_flag = OTA_STATE_OK;
		}else{
		#endif
			// error, ota failed
			++dbg_ota_dat_crc_err;
			cur_ota_flash_addr = ota_pkt_cnt = ota_rcv_last_idx = 0;
		    reset_flag = OTA_STATE_ERROR;
		#if 0
		}
		#endif
	}
	if(reset_flag){
	    #if 0   // can not set flag here for 8266, because of erase sector operation.
	    if(OTA_STATE_OK == reset_flag){
            rf_ota_set_flag (); // set flag first
	    }
	    #endif
	    rf_slave_ota_finished_flag = reset_flag;
	    rf_slave_ota_finished_time = clock_time();
	}
	dbg_ota++;
	return 1;
}

void rf_link_slave_ota_finish_led_and_reboot(int st)
{
    if(OTA_STATE_ERROR == st){
        erase_ota_data(flash_adr_ota_offset);
        rf_led_ota_error();
    }else if(OTA_STATE_OK == st){
        //rf_ota_save_data(0);
        rf_ota_set_flag (); 
        rf_led_ota_ok();
    }
    irq_disable ();
    light_sw_reboot();
}

void rf_link_slave_ota_finish_handle()
{
    if(rf_slave_ota_finished_flag){
        static u8 terminate_cnt;
        u8 reboot_flag = 0;
        if((0 == terminate_cnt) && (rf_slave_ota_terminate_flag)){
            if(is_add_packet_buf_ready()){
                terminate_cnt = 6;
                rf_link_add_tx_packet ((u32)(&pkt_terminate));
            }
        }
        
        if(terminate_cnt){
            terminate_cnt--;
            if(!terminate_cnt){
                reboot_flag = 1;
            }
        }
        
        if((!rf_slave_ota_terminate_flag)
         &&((u32)(clock_time() - rf_slave_ota_finished_time) > 2000*1000 * tick_per_us)){
            rf_slave_ota_terminate_flag = 1;    // for ios: no last read command
        }
        
        if(((u32)(clock_time() - rf_slave_ota_finished_time) > 4000*1000 * tick_per_us)){
            reboot_flag = 1;
        }
        
        if(reboot_flag){
            rf_link_slave_ota_finish_led_and_reboot(rf_slave_ota_finished_flag);
            // have been reboot
        }
    }
}

u32 get_ota_erase_sectors()
{
    return ERASE_SECTORS_FOR_OTA;
}

int is_ota_area_valid(u32 adr){
	u8 buf[4] = {0};
	foreach(i, ERASE_SECTORS_FOR_OTA){
    	flash_read_page(adr + i*0x1000, 4, buf);
    	u32 tmp = buf[0] | (buf[1]<<8) |(buf[2]<<16) | (buf[3]<<24);
    	if(tmp != ONES_32){
            return 0;
    	}
    }
	return 1;
}

void erase_ota_data(u32 adr){
    flash_protect_OTA_data_erase();
    #if 1
    foreach(i, ERASE_SECTORS_FOR_OTA){
        flash_erase_sector(adr+(ERASE_SECTORS_FOR_OTA -1 - i)*0x1000);
    }
    #else
    //  Note: differrent size or type may use differrent command of block erase.
    STATIC_ASSERT((ERASE_SECTORS_FOR_OTA % 16) == 0);
    u32 block_cnt = ERASE_SECTORS_FOR_OTA/16;
    foreach(i, block_cnt){
        flash_erase_block(adr+(block_cnt -1 - i)*0x10000);
    }
    #endif
}

void erase_ota_data_handle(){
	// for app ota  
    #if (MCU_CORE_TYPE == MCU_CORE_8267 || MCU_CORE_TYPE == MCU_CORE_8269)
    u32 adr_ota_data = ota_program_offset;  // 8267 should use "ota_program_offset"
    #else
    u32 adr_ota_data = flash_adr_light_new_fw;
    #endif
	if(0 == is_ota_area_valid(adr_ota_data)){
		erase_ota_data(adr_ota_data);
	}

    #if (MCU_CORE_TYPE == MCU_CORE_8267)
    flash_protect_8267_normal();    // must after erase_sector for OTA
    #elif (MCU_CORE_TYPE == MCU_CORE_8269)
    flash_protect_8269_normal();    // must after erase_sector for OTA
    #else
    flash_protect_8266_normal();
    #endif
}
void light_node_status_change_cb(u8 *p, u8 new_node){
    mesh_node_st_val_t *p_data = (mesh_node_st_val_t*)p;
    extern u8  sync_time_enable;
    if(sync_time_enable){
        p_data->par[0] &= ~FLD_SYNCED;   //Note: bit7 of par[0] have been use internal
    }
    static u8 dev_addr = 0;
    if(new_node){
        static u8 dev_new_node = 0;
        dev_new_node++;
        dev_addr = p_data->dev_adr;
    }else{
        static u8 dev_old_node = 0;
        dev_old_node++;
        dev_addr = p_data->dev_adr;
    }
}

void set_mesh_ota_type()
{
	u32 mesh_type = -1;
	flash_read_page(FLASH_ADR_MESH_TYPE_FLAG, 4,(u8 *)&mesh_type);
	if(mesh_type != -1){
		flash_erase_sector(FLASH_ADR_MESH_TYPE_FLAG);
	}
	mesh_type = TYPE_TLK_MESH;
	flash_write_page (FLASH_ADR_MESH_TYPE_FLAG, 4, (u8 *)&mesh_type);
}

u8 proc_sig_mesh_to_telink_mesh(void){
#if (MCU_CORE_TYPE == MCU_CORE_8267 || MCU_CORE_TYPE == MCU_CORE_8269)
	u32 mesh_type = *(u32 *) FLASH_ADR_MESH_TYPE_FLAG;
	if(-1 == mesh_type){	// 0xffffffff
		return 0;
	}

	u8 ret = 0;
	if(mesh_type == TYPE_SIG_MESH){
		u8 flash_data = 0;
		flash_read_page(FLASH_ADR_DC, 1, &flash_data);
		if(flash_data == 0xff){
			flash_read_page(0x77000, 1, &flash_data);
			flash_write_page(FLASH_ADR_DC, 1, &flash_data);
		}
		
		flash_read_page(FLASH_ADR_TP_LOW, 1, &flash_data);
		if(flash_data == 0xff){
			flash_read_page(0x77040, 1, &flash_data);
			flash_write_page(FLASH_ADR_TP_LOW, 1, &flash_data);
		}
		
		flash_read_page(FLASH_ADR_TP_HIGH, 1, &flash_data);
		if(flash_data == 0xff){
			flash_read_page(0x77041, 1, &flash_data);
			flash_write_page(FLASH_ADR_TP_HIGH, 1, &flash_data);
		}
		
		// no RC32K_CAP_INFO
		
		factory_reset();
		ret = 1;
	}

	flash_erase_sector(FLASH_ADR_MESH_TYPE_FLAG);
	return ret;
#endif
	return 0;
}


#if ADV_UUID
u8 adv_uuid_flag = 1;
#else
u8 adv_uuid_flag = 0;
#endif
u8 adv_uuid[4] = {0x03, 0x02, 0xAB, 0xCD};

// recover status before software reboot
void light_sw_reboot_callback(void){
#if ((__PROJECT_LIGHT_8266__)           \
    ||(__PROJECT_LIGHT_8267__)          \
    ||(__PROJECT_LIGHT_8269__)          \
    ||(__PROJECT_LIGHT_NO_MESH__))
    if(rf_slave_ota_busy || is_mesh_ota_slave_running()){
        analog_write (rega_light_off, light_off);
    }
#endif
}

/////////////////// mesh_node_filter  //////////////////////
#if 1
cb_mesh_node_filter_t	cb_mesh_node_filter = 0;
#else   // for test relay function  
int is_neighbor_light(u8 device_adr, u8 *p)
{
    rf_packet_att_cmd_t *pp = (rf_packet_att_cmd_t *)p;
    pp = pp;
    if((*(u8 *)0x60000 == device_adr) || (*(u8 *)0x60001 == device_adr)
     ||(*(u8 *)0x60002 == device_adr) || (*(u8 *)0x60003 == device_adr)
     ||(*(u8 *)0x60004 == device_adr) || (*(u8 *)0x60005 == device_adr)){
        // only receive some neighbor devices
        return 1;
    }
    return 0;
}

/*////////////////// mesh_node_filter  //////////////////////
int (*cb_mesh_node_filter_t) (u8 device_adr);
fuction      : use to test relay function  

device_adr : means this packet is sent from this device_adr
p              : point to this packet

return 0 : means skip this packet
return 1 : means accept this packet
*/

cb_mesh_node_filter_t	cb_mesh_node_filter = is_neighbor_light;
#endif

/////////////////// passive device  //////////////////////
u8 passive_en = PASSIVE_EN;
int get_command_type(u8 *p_att_value)
{
#if PASSIVE_EN
    rf_packet_att_value_t *p = (rf_packet_att_value_t*)(p_att_value);
    u8 op = p->val[0] & 0x3F;
    u8 *par = p->val+3;
    if(LGT_CMD_LIGHT_ONOFF == op){
        if(ON_OFF_FROM_PASSIVE_DEV == par[3]){
            return CMD_TYPE_PASSIVE_DEV;
        }else if(ON_OFF_FROM_PASSIVE_DEV_ALT == par[3]){
            return CMD_TYPE_PASSIVE_DEV_ALT;
        }
    }
#endif

    return CMD_TYPE_NORMAL;
}

// Note: par[8], par[9] of passive command have been used internal for sno2.
void set_command_type2alt(u8 *p_att_value)
{
#if PASSIVE_EN
    rf_packet_att_value_t *p = (rf_packet_att_value_t*)(p_att_value);
    u8 op = p->val[0] & 0x3F;
    u8 *par = p->val+3;
    if(LGT_CMD_LIGHT_ONOFF == op){
        if(ON_OFF_FROM_PASSIVE_DEV == par[3]){
            par[3] = ON_OFF_FROM_PASSIVE_DEV_ALT;
        }
    }
#endif
}

#if(MESH_PAIR_ENABLE)
u8 mesh_pair_enable = 0;
u32 mesh_pair_cmd_interval = 0;
u32 mesh_pair_start_time = 0;
u32 mesh_pair_timeout = 0;
u8 new_mesh_name[16] = {0};
u8 new_mesh_pwd[16] = {0};
u8 new_mesh_ltk[16] = {0};
u32 effect_new_mesh_delay_time = 0;
u8 effect_new_mesh = 0;
u32 mesh_pair_start_notify_time = 0;
u8 mesh_pair_retry_max = 3;
u8 mesh_pair_retry_cnt = 0;
u8 mesh_pair_notify_rsp_mask[32] = {0};
u8 mesh_pair_checksum[8] = {0};
u8 get_mesh_pair_checksum(u8 idx){
    u8 i = idx % 8;
    return (new_mesh_name[i] ^ new_mesh_name[i+8]) ^ (new_mesh_pwd[i] ^ new_mesh_pwd[i+8]) ^ (new_mesh_ltk[i] ^ new_mesh_ltk[i+8]);
}

u8 mesh_pair_notify_refresh(rf_packet_att_cmd_t *p){
    if(!memcmp(mesh_pair_checksum, p->value + 12, 8)){
        // mesh pair : success one device, clear the mask flag
        mesh_pair_notify_rsp_mask[(p->value[10]) / 8] &= ~(BIT(p->value[10] % 8));
    }
    return 1;// if return 2, then the notify rsp will not report to master.
}

void save_effect_new_mesh(void)
{
    if(effect_new_mesh == 0){
        memcpy4(pair_nn, new_mesh_name, 16);
        memcpy4(pair_pass, new_mesh_pwd, 16);
        memcpy4(pair_ltk, new_mesh_ltk, 16);
    }
    extern void pair_save_key();
    pair_save_key();
    rf_link_light_event_callback(LGT_CMD_SET_MESH_INFO);
    memset4(new_mesh_name, 0, 16);
    memset4(new_mesh_pwd, 0, 16);
    memset4(new_mesh_ltk, 0, 16);
    mesh_pair_start_notify_time = mesh_pair_retry_cnt = mesh_pair_start_time = 0;
    memset4(mesh_pair_notify_rsp_mask, 0, 32);
    pair_setting_flag = PAIR_SETTED;
}

void mesh_pair_cb(u8 *params)
{
    if(params[0] == MESH_PAIR_NAME1){
        mesh_pair_start_time = clock_time() | 1;
        memcpy(new_mesh_name, params + 1, 8);
    }else if(params[0] == MESH_PAIR_NAME2){
        memcpy(new_mesh_name + 8, params + 1, 8);
    }else if(params[0] == MESH_PAIR_PWD1){
        memcpy(new_mesh_pwd, params + 1, 8);
    }else if(params[0] == MESH_PAIR_PWD2){
        memcpy(new_mesh_pwd + 8, params + 1, 8);
    }else if(params[0] == MESH_PAIR_LTK1){
        memcpy(new_mesh_ltk, params + 1, 8);
    }else if(params[0] == MESH_PAIR_LTK2){
        memcpy(new_mesh_ltk + 8, params + 1, 8);
    }else if(params[0] == MESH_PAIR_EFFECT_DELAY){
        effect_new_mesh_delay_time = clock_time() | 1;
    }else if(params[0] == MESH_PAIR_EFFECT){
        effect_new_mesh = 1;
    }
}

u8 get_online_node_cnt(void)
{
    u8 cnt = 0;
    extern u8 mesh_node_max;
	foreach(i, mesh_node_max){
	    if(mesh_node_st[i].tick){
	        cnt++;
	        if(i > 0){
	            mesh_pair_notify_rsp_mask[mesh_node_st[i].val.dev_adr / 8] |= BIT(mesh_node_st[i].val.dev_adr % 8);
	        }
	    }
	}
	return cnt;
}

extern u8 pair_ltk_mesh[];
void mesh_pair_proc(void)
{
    static u32 mesh_pair_time = 0;
    static u8 mesh_pair_state = MESH_PAIR_NAME1;
    u16 dst_addr = 0xFFFF;
    u8 op_para[16] = {0};
    if(mesh_pair_start_time && clock_time_exceed_lib(mesh_pair_start_time, mesh_pair_timeout*1000*1000)){
        //mesh pair time out 
        pair_load_key();
        memset4(new_mesh_name, 0, 16);
        memset4(new_mesh_pwd, 0, 16);
        memset4(new_mesh_ltk, 0, 16);
        mesh_pair_state = MESH_PAIR_NAME1;
        mesh_pair_start_notify_time = mesh_pair_retry_cnt = mesh_pair_start_time = 0;
        memset4(mesh_pair_notify_rsp_mask, 0, 32);
        pair_setting_flag = PAIR_SETTED;
        rf_link_light_event_callback(LGT_CMD_MESH_PAIR_TIMEOUT);
        return;
    }
    
    if(pair_setting_flag == PAIR_SET_MESH_TX_START && (mesh_pair_state == MESH_PAIR_NAME1) && get_online_node_cnt() == 1){
        op_para[0] = LGT_CMD_MESH_PAIR;
        op_para[3] = MESH_PAIR_EFFECT;
        dst_addr = 0x0000;// there is noly one device in mesh,just effect itself.
        mesh_pair_state = MESH_PAIR_NAME1;
        mesh_pair_start_notify_time = mesh_pair_retry_cnt = mesh_pair_start_time = 0;
        memset4(mesh_pair_notify_rsp_mask, 0, 32);
        pair_setting_flag = PAIR_SETTED;
    }else if(pair_setting_flag >= PAIR_SET_MESH_TX_START && clock_time_exceed(mesh_pair_time, mesh_pair_cmd_interval*1000)){
        mesh_pair_time = clock_time();
        if(pair_setting_flag == PAIR_SET_MESH_TX_START){
            op_para[0] = LGT_CMD_MESH_PAIR;
            op_para[3] = mesh_pair_state;
            if(mesh_pair_state == MESH_PAIR_NAME1){
                // send mesh name [0-7]
        		memcpy(op_para + 4, pair_nn, 8);
        		mesh_pair_state = MESH_PAIR_NAME2;
            }else if(mesh_pair_state == MESH_PAIR_NAME2){
                // send mesh name [8-15]
        		memcpy(op_para + 4, pair_nn + 8, 8);
        		mesh_pair_state = MESH_PAIR_PWD1;
            }else if(mesh_pair_state == MESH_PAIR_PWD1){
                // send mesh pwd [0-7]
        		memcpy(op_para + 4, pair_pass, 8);
        		mesh_pair_state = MESH_PAIR_PWD2;
            }else if(mesh_pair_state == MESH_PAIR_PWD2){
                // send mesh pwd [8-15]
        		memcpy(op_para + 4, pair_pass + 8, 8);
        		mesh_pair_state = MESH_PAIR_LTK1;
            }else if(mesh_pair_state == MESH_PAIR_LTK1){
                // send mesh ltk [0-7]
        		memcpy(op_para + 4, pair_ltk_mesh, 8);
        		mesh_pair_state = MESH_PAIR_LTK2;
            }else if(mesh_pair_state == MESH_PAIR_LTK2){
                // send mesh ltk [8-15]
        		memcpy(op_para + 4, pair_ltk_mesh + 8, 8);
        		mesh_pair_state = MESH_PAIR_NAME1;
        		pair_setting_flag = PAIR_SET_MESH_TX_DONE;
            }else{
                mesh_pair_state = MESH_PAIR_NAME1;
                mesh_pair_start_notify_time = mesh_pair_retry_cnt = mesh_pair_start_time = 0;
                memset4(mesh_pair_notify_rsp_mask, 0, 32);
        		pair_setting_flag = PAIR_SETTED;
        		return;
            }
        }else if(pair_setting_flag == PAIR_SET_MESH_TX_DONE){
            // get mesh nodes' confirm value
            //rf_link_slave_read_status_start();
            op_para[0] = LGT_CMD_MESH_OTA_READ;
            op_para[3] = 0x10;// bridge cnt
            op_para[4] = PAR_READ_MESH_PAIR_CONFIRM;
            pair_setting_flag = PAIR_SET_MESH_RX_DONE;
            mesh_pair_start_notify_time = clock_time() | 0;
            foreach(i, 8){
                mesh_pair_checksum[i] = get_mesh_pair_checksum(i);
            }
        }else if(pair_setting_flag == PAIR_SET_MESH_RX_DONE){
            u8 zero_buff[32] = {0};
            u8 effect_flag = 0;
            effect_flag = !memcmp(mesh_pair_notify_rsp_mask, zero_buff, 32);
            if(!effect_flag && clock_time_exceed_lib(mesh_pair_start_time, MESH_PAIR_NOTIFY_TIMEOUT*1000)){
                if(mesh_pair_retry_cnt++ < mesh_pair_retry_max){
                    mesh_pair_start_time = clock_time() | 1;
                    pair_setting_flag = PAIR_SET_MESH_TX_START;
                    mesh_pair_state = MESH_PAIR_NAME1;
                }else{
                    // retry timeout, effect or cancel?? effect now
                    effect_flag = 1;
                }
            }
            if(effect_flag){
                //send cmd to switch to new mesh
                op_para[0] = LGT_CMD_MESH_PAIR;
                op_para[3] = MESH_PAIR_EFFECT_DELAY;
                mesh_pair_state = MESH_PAIR_NAME1;
                mesh_pair_start_notify_time = mesh_pair_retry_cnt = mesh_pair_start_time = 0;
                memset4(mesh_pair_notify_rsp_mask, 0, 32);
                pair_setting_flag = PAIR_SETTED;
            }
        }
    }else{
        return;
    }
    
    light_slave_tx_command(op_para, dst_addr);
}
#else
u8 mesh_pair_enable = 0;
void mesh_pair_proc(void)
{
    return;
}

u8 mesh_pair_notify_refresh(rf_packet_att_cmd_t *p){
    return 1;
}
#endif
#if 1	// no use internal
/*
int user_cmd2app_cmd()
cmd: mesh command (0--3F)
*/
int user_cmd2app_cmd(u8 cmd, u16 dst_adr, u8 *par, u8 par_len){	// should be in connected state
    if(par_len > 10 || (!(slave_link_connected))){
        return -1;
    }

    // packet 
	rf_packet_ll_app_t  pkt_app_data = {};
    memset(&pkt_app_data, 0, sizeof(pkt_app_data));
    pkt_app_data.type = 0x02;
    pkt_app_data.rf_len = 17 + par_len;
    pkt_app_data.dma_len = pkt_app_data.rf_len + 2;
    pkt_app_data.l2capLen = pkt_app_data.rf_len - 4;
    pkt_app_data.chanId = 0x04;
    pkt_app_data.opcode = ATT_OP_WRITE_CMD;
    pkt_app_data.handle= 0x15;//handle & 0xff;
    pkt_app_data.handle1 = 0x00;//(handle >> 8) & 0xff;

    static u8 user_sno_cmd_init_flag = 1;
	static int user_sno_cmd;
	if(user_sno_cmd_init_flag){
		user_sno_cmd_init_flag = 0;
		user_sno_cmd = ((clock_time() + device_address) & 0xff) << 16;;		// should be initialized as random value.
	}
	
    user_sno_cmd++;
    if(0 == user_sno_cmd){
    	user_sno_cmd = 1;		// make sure not zero.
    }
    
    memcpy(pkt_app_data.app_cmd_v.sno, &user_sno_cmd, 3);
    //memcpy(pkt_app_data.app_cmd_v.src, &device_address, 2);
    memcpy(pkt_app_data.app_cmd_v.dst, &dst_adr, 2);
    pkt_app_data.app_cmd_v.op = (cmd & 0x3F) | 0xC0;
    pkt_app_data.app_cmd_v.vendor_id = VENDOR_ID;
    memcpy(pkt_app_data.app_cmd_v.par, par, par_len);
    
    // handle and send command
    u8 r = irq_disable();
    rf_link_slave_data_write_no_dec((rf_packet_ll_data_t *)(&pkt_app_data));
    irq_restore(r);
    
    return 0;
}
#endif

/////////////////// set and get mac by mesh  //////////////////////
u8 get_mac_en = 1;		// no use in library

int dev_addr_with_mac_flag(u8 *params)
{
	return (DEV_ADDR_PAR_WITH_MAC == params[2]);
}

int dev_addr_with_mac_match(u8 *params)
{
	if((params[0] == 0xff)&&(params[1] == 0xff)){	// get
		return get_mac_en;
	}else{											// set
		return (!memcmp(params+4, slave_p_mac, 6));
	}
}

int dev_addr_with_mac_rsp(u8 *params, u8 *par_rsp)
{
	if(dev_addr_with_mac_match(params)){
		memcpy(par_rsp, &device_address, 2);
		memcpy(par_rsp + 2, slave_p_mac, 6);
		return 1;
	}
	return 0;
}

/////////////////// Adjust TP according to the temperature  //////////////////////
#if  ADC_TEMP_ENABLE
#define ADC_TEMP_SAMP_PERIOD		3000000		// unit: us
#define TEMP_ADC_TP_CRIT_VALUE		5

#define	TEMP_HIGH_VAL				(80)		// degree centigrade

u8			adc_adj_tp_flag = 1;
u8 			rf_tp_base_init;
u8 			rf_tp_delta;

u8 adc_adj_tp_map[][2]=
{
	{25,0x11},				// room temperature
//	{40,0x12},
//	{60,0x13},
	{TEMP_HIGH_VAL,0x14},
//	{100,0x15},
//	{120,0x16},
};

extern int rf_tp_base,rf_tp_gain;

void adc_adj_tp_init(void)
{
	if (*(u8 *) FLASH_ADR_TP_HIGH != 0xFF){
		rf_tp_delta = *(u8 *) FLASH_ADR_TP_LOW - *(u8 *) FLASH_ADR_TP_HIGH;
    }
	else
	{
		rf_tp_delta = rf_get_default_tp_delta();
	}

	rf_tp_base_init = rf_tp_base;
	
	adc_adj_tp_map[0][1] = rf_tp_base_init;	//tp base  when temperature = 25
	
	foreach_arr(i,adc_adj_tp_map){
		if(adc_adj_tp_map[i][0] == TEMP_HIGH_VAL){
			adc_adj_tp_map[i][1] = rf_tp_base_init + 3;
		}
	}
	
}

void ADC_TempSensorInit(enum ADCINPUTCH chn){
	WriteAnalogReg(0x05, ReadAnalogReg(0x05) & 0xdf);
	adc_ChannelInit(chn, SINGLEEND, RV_AVDD, RES14, S_3);
}


#define ADC_SIZE 10

unsigned short TEMSENSOR_N_buff[ADC_SIZE] = {0x00};
unsigned short TEMSENSOR_P_buff[ADC_SIZE] = {0x00};
unsigned int TEMSENSOR_N_value = 0;
unsigned int TEMSENSOR_P_value = 0;
unsigned int TEMSENSOR_voltage = 0;   		//0.00mv
int adc_temperature = 0;				//temperature value 0.0 degree
unsigned int dif_PN = 0;        			//P_ch - N_ch

int adc_get_temp(void)
{
	unsigned short i = 0;
	unsigned int temp = 0;
	static unsigned int sum = 0;

	//Init AD ch TEMSENSORN
	ADC_TempSensorInit(TEMSENSORN);
	for(i=0;i<ADC_SIZE+8;i++){
		if(i<8)
		{
			temp = adc_SampleValueGet();////get ADC value of pin B1
		}
		else
		{
			TEMSENSOR_N_buff[i-8] = adc_SampleValueGet();////get ADC value of pin B1
			sum += TEMSENSOR_N_buff[i-8];
		}
	}
	sum /= ADC_SIZE;
	if(sum < 128){
		sum = 128;
	}
	TEMSENSOR_N_value = sum;
	sum = 0;

	//Init AD ch TEMSENSORP
	ADC_TempSensorInit(TEMSENSORP);
	for(i=0;i<ADC_SIZE+8;i++){
		if(i<8)
		{
			temp = adc_SampleValueGet();////get ADC value of pin B1
		}
		else
		{
			TEMSENSOR_P_buff[i-8] = adc_SampleValueGet();////get ADC value of pin B1
			sum += TEMSENSOR_P_buff[i-8];
		}
	}
	sum /= ADC_SIZE;
	if(sum < 128){
		sum = 128;
	}
	TEMSENSOR_P_value = sum;
	sum = 0;

	dif_PN = TEMSENSOR_P_value - TEMSENSOR_N_value;
	TEMSENSOR_voltage = 306100*(dif_PN-128)/(16383-256);
	temp = (TEMSENSOR_voltage-14362)*1650/10006 - 400;

	return temp;
}

unsigned char adc_temp_proc(void){
	
	//static u32 temp_time_dly,temp_time_dly2;
	
	//temp_time_dly = clock_time();
	u8 r = irq_disable();

	adc_temperature = adc_get_temp();
	
	adc_chn_ref_init();			// recover channel and reference configure

	irq_restore(r);
	//temp_time_dly2 = (clock_time() - temp_time_dly)/CLOCK_SYS_CLOCK_1US;
	
	return 1;
}

void adc_adj_tp_proc(void)
{
	u8 tmp_tp_base = rf_tp_base;

	foreach_arr(i,adc_adj_tp_map){
		if(adc_temperature > (adc_adj_tp_map[i][0]*10+TEMP_ADC_TP_CRIT_VALUE*10)&& rf_tp_base < adc_adj_tp_map[i][1]){
			tmp_tp_base = adc_adj_tp_map[i][1];
		}
		else if(rf_tp_base == adc_adj_tp_map[i][1] && adc_temperature < (adc_adj_tp_map[i][0]*10-TEMP_ADC_TP_CRIT_VALUE*10)){
			if(i>0){
				tmp_tp_base = adc_adj_tp_map[i-1][1];
			}
		}
	}

	if(tmp_tp_base != rf_tp_base){
		rf_tp_base = tmp_tp_base;
		rf_tp_gain = rf_get_tp_gain(rf_tp_base, rf_tp_base-rf_tp_delta);
	}
}

void adc_adj_tp_handle(void)
{
	static u32 adj_tp_check_time;
	
	if(adc_adj_tp_flag && clock_time_exceed(adj_tp_check_time, ADC_TEMP_SAMP_PERIOD)){
		if(adc_temp_proc()){
			adj_tp_check_time = clock_time();
			adc_adj_tp_proc();
		}
	}
}

void adc_adj_tp_reset(void)
{
	if(rf_tp_base != rf_tp_base_init){
		rf_tp_base = rf_tp_base_init;
		rf_tp_gain = rf_get_tp_gain(rf_tp_base, rf_tp_base-rf_tp_delta);
	}
	if(adc_adj_tp_flag){
		adc_adj_tp_flag = 0;
	}
}

#endif

