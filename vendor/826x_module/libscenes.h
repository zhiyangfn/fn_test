#ifndef _LIB_SCENES_
#define _LIB_SCENES_


typedef enum _scenes
{
	SCENE_MORNING =0,
	SCENE_DUSK,
	SCENE_NIGHT_LAMP,
	SCENE_PARTY,
	SCENE_MOVIE,
	SCENE_DATING,
	SCENE_COMFORT,
	SCENE_ROMANCE,
	SCENE_BLINK,
	SCENE_CANDLE

}scenes;

void libSceneLight_start(void);
void libSceneLight_stop();
u8 libSceneLight_work_thread(void);
void libSceneLight_set(u8 SceneMode);
void libzled_stopChangeOfTime();
void led_set_change_para(u8 r,u8 g, u8 b,u8 luminance, u32 reach_time,u32 static_time);//just execute one
char led_gradual_change(void);


#endif
