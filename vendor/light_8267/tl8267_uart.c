/*
 * tl8267_uart.c
 *
 *  Created on: 2018-7-10
 *      Author: FN
 */
#include "../../proj/tl_common.h"
#include "../../proj/drivers/uart.h"

#include "stdarg.h"
#include "tl8267_uart.h"

///-------------printf 用到定义-----------------
#define			DECIMAL_OUTPUT		10
#define			OCTAL_OUTPUT		8
#define			HEX_OUTPUT			16

//#define va_start(ap,v)    (ap = (char *)((int)&v + sizeof(v)))
//#define va_arg(ap,t)      ((t *)(ap += sizeof(t)))[-1]
///---------------end--------------------

//MYFIFO_INIT(hci_rx_fifo, 72, 2);//2个fifo,每个FIFO 72个字节
//MYFIFO_INIT(hci_tx_fifo, 72, 2);//8个fifo,每个FIFO 72个字节

extern int rx_from_uart_cb (void);
extern int tx_to_uart_cb (void);

_attribute_ram_code_ static void uart_put_char(u8 byte)
{
	u8 char_buff[5] ={1,0,0,0,0};
	char_buff[4] = byte;
	while(!uart_Send(char_buff));
}

static int puts(char *s){
	while((*s != '\0')){
		uart_put_char(*s++);
	}
	return 0;
}

static void puti(unsigned int num, int base){
	char re[]="0123456789ABCDEF";

	char buf[50];

	char *addr = &buf[49];

	*addr = '\0';

	do{
		*--addr = re[num%base];
		num/=base;
	}while(num!=0);

	puts(addr);
}

int U1_printf(const char *format, ...){

	char span;
	unsigned long j;
	char *s;
	//char *msg;
	va_list arg_ptr;
	va_start(arg_ptr, format);

	while((span = *(format++))){
		if(span != '%'){
			uart_put_char(span);
		}else{
			span = *(format++);
			if(span == 'c'){
				j = va_arg(arg_ptr,int);//get value of char
				uart_put_char(j);
			}else if(span == 'd'){
				j = va_arg(arg_ptr,int);//get value of char
				if(j<0){
					uart_put_char('-');
					j = -j;
				}
				puti(j,DECIMAL_OUTPUT);
			}else if(span == 's'){
				s = va_arg(arg_ptr,char *);//get string value
				puts(s);
			}else if(span == 'o'){
				j = va_arg(arg_ptr,unsigned int);//get octal value
				puti(j,OCTAL_OUTPUT);
			}else if(span == 'x'){
					j = va_arg(arg_ptr,unsigned int);//get hex value
					puti(j,HEX_OUTPUT);
			}else if(span == 0){
				break;
			}else{
				uart_put_char(span);
			}
		}

	}
	va_end(arg_ptr);

	return 0;
}
void U1_putchar(u8 byte)
{
	uart_put_char(byte);
}

