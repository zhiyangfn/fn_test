/*
 * libworkthread.h
 *
 *  Created on: 2018-7-17
 *      Author: FN
 */

#ifndef LIBWORKTHREAD_H_
#define LIBWORKTHREAD_H_
#include "../../proj/tl_common.h"

void masterWorkThread(void);
void setWorkStatus(u8 cmd,u8 *params,u8 params_len);
void getNotifyData(u8 *get_notify_data_buf);

enum _status_station_{
	DEFEAULT_MODE,
	SCENE_MODE,
	DIY_MODE
};

typedef struct _config_parameter_t
{
	u8 cmd;
	u8 param_len;
	u8 set_param_buf[20];
}config_parameter_t;


#endif /* LIBWORKTHREAD_H_ */
