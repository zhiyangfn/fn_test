/*
 * libscenes.h
 *
 *  Created on: 2018-3-6
 *      Author: iHoment-MQ
 */

#ifndef LIBSCENES_H_
#define LIBSCENES_H_

#include "../../proj/tl_common.h"

typedef enum _scenes
{
	SCENE_SUNRISE 	=0,
	SCENE_SUNSET	=1,
	SCENE_NIGHT_LAMP=2,
	SCENE_PARTY		=3,
	SCENE_MOVIE		=4,
	SCENE_DATING	=5,
	SCENE_COMFORT	=6,
	SCENE_ROMANCE	=7,
	SCENE_BLINK		=8,
	SCENE_CANDLE	=9,
	SCENE_BREATHE	=10,
	SCENE_COLORFUL	=11,
	SCENE_VIVID		=12,
	SCENE_READING	=13,
	SCENE_EASINESS	=14,

}scenes;

void libSceneLight_start(void);
void libSceneLight_stop();
u8 libSceneLight_work_thread(void);
void libSceneLight_set(u8 SceneMode);




#endif /* LIBSCENES_H_ */
