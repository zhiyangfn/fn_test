
#pragma once

#include "../../proj/tl_common.h"
#include "../../proj_lib/ble_ll/blueLight.h"
#include "../../proj/drivers/i2c.h"
#include "../../vendor/common/crc.h"

void light_onoff_step(u8 on);
void light_onoff_step_timer();
void light_step_correct_mod(u16 *pwm_val, u8 lum);
void light_onoff(u8 on);
void light_onoff_step_init();
u8 is_lum_invalid(u8 lum);

void pa_init(u8 tx_pin_level, u8 rx_pin_level);
void pa_txrx(u8 val);
void light_slave_tx_command_callback (u8 *p);
void rf_link_slave_connect_callback();
void update_ble_parameter_cb();
int light_slave_tx_command(u8 *p_cmd, int para);
int factory_reset();

/////////////// adv par define ///////////////////////////////////////////////////
extern u16 adv_interval2listen_interval;
extern u16 online_status_interval2listen_interval;
extern u8  separate_ADVpkt;
extern u8  mesh_chn_amount;
/////////////// mesh node define ////////////////////////////////////////////////////

#if (__PROJECT_LIGHT_SWITCH__)
#define				MESH_NODE_MAX_NUM		32      // switch use minimum value
#else
#define				MESH_NODE_MAX_NUM		64      // must multy of 32 and max 256
#endif

/*
Note: ONLINE_STATUS_TIMEOUT may be set longer after set MESH_NODE_MAX_NUM or MESH_NODE_ST_VAL_LEN larger, 
*/
#define				MESH_NODE_ST_VAL_LEN    4       // MIN: 4,   MAX: 10
#define				MESH_NODE_ST_PAR_LEN    (MESH_NODE_ST_VAL_LEN - 2)  //  //lumen-rsv, exclude dev_adr and sn.

typedef struct{
    u8 dev_adr;     // don't change include type
    u8 sn;          // don't change include type
    u8 par[MESH_NODE_ST_PAR_LEN];  //lumen-rsv,
}mesh_node_st_val_t;

typedef struct{
    u16 tick;       // don't change include type
    mesh_node_st_val_t val;
}mesh_node_st_t;

typedef struct{
    u8 adr[1];      // don't modify, use internal
    u8 alarm_id;    // don't modify, use internal
}status_record_t;

extern status_record_t slave_status_record[];
extern u16 slave_status_record_size;

extern u8 mesh_node_st_val_len;
extern u8 mesh_node_st_par_len;
extern u8 mesh_node_st_len;
extern u16 mesh_node_max_num;

void mesh_node_buf_init ();
void device_status_update();

extern u8   SW_Low_Power;

extern void get_flash_user_data();
extern void check_store_user_data();
extern u8 flash_user_data[16];
extern u8 user_data_idx;

////////////////// gate way /////////////////////////////////////
void gateway_init();
int gatway_tx_command(u8 cmd, u16 dst_adr, u8 *par, u8 par_len, u8 op);
int gateway_mode_onoff(u8 on);
void proc_ui_gateway ();
void gateway_set_login_flag();
void ble_event_callback (u8 status, u8 *p, u8 rssi);
void proc_host ();
int host_write_master (int n);
void host_init ();

extern u8   gateway_en;
extern u8   gateway_security;
extern u8   mode_master;

////////////////// check stack /////////////////////////////////////
void stack_check_init();
void stack_check();
////////////////// ALARM and SCENE /////////////////////////////////////
int is_bridge_task_busy();
extern u8 	mesh_user_cmd_idx;
int is_tx_cmd_busy();

////////////////// OTA  /////////////////////////////////////
u32 get_ota_erase_sectors();
void erase_ota_data(u32 adr);
int is_ota_area_valid(u32 adr);
void erase_ota_data_handle();
extern u32  ota_program_offset;

extern u8 iBeaconInterval;
extern u8 iBeaconData[];
extern u8 eddystone_uid[];
extern u8 eddystone_url[];
extern u8 eddystone_tlm[];
extern u8 beacon_len;
extern u8 beacon_with_mesh_adv;// 0 means only send beacon adv pkt;  1 means send both of beacon pkt and mesh adv pkt
extern void set_ibeacon_data(u8 *val, int n);

/////////////// encode / decode password  //////////////
void encode_password(unsigned char *pd);
void decode_password(unsigned char *pd);
extern u8   pair_config_pwd_encode_enable;

extern u8 adv_uuid_flag;
extern u8 adv_uuid[4];

// recover status before software reboot
#define		rega_light_off              0x34
void light_sw_reboot_callback(void);

/////////////////// mesh_node_filter  //////////////////////
typedef int (*cb_mesh_node_filter_t) (u8 device_adr, u8 *p);
extern cb_mesh_node_filter_t	cb_mesh_node_filter;

/////////////////// call back function  //////////////////////
typedef void (*cb_func_void_t) ();
extern cb_func_void_t	p_cb_ble_slave_disconnect;
extern cb_func_void_t	p_cb_pair_failed;

/////////////////// passive device  //////////////////////
int get_command_type(u8 *p_att_value);
void set_command_type2alt(u8 *p_att_value);
extern u8 passive_en;

enum {
	CMD_TYPE_NORMAL				= 0,
	CMD_TYPE_PASSIVE_DEV		= 1,
	CMD_TYPE_PASSIVE_DEV_ALT	= 2,
};

/////////////////// mesh ota master  //////////////////////
void mesh_ota_master_start_firmware_from_20000(u8 dev_mode);
void mesh_ota_master_start_firmware_from_own();
void mesh_ota_led_cb(u32 type);

///////////////////////////////////////////////////
int user_cmd2app_cmd(u8 cmd, u16 dst_adr, u8 *par, u8 par_len);
extern u8 tx_packet_bridge_random_en;

/////////////////// set and get mac by mesh  //////////////////////
int dev_addr_with_mac_flag(u8 *params);
int dev_addr_with_mac_match(u8 *params);
int dev_addr_with_mac_rsp(u8 *params, u8 *par_rsp);
extern u8* 	slave_p_mac;
extern u8 get_mac_en;
extern u8 mesh_pair_enable;
extern void mesh_pair_proc(void);
u8 mesh_pair_notify_refresh(rf_packet_att_cmd_t *p);
enum{
    PAIR_SETTED = 0,
    PAIR_SETTING,
    PAIR_SET_MESH_TX_START,
    PAIR_SET_MESH_TX_DONE,// send notify req, get mesh nodes' ac
    PAIR_SET_MESH_RX_DONE,// received all mesh nodes' ac, send cmd to switch to new mesh
};
#if(MESH_PAIR_ENABLE)
//unit: ms
#define MESH_PAIR_CMD_INTERVAL          500
extern u32 mesh_pair_cmd_interval;
//unit: s
#define MESH_PAIR_TIMEOUT               10
//unit: ms
#define MESH_PAIR_NOTIFY_TIMEOUT        2500
extern u32 mesh_pair_timeout;
extern u32 mesh_pair_start_time;
extern u8 pair_nn[];
extern u8 pair_pass[];
extern u8 pair_ltk[];
extern u32 pair_ac;
extern u8 new_mesh_name[];
extern u8 new_mesh_pwd[];
extern u8 new_mesh_ltk[];
extern u32 effect_new_mesh_delay_time;
extern u8 effect_new_mesh;
extern u8 pair_setting_flag;
extern u8 get_mesh_pair_checksum(u8 idx);
extern void save_effect_new_mesh(void);
extern void mesh_pair_cb(u8 *params);
extern void pair_load_key ();
extern u8 mesh_pair_checksum[];

enum{
    MESH_PAIR_NAME1 = 0,
    MESH_PAIR_NAME2,
    MESH_PAIR_PWD1,
    MESH_PAIR_PWD2,
    MESH_PAIR_LTK1,
    MESH_PAIR_LTK2,
    MESH_PAIR_EFFECT_DELAY,
    MESH_PAIR_EFFECT,
};
enum{
    USER_ALL_GET_MAC = 1,
    USER_ALL_GET_PAIR_CONFIRM,
};

#endif

#if(IBEACON_ENABLE)
//apple ibeacon
typedef struct{
    u8 flag_len; //constant, 0x02
    u8 flag_type;//constant, 0x01, flags type
    u8 flag; //constant, 0x06, flags indicating LE General discoverable and not supporting BR/EDR
    u8 ibeacon_len;//constant, 0x1A
    u8 manu_spec; //constant, 0xFF
    u16 company_ID;//constant,0X004C, Apple Inc
    u16 beacon_type; //constant,0X0215
    u8 proximity_UUID[16];//Defined by customer
    u16 major;//Defined by Customer
    u16 minor;//Defined by Customer
    s8 measured_power;//Defined by Customer, TX Power-41
}ibeacon_adv_t;
extern ibeacon_adv_t ibeacon_customer_data;
#endif
extern unsigned char adc_temp_proc(void);
extern void adc_adj_tp_handle(void);
extern void adc_adj_tp_init(void);

void rf_link_slave_ota_finish_led_and_reboot(int st);

