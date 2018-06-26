
#include "../../proj/tl_common.h"
#include "../../proj_lib/ble/blt_config.h"

#include "tl8267_uart.h"
#include "libflashdata.h"
#include "libzledsend.h"


#define DEVICE_PARAM_OFFSET		0x1000
#define CONFIG_PARAM_OFFSET		0x1000

#define FLASH_STAET_ADDR		0x78000
#define FLASH_DEVICE_ADDR		FLASH_STAET_ADDR
#define FLASH_CONFIG_ADDR		(FLASH_DEVICE_ADDR + DEVICE_PARAM_OFFSET)

config_parameter_t g_config_param;
device_parameter_t g_device_param;
time_cur_t g_time_cur = {0,0,0,0};


u8 g_need_WriteFalsh = WRITE;

const u8 flash_init_check[4]={0x30,0x08,0x17,0x20};

void Init_flash_config()
{
	memcpy(g_config_param.flash_init, flash_init_check,4);
	g_config_param.led_status =1;
	g_config_param.init_camera =1;
	g_config_param.led_nums =50;
	g_config_param.led_light =255;
	g_config_param.one_seg_mode =0;
	g_config_param.led_work_mode.mode =LED_WORK_MODE_MUSIC;
	if((g_device_param.production[1]==0x01&&g_device_param.production[0]==0x03)||\
	   (g_device_param.production[1]==0x02&&g_device_param.production[0]==0x09)){
		g_config_param.led_work_mode.music_mode.mode = 100;
	}
	else{
		g_config_param.led_work_mode.music_mode.mode = 0;
	}
	
	g_config_param.led_work_mode.music_mode.color[0]=255;
	g_config_param.led_work_mode.music_mode.color[1]=0;
	g_config_param.led_work_mode.music_mode.color[2]=0;

	g_config_param.led_work_mode.color_mode.color[0] =255;
	g_config_param.led_work_mode.color_mode.color[1] =0;
	g_config_param.led_work_mode.color_mode.color[2] =0;
	
	g_config_param.led_work_mode.scene_mode.mode = 0;
	
	g_config_param.switch_time.flag = 0;
	g_config_param.switch_time.on_hour = 0;
	g_config_param.switch_time.on_hour = 0;
	g_config_param.switch_time.off_hour= 23;
	g_config_param.switch_time.off_min = 59;

	flash_erase_sector(FLASH_CONFIG_ADDR);
	gm_printf("Init_flash_config:%d\r\n",sizeof(g_config_param));
	flash_write_page ((u32)(FLASH_CONFIG_ADDR), sizeof(g_config_param), (u8*)(&g_config_param));
	u32 *pdata1= (u32*)(FLASH_CONFIG_ADDR);
	gm_printf(" Init_flash head %x\r\n",*pdata1);
}

void Reset_flash_config()
{
	g_config_param.flash_init[0] = 0xAA;
	g_config_param.flash_init[1] = 0xBB;
	g_config_param.flash_init[2] = 0xCC;
	g_config_param.flash_init[3] = 0xDD;
	flash_erase_sector(FLASH_CONFIG_ADDR);
	gm_printf(" Reset_flash head \r\n");
	flash_write_page ((u32)(FLASH_CONFIG_ADDR), sizeof(g_config_param), (u8*)(&g_config_param));
}

void Init_flash_device()
{
	char production_init[4]={0x06,0x01,0x00,0x02};
	char hw_version_init[4]={0x00,0x01,0x00,0x00};
	char production_date_init[8]={0x02,0x00,0x01,0x08,0x00,0x02,0x02,0x5};
	
	memcpy(g_device_param.flash_init, flash_init_check,4);
	memcpy(g_device_param.production, production_init,4);
	memcpy(g_device_param.hw_version, hw_version_init,4);
	memcpy(g_device_param.production_date, production_date_init,8);
	g_device_param.chip_number[0] = 0x30;
	g_device_param.led_length[0]  = 0x05;

	flash_erase_sector(FLASH_DEVICE_ADDR);
	flash_write_page ((u32)(FLASH_DEVICE_ADDR), sizeof(g_device_param), (u8*)(&g_device_param));
}

void Config_Write_to_Flash(void)
{
	flash_erase_sector(FLASH_CONFIG_ADDR);
	flash_write_page ((u32)(FLASH_CONFIG_ADDR), sizeof(g_config_param), (u8*)(&g_config_param) );

}

void device_read_form_Flash(void)
{
	u32 *pdata1= (u32*)(FLASH_DEVICE_ADDR);
	u8 i ,flag;
	gm_printf("device head %x\r\n",*pdata1);
	#if 0
	gm_printf("device head %x\r\n",*pdata1);
	if(0x20170830!=*pdata1){		//device is error 
		gm_printf("device is error\r\n");
		Init_flash_device();
		*pdata1= (u32*)(FLASH_DEVICE_ADDR);
		gm_printf("device Init %x\r\n",*pdata1);
	}
	else{
		flash_read_page((u32)(FLASH_DEVICE_ADDR) ,sizeof(g_device_param), (u8*)(&g_device_param) );
	}
	#endif
	flash_read_page((u32)(FLASH_DEVICE_ADDR) ,sizeof(g_device_param), (u8*)(&g_device_param) );
	flag = 0;
	for(i = 0 ; i< 4 ;i++){
		if(g_device_param.production[i]>0x09)flag++;
	}
	if(flag==0){
		sprintf(productName,"H%d%d%d%d",g_device_param.production[3],g_device_param.production[2],\
			g_device_param.production[1],g_device_param.production[0]);
	}
}

void config_read_form_Flash(void)
{
	u32 *pdata1= (u32*)(FLASH_CONFIG_ADDR);
	gm_printf("config head %x\r\n",*pdata1);
	if(0x20170830!=*pdata1){		//device is error 
		gm_printf("config is Init\r\n");
		Init_flash_config();
	}
	else{
		gm_printf("config is power read\r\n");
		flash_read_page((u32)(FLASH_CONFIG_ADDR) ,sizeof(g_config_param), (u8*)(&g_config_param) );
	}

	if(	g_config_param.led_work_mode.music_mode.color[0]==0&&\
		g_config_param.led_work_mode.music_mode.color[1]==0&&\
		g_config_param.led_work_mode.music_mode.color[2]==0){

		g_config_param.led_work_mode.music_mode.color[0]=255;
		g_config_param.led_work_mode.music_mode.color[1]=0;
		g_config_param.led_work_mode.music_mode.color[2]=0;
	}

	if(	g_config_param.led_work_mode.color_mode.color[0]==0&&\
		g_config_param.led_work_mode.color_mode.color[1]==0&&\
		g_config_param.led_work_mode.color_mode.color[2]==0){

		g_config_param.led_work_mode.color_mode.color[0]=255;
		g_config_param.led_work_mode.color_mode.color[1]=0;
		g_config_param.led_work_mode.color_mode.color[2]=0;
	}
}


void System_Flash_Init()
{
	device_read_form_Flash();
	config_read_form_Flash();
}


#if 0
static u32 t1,t2,t;
void test_flash_app()
{
	u32 *pdata1= (u32*)( FLASH_DEVICE_ADDR);
	u32 *pdata2= (u32*) (FLASH_CONFIG_ADDR);
	u32 data;

	t1 = reg_system_tick;
	Config_Write_to_Flash();
	t2 = reg_system_tick;

	t=t2 -t1;
	U1_printf("%d\r\n",t);
	//device Msg
	U1_printf("g_device_param size:%d\r\n",sizeof(g_device_param));
	for(u8 i=0;i<sizeof(g_device_param)/4;i++){
		data = (u32)*(pdata1+i);
		U1_printf("add %x -data %x\r\n",pdata1+i ,data);
	}
	//config msg
	U1_printf("g_device_param size:%d\r\n",sizeof(g_config_param));
	for(u8 i=0;i<sizeof(g_config_param)/4;i++){
		data = (u32)*(pdata2+i);
		U1_printf("add %x -data %x\r\n",pdata2+i ,data);
	}
}

#endif
