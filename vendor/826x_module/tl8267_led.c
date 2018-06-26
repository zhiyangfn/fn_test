
#include "../../proj/tl_common.h"
#include "../../proj_lib/ble/blt_config.h"
#include "../../proj/drivers/i2c.h"

#include "tl8267_led.h"
#include "libzledsend.h"


static u16 pwm_cycle = 0;//
/*
 * @Desription:初始化light的PWM 模块
 * @param	:pwm_khz 单位为khz
 * @return 	:NULL
 */
void Init_PWM_RGB(u8 pwm_khz)
{
	pwm_cycle = (1000*CLOCK_SYS_CLOCK_1US)/pwm_khz;

	pwm_set_clk(CLOCK_SYS_CLOCK_HZ,CLOCK_SYS_CLOCK_HZ);//设计PWM的计数时钟

	//Green
	gpio_set_func(G_COLOR_PIN, AS_PWM);
	pwm_set_mode(G_COLOR_PWM_ID, PWM_NORMAL_MODE);
	pwm_set_phase(G_COLOR_PWM_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(G_COLOR_PWM_ID, pwm_cycle,  0 );
//	pwm_revert(G_COLOR_PWM_ID);//PWM取反
	pwm_start(G_COLOR_PWM_ID);

	//Red
	gpio_set_func(R_COLOR_PIN, AS_PWM);
	pwm_set_mode(R_COLOR_PWM_ID, PWM_NORMAL_MODE);
	pwm_set_phase(R_COLOR_PWM_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(R_COLOR_PWM_ID, pwm_cycle,  0 );
//	pwm_revert(R_COLOR_PWM_ID);//PWM取反
	pwm_start(R_COLOR_PWM_ID);

	//Blue
	gpio_set_func(B_COLOR_PIN, AS_PWM);
	pwm_set_mode(B_COLOR_PWM_ID, PWM_NORMAL_MODE);
	pwm_set_phase(B_COLOR_PWM_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(B_COLOR_PWM_ID, pwm_cycle,  0 );
//	pwm_revert(B_COLOR_PWM_ID);//PWM取反
	pwm_start(B_COLOR_PWM_ID);

//	RGB_set_fre_Hhz(20);
}
/*
 * @Desription: 设定RGB pwm frequency
 * @param	:pwm_khz 单位为khz 1~
 * @return 	:NULL
 */
void RGB_set_fre_kHhz(u8 pwm_khz)
{
	pwm_cycle = (u16)(1000*CLOCK_SYS_CLOCK_1US)/pwm_khz;
	pwm_set_clk(CLOCK_SYS_CLOCK_HZ,CLOCK_SYS_CLOCK_HZ);//设计PWM的计数时钟

	pwm_set_cycle(R_COLOR_PWM_ID,pwm_cycle);
	pwm_set_cycle(G_COLOR_PWM_ID,pwm_cycle);
	pwm_set_cycle(B_COLOR_PWM_ID,pwm_cycle);

}

/*
 * @Desription: 设定RGB pwm frequency
 * @param	:pwm_khz 单位为hz 10~1000
 * @return 	:NULL
 */
void RGB_set_fre_Hhz(u16 pwm_hz)
{
	pwm_cycle = (u16)(500000/pwm_hz);
	pwm_set_clk(CLOCK_SYS_CLOCK_HZ,100000);//设计PWM的计数时钟10000 = 500k计数

	pwm_set_cycle(R_COLOR_PWM_ID,pwm_cycle);
	pwm_set_cycle(G_COLOR_PWM_ID,pwm_cycle);
	pwm_set_cycle(B_COLOR_PWM_ID,pwm_cycle);

}

/*
 * @Desription:设定RGB颜色
 * @param	:0~255
 * @return 	:
 */
void RGB_set_color_value(u8 R , u8 G , u8 B ,u8 Luminance)
{
	R = R*(u16)Luminance/255;
	G = G*(u16)Luminance/255;
	B = B*(u16)Luminance/255;

	//gm_printf("RGB Color:%d-%d-%d\r\n",R,G,B);
	pwm_set_cmp(R_COLOR_PWM_ID,(R*pwm_cycle)/255);
	pwm_set_cmp(G_COLOR_PWM_ID,(G*pwm_cycle)/255);
	pwm_set_cmp(B_COLOR_PWM_ID,(B*pwm_cycle)/255);
}

#if 0
static u8 green=100,red=50,blue=210;
static u16 t_ms;
static u32 taskTick;
void test_pwm_app(void)
{
	if(clock_time_exceed(taskTick, 20000)){
		taskTick = clock_time();

		t_ms++;
		if(t_ms<=100)
		{
			green++;
			red--;
			blue++;
		}
		else if(t_ms>100 && t_ms<=150)
		{
			green++;
			red++;
			blue-=2;
		}
		else if(t_ms>150 && t_ms<=250)
		{
			green--;
			red-=2;
			blue--;
		}
		else
			t_ms=0;

		RGB_set_color_value(green,red ,blue,100);

	}

}
#endif


unsigned char iic_data[LIGHT_IC_NUM*3+6]={11,12,13,14,15,16,17,18,19,20};//用来模拟iic寄存器数据的存放

//#define I2C_GPIO_GROUP_A3A4		 0
#define IIC_SLAVE_ADDR 			0x66

#define MCU_STATUS_PIN		GPIO_PB7
#define READ_MCU_STATUS		 gpio_read(MCU_STATUS_PIN)

static u8 f_AllGroup = 0;//
static u8 f_LedColorRank = LED_COLOR_GRB;//
void Init_IIC_RGB(void)
{
	i2c_pin_init(I2C_GPIO_GROUP_A3A4);
		//800khz
	i2c_master_init_div(IIC_SLAVE_ADDR,10);//para1:ID;para2:DivClock,i2c clock = system_clock/4*DivClock

	gpio_set_func(MCU_STATUS_PIN, AS_GPIO);
	gpio_set_output_en(MCU_STATUS_PIN, 0);//disable output
	gpio_set_input_en(MCU_STATUS_PIN, 1); //enable input
	gpio_setup_up_down_resistor(MCU_STATUS_PIN, PM_PIN_PULLUP_10K);  //open pull up resistor

}

/*
 * @Desription: setting color rank, rgb or grb
 * @param	:  LED_COLOR_GRB ,LED_COLOR_RGB ...
 * @return 	:
 */

void I2c_set_color_rank(u8 rank)
{
	f_LedColorRank = rank;
}

/*
 * @Desription:
 * @param	:
 * @return 	: status
 */
 
u8 I2c_set_color_buf(u8 *color_data,u16 len,u8 mode)
{
	u16 i =0,data_len=0;
	u8 check_sum=0;
	u16 waitTime=0;
	data_len=len;

	if(mode == FRAME_WAIT_BCC){
		while(READ_MCU_STATUS){
			waitTime++;
			sleep_us(10);
			if(waitTime>700)
				return BUSY_STATUS;
		}
	}
	else{
		if(READ_MCU_STATUS)
			return BUSY_STATUS;
	}


	iic_data[0]=FRAME_SOF;//header
	iic_data[1]=FRAME_TYPE_COLOR;//type
	iic_data[2]=(u8)(data_len>>8);//length
	iic_data[3]=(u8)(data_len);//length

//	memcpy(iic_data+4,color_data,data_len);
	//rank rgb --> grb or gbr ...
	if(f_LedColorRank == LED_COLOR_GRB){
		for(i =0;i<data_len; i+=3 ){
			iic_data[4+i+0] =color_data[i+1];
			iic_data[4+i+1] =color_data[i+0];
			iic_data[4+i+2] =color_data[i+2];
		}
	}
	else{
		for(i =0;i<data_len; i+=3 ){
			iic_data[4+i+0] =color_data[i+0];
			iic_data[4+i+1] =color_data[i+1];
			iic_data[4+i+2] =color_data[i+2];
		}
	}

	for(i=0;i<(data_len+4);i++){
		check_sum ^= iic_data[i];
	}
	iic_data[data_len+4]=check_sum;//数据尾

	i2c_write_dma(4, 2, iic_data, data_len+5);//

	if(mode == FRAME_WAIT_BCC)
	{
		while(!READ_MCU_STATUS)
		{
			waitTime++;
			sleep_us(5);
			if(waitTime>500)
				return BCC_BAD;
		}
	}
	return BCC_OK;
}

/*
 * @Desription:设定RGB颜色
 * @param	:0~255
 * @return 	:
 */
void I2c_set_color_value(u8 R , u8 G , u8 B ,u8 Luminance,u8 mode)
{

	R = R*(u16)Luminance/255;
	G = G*(u16)Luminance/255;
	B = B*(u16)Luminance/255;
	f_AllGroup = libzled_get();//

	u8 rgb_buff[f_AllGroup*3];

	for(u8 i=0;i<f_AllGroup;i++)
	{
		rgb_buff[i*3+0] = R;
		rgb_buff[i*3+1] = G;
		rgb_buff[i*3+2] = B;
	}
	luminance_limit(rgb_buff,f_AllGroup*3);
	I2c_set_color_buf(rgb_buff,f_AllGroup*3,mode);
}

unsigned char getAuxMCU_sw_version(unsigned char *pdata)
{
	if(READ_MCU_STATUS)
	{
		return BUSY_STATUS;
	}
	i2c_read_dma(1, 2, pdata, 3);
	return 0;
}



#if 0
/*
 * @Desription:测试用IIC与MCU通信
 * @param	:
 * @return 	:
 */
static u8 green=100,red=50,blue=210;
static u16 t_ms;
static u32 taskTick;
void test_light_ic_app(void)
{
	static u8 _color=0;
	u8 RGB_data[LIGHT_IC_NUM*3];
	if(clock_time_exceed(taskTick, 20000)){
		taskTick = clock_time();

		t_ms++;
		if(t_ms<100)
		{
			green= 0xff;
			red =0;
			blue =0;
		}
		else if(t_ms>=100 && t_ms<200)
		{
			green= 0;
			red =0xff;
			blue =0;
		}
		else if(t_ms>=200 && t_ms<300)
		{
			green= 0;
			red =0;
			blue =0xff;
		}
		else
			t_ms=0;

		_color+=1;
		if(_color >=LIGHT_IC_NUM)
			_color=0;
		for(u8 i=0;i<LIGHT_IC_NUM;i++)
		{
			if(_color == i)
			{
				RGB_data[i*3+0] = green;
				RGB_data[i*3+1] = red;
				RGB_data[i*3+2] = blue;
			}
			else
			{
				RGB_data[i*3+0] = 0;
				RGB_data[i*3+1] = 0;
				RGB_data[i*3+2] = 0;
			}
		}

		u8 iic_sta =I2c_set_color_buf(RGB_data,LIGHT_IC_NUM*3,FRAME_WAIT_BCC);
		if(iic_sta != BCC_OK){
			gm_printf("iic_sta=%x\r\n",iic_sta);
		}
//		getAuxMCU_sw_version(RGB_data);
//		gm_printf("Version =%x %x %x\r\n",RGB_data[0],RGB_data[1],RGB_data[2]);



	}
}

#endif


#if 0
#define WS2811_Pin	GPIO_PA1//定义灯带的输出IO

/*
 * @Desription:初始化WS2811的引脚
 * @param	:
 * @return 	:
 */
void Init_WS2811(void)
{
	gpio_set_func(WS2811_Pin, AS_GPIO);
	gpio_set_output_en(WS2811_Pin, 1);//enable output
	gpio_set_input_en(WS2811_Pin, 0); //disable input
	gpio_write(WS2811_Pin, 0);
}


#if (CLOCK_SYS_CLOCK_HZ == 16000000)
	//16Mhz 灯
	#define SET_BIT1 	{gpio_write(WS2811_Pin, 1);_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;\
						gpio_write(WS2811_Pin, 0);_ASM_NOP_;_ASM_NOP_; }
	#define SET_BIT0	{gpio_write(WS2811_Pin, 1);\
						gpio_write(WS2811_Pin, 0);}
#elif (CLOCK_SYS_CLOCK_HZ == 32000000)

#endif
/*
 * @Desription:
 * @param	:
 * @return 	:
 */
_attribute_ram_code_ void WS2811_SendByte(char dat)//发送1BIT的数据
{
#if (CLOCK_SYS_CLOCK_HZ == 16000000)
	char bit[8] = {0};
	bit[0]=(dat>>0)&0x01;
	bit[1]=(dat>>1)&0x01;
	bit[2]=(dat>>2)&0x01;
	bit[3]=(dat>>3)&0x01;
	bit[4]=(dat>>4)&0x01;
	bit[5]=(dat>>5)&0x01;
	bit[6]=(dat>>6)&0x01;
	bit[7]=(dat>>7)&0x01;

	(bit[7]!=0)?(SET_BIT1):(SET_BIT0);
	(bit[6]!=0)?(SET_BIT1):(SET_BIT0);
	(bit[5]!=0)?(SET_BIT1):(SET_BIT0);
	(bit[4]!=0)?(SET_BIT1):(SET_BIT0);
	(bit[3]!=0)?(SET_BIT1):(SET_BIT0);
	(bit[2]!=0)?(SET_BIT1):(SET_BIT0);
	(bit[1]!=0)?(SET_BIT1):(SET_BIT0);
	(bit[0]!=0)?(SET_BIT1):(SET_BIT0);

#elif  (CLOCK_SYS_CLOCK_HZ == 32000000)
	u8 i=8;
	while(i--)
	{
		if(dat & 0x80) //发送数据1
		{
			gpio_write(WS2811_Pin, 1);
			_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;
			_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;

			gpio_write(WS2811_Pin, 0);
			_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;
		}
		else//发送数据0
		{
			gpio_write(WS2811_Pin, 1);
			_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;

			gpio_write(WS2811_Pin, 0);
			_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;
		}
		 dat <<= 1;
	}
#endif
}

/*
 * @Desription:
 * @param	:
 * @return 	:0-success  1-fail,cause len%3!=0
 */
char GRB_send(char *GRB,short unsigned int len){
	if((len%3)!=0)
		return 1;
	short unsigned int cnt;
	for(cnt=0;cnt<len;cnt+=3){
		WS2811_SendByte((char)(GRB[cnt]));//G
		WS2811_SendByte((char)(GRB[cnt+1]));//R
		WS2811_SendByte((char)(GRB[cnt+2]));//B
	}
	return 0;
}

#endif


