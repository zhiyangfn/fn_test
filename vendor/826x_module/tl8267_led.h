#ifndef _LED_DRIVE_H
#define _LED_DRIVE_H

/////////////////////////////////////////////

#define G_COLOR_PIN		GPIO_PB4
#define R_COLOR_PIN		GPIO_PB6
#define B_COLOR_PIN		GPIO_PE0

#define G_COLOR_PWM_ID  PWM4_ID
#define R_COLOR_PWM_ID  PWM5_ID
#define B_COLOR_PWM_ID  PWM0_ID

void Init_PWM_RGB(u8 pwm_khz);
void RGB_set_color_value(u8 R , u8 G , u8 B ,u8 Luminance);
void RGB_set_fre_kHhz(u8 pwm_khz);
void RGB_set_fre_Hhz(u16 pwm_hz);
void test_pwm_app(void);


#define LIGHT_IC_NUM	60	//定义灯带IC数目
#define IIC_SLAVE_ADDR	0x66	//从机地址

typedef enum _iic_frame
{
	FRAME_SOF =0x3a,
	FRAME_TYPE_COLOR =0x01,
	FRAME_INSTANT =0x0,
	FRAME_WAIT_BCC =0x1,

}iic_frame;

typedef enum _iic_status
{
	BUSY_STATUS = 0x20,
	IDLE_STATUS,
	BCC_OK ,
	BCC_BAD ,
	BCC_CLEAR

}iic_status;

typedef enum _led_color_rank
{
	LED_COLOR_RGB =0,
	LED_COLOR_RBG ,
	LED_COLOR_GRB ,
	LED_COLOR_GBR ,
	LED_COLOR_BRG ,
	LED_COLOR_BGR ,
}led_color_rank;

void Init_IIC_RGB(void);
void I2c_set_color_rank(u8 rank);
u8 I2c_set_color_buf(u8 *color_data,u16 len,u8 mode);
void I2c_set_color_value(u8 R , u8 G , u8 B ,u8 Luminance,u8 mode);
void RGB_set_color_value(u8 R , u8 G , u8 B ,u8 Luminance);
unsigned char getAuxMCU_sw_version(unsigned char *pdata);
void test_light_ic_app(void);

#if 0
void Init_WS2811(void);
void WS2811_SendByte(char dat);//发送1BIT的数据
char GRB_send(char *GRB,short unsigned int len);
#endif



#endif

