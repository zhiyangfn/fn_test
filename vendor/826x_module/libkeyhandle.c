
#include "../../proj/tl_common.h"
#include "../../proj_lib/ble/blt_config.h"
#include "../../proj/drivers/i2c.h"
#include "../../proj/mcu/watchdog_i.h"

#include "tl8267_uart.h"
#include "libkeyhandle.h"
#include "libzledsend.h"
#include "tl8267_led.h"
#include "libflashdata.h"

#define KEY_LIGHT_MARGIN 50
#define MIN_LUMINANCE    50

void Led_KeyOpen_Tap();
void Led_KeyColor_Tap();
void Led_KeyMode_Tap();
void Led_KeyMode_Press();
void Led_Key_ChangeLight();
void Led_KeyFactory_Tap();

/*
 * @Desription: 初始化 按键
 * @param	:
 * @return 	:
 */

const u8  color_rainbow_stand[9][3]= {
		{200	,255	,255},		//cold white
		{245	,195	,40},		//warm white
		{255	,0		,0	},		//red
		{255	,127	,0	},		//orange
		{255	,255	,0	},		//yellow
		{0		,255	,0	},		//green
		{0		,0		,255},		//blue
		{0		,255	,255},		//cyan
		{139	,0		,255},		//purple
};


void Init_key(void)
{
	//switch to open or close
	gpio_set_func(KEY_OPEN, AS_GPIO);
	gpio_set_output_en(KEY_OPEN, 0);//disable output
	gpio_set_input_en(KEY_OPEN, 1); //enable input
	gpio_setup_up_down_resistor(KEY_OPEN, PM_PIN_PULLUP_10K);  //open pull up resistor

	gpio_set_func(KEY_COLOR, AS_GPIO);
	gpio_set_output_en(KEY_COLOR, 0);//disable output
	gpio_set_input_en(KEY_COLOR, 1); //enable input
	gpio_setup_up_down_resistor(KEY_COLOR, PM_PIN_PULLUP_10K);  //open pull up resistor

	gpio_set_func(KEY_MODE, AS_GPIO);
	gpio_set_output_en(KEY_MODE, 0);//disable output
	gpio_set_input_en(KEY_MODE, 1); //enable input
	gpio_setup_up_down_resistor(KEY_MODE, PM_PIN_PULLUP_10K);  //open pull up resistor

}


/*
 * @Desription: key scan at 10ms tick
 * @param	:
 * @return 	:
 */
static u16  key1_Cnt=0,key2_Cnt=0,key3_Cnt =0; //按键长短的计时
static u16 key1_Multi_Cnt =0, key2_Multi_Cnt =0, key3_Multi_Cnt =0;
static u8 key_group_Cnt =0;
#define MAX_HOLD_CNT  50//scan period * HOLD_CNT = pressTime
#define PRESS_MULTI_CNT	60

u16 keys_Status = NOTHING;

 u8 Scan_KeyLow_Value(void)
{
/*************Key_Open 1**********/
    if( !KEY_OPEN_VALUE )
    {
    	key1_Cnt+=1;
        if(key1_Cnt>=MAX_HOLD_CNT)//长按
        {
            keys_Status |= OPEN_PRESS_HOLD;
        	if(!(keys_Status & OPEN_PRESS_MULTI) )
        	{
				key1_Multi_Cnt +=1;
				if(key1_Multi_Cnt > PRESS_MULTI_CNT){
					keys_Status |= OPEN_PRESS_MULTI;
				}
        	}
        	else
        	{
        		key1_Multi_Cnt =0;
        	}
        }
    }
    else if( KEY_OPEN_VALUE!=0 && (key1_Cnt>3 && key1_Cnt<MAX_HOLD_CNT))//消抖短按
    {
    	key1_Cnt=0;
        keys_Status |= OPEN_TAP;
    }
    else
    {
    	key1_Cnt=0;
    	key1_Multi_Cnt =0;
    	key_group_Cnt =0;
    }

/*************Key_Color 2**********/
    if( !KEY_COLOR_VALUE )
    {
    	key2_Cnt+=1;
        if(key2_Cnt>=MAX_HOLD_CNT)//长按
        {
//            keys_Status |= COLOR_PRESS_HOLD;
        	if(!(keys_Status & COLOR_PRESS_MULTI) )
        	{
				key2_Multi_Cnt +=1;
				if(key2_Multi_Cnt > PRESS_MULTI_CNT){
					keys_Status |= COLOR_PRESS_MULTI;
				}
        	}
        	else
        	{
        		key2_Multi_Cnt =0;
        	}
        }
    }
    else if( KEY_COLOR_VALUE && (key2_Cnt>3 && key2_Cnt<MAX_HOLD_CNT))//消抖短按
    {
    	key2_Cnt=0;
        keys_Status |= COLOR_TAP;

        if( keys_Status & OPEN_PRESS_HOLD){
        	key_group_Cnt+=1;
        	if(key_group_Cnt >=4){
        		key_group_Cnt = 0;
        		keys_Status |=OPEN_COMBINE_COLOR;
        	}
        }
    }
    else
    {
    	key2_Cnt=0;
    	key2_Multi_Cnt =0;
    }
/*************Key_Mode 3**********/
    if( !KEY_MODE_VALUE )
    {
    	key3_Cnt+=1;
        if(key3_Cnt>=MAX_HOLD_CNT)//长按
        {
//            keys_Status |= MODE_PRESS_HOLD;
        	if(!(keys_Status & MODE_PRESS_MULTI) )
        	{
				key3_Multi_Cnt +=1;
				if(key3_Multi_Cnt > PRESS_MULTI_CNT){
					keys_Status |= MODE_PRESS_MULTI;
				}
        	}
        	else
        	{
        		key3_Multi_Cnt =0;
        	}
        }
    }
    else if( KEY_MODE_VALUE && (key3_Cnt>3 && key3_Cnt<MAX_HOLD_CNT))//消抖短按
    {
    	key3_Cnt=0;
        keys_Status |= MODE_TAP;
    }
    else
    {
    	key3_Cnt=0;
    	key3_Multi_Cnt =0;
    }

    return keys_Status;
}

void Key_Handle(void)
{
	if(keys_Status!=NOTHING){
		key_printf("key Handle :%x\r\n",keys_Status);
		g_System_Status |= NEED_WRITE_FLASH;

	}
	switch(keys_Status)
	{
		case	OPEN_COMBINE_COLOR|OPEN_PRESS_HOLD|COLOR_TAP: //openHold  combine colorTap for 5x
				Led_KeyFactory_Tap();
		break;

		case	OPEN_TAP:
			Led_KeyOpen_Tap();
			break;
		case 	OPEN_PRESS_MULTI:
			break;


		case	COLOR_TAP:
			Led_KeyColor_Tap();
			break;
		case 	COLOR_PRESS_MULTI:
			break;


		case	MODE_TAP:
			Led_KeyMode_Tap();
			break;
		case 	MODE_PRESS_MULTI:
			Led_KeyMode_Press();
			break;

		default:
			break;
	}
	keys_Status = NOTHING;
}


void Led_KeyFactory_Tap()
{
	u8 i = 0;
	key_printf("Restore Factory Defaults \r\n");
	
	Reset_flash_config();
	wd_clear();
	for(i = 0; i< 17;i++)
	{
		libzled_setColor(0xff,0x00,0x30,100,SEND_SOFT1_MODE);
		WaitMs(20);
		libzled_setColor(0x00,0x00,0x00,100,SEND_SOFT1_MODE);
		WaitMs(20);
		wd_clear();
	}
	while(!KEY_OPEN_VALUE){
		wd_clear();
	}
	while(1);
}


void Led_KeyOpen_Tap()
{
	if(CLOSE==g_config_param.led_status){			//led opening
		g_config_param.led_status = OPEN;
		start_led_service();
		key_printf("\rkey_open press ,led opening.....%d\r\n",g_config_param.led_status);
	}
	else{
		g_config_param.led_status = CLOSE;			//led closeing
		stop_all_led_service();
		key_printf("\rkey_open press ,led closeing.....%d\r\n",g_config_param.led_status);
	}
}

void Led_KeyColor_Tap()
{
	static u8  gLedColorMode = 0;
	if(CLOSE==g_config_param.led_status)return;
	if(g_config_param.led_work_mode.mode!=LED_WORK_MODE_COLOR){
		stop_led_service();
	}
	
	if(LED_WORK_MODE_COLOR==g_config_param.led_work_mode.mode)gLedColorMode++;
	if(gLedColorMode>PURPLE){
		gLedColorMode=0;//
	}
	g_config_param.led_work_mode.mode = LED_WORK_MODE_COLOR;
	g_config_param.led_work_mode.color_mode.color[0] = color_rainbow_stand[gLedColorMode][0];//
	g_config_param.led_work_mode.color_mode.color[1] = color_rainbow_stand[gLedColorMode][1];//
	g_config_param.led_work_mode.color_mode.color[2] = color_rainbow_stand[gLedColorMode][2];//
	libzled_setConstColor(g_config_param.led_work_mode.color_mode.color[0],g_config_param.led_work_mode.color_mode.color[1],g_config_param.led_work_mode.color_mode.color[2],SEND_SOFT1_MODE);
	key_printf("\rkey_color press ,color mode.....%d\r\n",gLedColorMode);	
}


void Led_KeyMode_Tap()
{
	u8 music_mode =0;
	if(CLOSE==g_config_param.led_status)return;
	stop_led_service();

	music_mode = g_config_param.led_work_mode.music_mode.mode;

	if(music_mode<100){// ic
		if(LED_WORK_MODE_MUSIC==g_config_param.led_work_mode.mode)
		{
			if(music_mode==0){//in order to compatible app
				music_mode =3;
			}else if(music_mode ==3)
			{
				music_mode=1;
			}else if(music_mode==1){
				music_mode =2;
			}else{
				music_mode =0;
			}
		}
	}else{//no ic
		if(LED_WORK_MODE_MUSIC==g_config_param.led_work_mode.mode)music_mode++;

		if(music_mode>101){
			music_mode =100;
		}
	}
	g_config_param.led_work_mode.mode = LED_WORK_MODE_MUSIC;
	g_config_param.led_work_mode.music_mode.mode =music_mode ;

	start_led_service();
	key_printf("\r Key Mode Music & mode=%d\r\n",music_mode);

}	

void Led_KeyMode_Press()
{
	if(CLOSE==g_config_param.led_status)return;
	Led_Key_ChangeLight();
}

void Led_Key_ChangeLight()
{
	u16 Luminance = libzled_getMaxLuminance();
	Luminance+=KEY_LIGHT_MARGIN;
	if(Luminance>250) Luminance=MIN_LUMINANCE;
	key_printf("Led_Key_ChangeLight = %d\r\n",Luminance);
	libzled_setMaxLuminance(Luminance);
	if(LED_WORK_MODE_COLOR==g_config_param.led_work_mode.mode){
		libzled_setConstColor(g_config_param.led_work_mode.color_mode.color[0],\
							  g_config_param.led_work_mode.color_mode.color[1],\
							  g_config_param.led_work_mode.color_mode.color[2],
							  SEND_SOFT1_MODE);
	}
}






