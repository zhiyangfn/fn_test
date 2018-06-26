/*
 * libdiylight.h
 *
 *  Created on: 2018-6-20
 *      Author: FN
 */

#ifndef LIBDIYLIGHT_H_
#define LIBDIYLIGHT_H_

#include "libflashdata.h"
extern config_parameter_t g_config_param;


void libDiyLightStart(void);
void libDiyLightStop(void);
void libDiyModeWorkThread(void);
void diyModeDetailPrintf(void);

#endif /* LIBDIYLIGHT_H_ */
