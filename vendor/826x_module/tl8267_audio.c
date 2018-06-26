#include "../../proj/tl_common.h"
#include "../common/tl_audio.h"
#include "../../proj/drivers/audio.h"
#include "../../proj/drivers/pga.h"

#include "tl8267_audio.h"

/*
 * @Desription: 初始化amic音频采集
 * @param	:
 * @return 	:
 */
void Init_amicAudio(void){
	//buffer_mic set must before audio_init !!!
	config_mic_buffer((u32)buffer_mic, TL_MIC_BUFFER_SIZE);
	//////////////// AMIC:PC4/PC5 - input
	
	// system_clock/(misc*4 + left*16 + fine)/d_samp  其中R2就是2
	audio_amic_init( DIFF_MODE, 45, 20, R8, CLOCK_SYS_TYPE); 
	audio_amic_input_set(PGA_CH);//audio input set, ignore the input parameter

}

/*
 * @Desription: 获取指定长度的音频数据,
 * @param	:
 * @return 	:
 * @other	: 由于采样一次存四个字节，所以
 */
 
void getAMIC_data(short int *pdata ,u16 len)
{
	u16 w_ptr=0;
	u16 lack_ptr;

	w_ptr = reg_audio_wr_ptr<<1;		// reg_audio_wr_ptr 0~127	

	if(w_ptr > len )
	{
		memcpy(pdata ,buffer_mic + (w_ptr - len), len<<1 );
	}
	else
	{
		lack_ptr =len - w_ptr;
		memcpy( pdata , buffer_mic + ((TL_MIC_BUFFER_SIZE>>1)-lack_ptr) ,lack_ptr<<1);
		memcpy( pdata + lack_ptr, buffer_mic ,w_ptr<<1);
	}
}



/*
 * @Desription: Adjust amic input Gain
 * @param	:level_1~level_8
 * @return 	:
 * @other	:
 */
void setAMIC_gain_level(amic_gain level)
{
	enum PREAMPValue preGain;
	enum POSTAMPValue posGain;

	switch(level)
	{
		case	level_1:
			preGain = DBP0;
			posGain = DB0;
			break;

		case	level_2:
			preGain = DBP0;
			posGain = DB3;
			break;

		case	level_3:
			preGain = DBP0;
			posGain = DB6;
			break;

		case	level_4:
			preGain = DBP0;
			posGain = DB9;
			break;

		case	level_5:
			preGain = DB20;
			posGain = DB0;
			break;

		case	level_6:
			preGain = DB20;
			posGain = DB3;
			break;

		case	level_7:
			preGain = DB20;
			posGain = DB6;
			break;

		case	level_8:
			preGain = DB20;
			posGain = DB9;
			break;
		default:
			preGain = DB20;
			posGain = DB0;
			break;
	}
	preGainAdjust(preGain); //set pre pga gain to 20db
	postGainAdjust(posGain); //set post pga gain to 0db
}
/*
get data to int32 buffer
*/
#define AGC_ON	1
#if AGC_ON
static int sum_100 = 0;
static int cnt_100 = 0;
u8 gGain = 8;
#define AGC_TIME_TO_BIGGER	800
#define AGC_TIME_TO_SMALLER	8000
#endif
void getAMIC_data_int(int *pdata ,u16 len)
{
	u16 w_ptr=0;
	u16 lack_ptr;
	u16 i;
	s16* ptr;
	w_ptr = reg_audio_wr_ptr<<1;

	if(w_ptr > len )
	{
		ptr = buffer_mic+ (w_ptr - len);
		for(i=0;i<len;i++){
			pdata[i] = (int)(*ptr);
			ptr++;
		}
	}
	else
	{
		lack_ptr =len - w_ptr;
		ptr = buffer_mic+ (TL_MIC_BUFFER_SIZE>>1) - (lack_ptr);
		for(i=0;i<lack_ptr;i++){
			pdata[i] = *ptr;
			ptr++;
		}
		ptr = buffer_mic;
		for(i=0;i<w_ptr;i++){
			pdata[i] = *ptr;
			ptr++;
		}
	}
#if AGC_ON
	int sum = 0;
	for(i=0;i<len;i++){
		sum+= pdata[i]>0?pdata[i]:-pdata[i];
	}
	sum/=100;
	sum_100+=sum;
	cnt_100++;
	if(cnt_100 == 50){
		sum_100/=50;
		if(sum_100 > AGC_TIME_TO_SMALLER){
			if(gGain > 1){
				gGain--;
				setAMIC_gain_level(gGain);
				gm_printf("sum100 = %d gain=%d\r\n",sum_100,gGain);
			}
		}
		if(sum_100 < AGC_TIME_TO_BIGGER){
			if(gGain < 8){
				gGain++;
				setAMIC_gain_level(gGain);
				gm_printf("sum100 = %d gain=%d\r\n",sum_100,gGain);
			}
		}
		cnt_100 = 0;
		sum_100 = 0;
	}
#endif	
}
#if 0
int gSum_100 = 0;
int gGainTest = 1;

void testWithPython(short* pdata)
{
	int sum = 0;
	static int sum_100 = 0;
	static int cnt_100 = 0;
	int i;
	getAMIC_data(pdata,100);
	for(i=0;i<100;i++){
		sum+= pdata[i]>0?pdata[i]:-pdata[i];
	}
	sum/=100;
	sum_100+=sum;
	cnt_100++;
	if(cnt_100 == 50){
		sum_100/=50;
		if(sum_100 > 2000){
			if(gGainTest > 1){
				gGainTest--;
				setAMIC_gain_level(gGainTest);
			}
		}
		if(sum_100 < 500){
			if(gGainTest < 8){
				gGainTest++;
				setAMIC_gain_level(gGainTest);
			}
		}

		gSum_100 = sum_100;
		U1_printf("sum100 = %d gain=%d\r\n",sum_100,gGainTest);
		cnt_100 = 0;
		sum_100 = 0;
	}
#if 0	
	U1_putchar(0xaa);
	U1_putchar(0xaa);
	for(i=0;i<(100);i+=1)
	{
		U1_putchar((pdata[i]>>8));
		U1_putchar((pdata[i]&0x00ff));
	}
#endif	
}
#endif
