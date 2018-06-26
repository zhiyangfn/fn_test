#ifndef _AMICAUDIO_H
#define _AMICAUDIO_H
#pragma once
#include "../../proj/tl_common.h"



typedef enum _amic_gain
{
	level_1 =1,
	level_2 ,
	level_3 ,
	level_4 ,
	level_5 ,
	level_6 ,
	level_7 ,
	level_8
}amic_gain;
/*********************************************************/

//function declaration
void Init_amicAudio(void);
void getAMIC_data(short int *pdata ,u16 len);
void setAMIC_gain_level(amic_gain level);
void getAMIC_data_int(int *pdata ,u16 len);



#endif

