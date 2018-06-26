#ifndef _KEY_HANDLE_H
#define _KEY_HANDLE_H

////////////
//key GPIO
#define KEY_OPEN	GPIO_PB5//
#define KEY_COLOR	GPIO_PB1//
#define KEY_MODE	GPIO_PA7//

#define KEY_OPEN_VALUE		 gpio_read(KEY_OPEN)
#define KEY_COLOR_VALUE		 gpio_read(KEY_COLOR)
#define KEY_MODE_VALUE		 gpio_read(KEY_MODE)

typedef enum _KEY_HANDLE_
{
	NOTHING=0,
	OPEN_TAP=1<<0,
	OPEN_PRESS_HOLD=1<<1,
	OPEN_PRESS_SINGLE=1<<2,
	OPEN_PRESS_MULTI=1<<3,

	COLOR_TAP=1<<4,
	COLOR_PRESS_HOLD=1<<5,
	COLOR_PRESS_SINGLE=1<<6,
	COLOR_PRESS_MULTI=1<<7,

	MODE_TAP=1<<8,
	MODE_PRESS_HOLD=1<<9,
	MODE_PRESS_SINGLE=1<<10,
	MODE_PRESS_MULTI=1<<11,
	OPEN_COMBINE_COLOR =1<<12,

}key_handle;

typedef enum _color_rainbow_
{
	COLD_WHITE,
	WARM_WHITE,
	RED,
	ORANGE,
	YLLOW,
	GREEN,
	BLUE,
	CYAN,
	PURPLE,
}color_enum;


void Init_key(void);
u8 Scan_KeyLow_Value(void);
void Key_Handle(void);

void timer_irq_proc(void);
void test_key_app(void);

#endif
