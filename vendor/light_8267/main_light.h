/*
 * main_light.h
 *
 *  Created on: 2018-7-13
 *      Author: FN
 */

#ifndef MAIN_LIGHT_H_
#define MAIN_LIGHT_H_
#include "../../proj/tl_common.h"
void	pwm_set_lum (int id, u16 y, int pol);
u32 get_pwm_cmp(u8 val, u8 lum);
void setLedRGBValue(u8 r_value,u8 g_value,u8 b_value);
//void light_adjust_RGB_hw(u8 val_R, u8 val_G, u8 val_B, u8 lum);
void setRGBSaveValue(u8 r_save,u8 g_save,u8 b_save);
void setCTValue(u8 color_tem);
u8 getLumValue(void);
void getRGBValue(u8 *RGB_buf);
#endif /* MAIN_LIGHT_H_ */
