#include "../../proj/tl_common.h"
#include "../../proj_lib/ble/blt_config.h"

#include "tl8267_uart.h"
#include "libflashdata.h"
#include "libMicLight.h"
#include "tl8267_led.h"
#include "tl8267_audio.h"
#include "libzledsend.h"

typedef struct audio_info{
	u8 musicMode;
	u8 color_num;//add by fn in 20180623 to diy mode
	u8 led_num;
	u8 mainRGB[3];
}audio_info;
audio_info musicInfo;

static u32 taskTick = 0;
static u8 gpthreadruing = 0;
static int audioBuf[128];
//static u8 rgb_store[180];
//static u8 rgb[180];
static u8 diy_rgb[4][3];
static u8 rgb[64*4];
extern u8 gGain;
#if 0
static u8 gEnergyInited = 0;
void energy(int *fft_data)
{
	int i;
	int r,g,b;
	//64 1-10 11-30 31-64
	r=0;g=0;b=0;
	for(i=1;i<10;i++){
		r+=fft_data[i];
	}
	r/=10;
	for(i=10;i<30;i++){
		g+=fft_data[i];
	}
	g/=20;
	for(i=30;i<63;i++){
		b+=fft_data[i];
	}
	b/=33;
	if(r > 255)r = 255;
	if(g > 255)g = 255;
	if(b > 255)b = 255;
	for(i=0;i<60;i++){
		rgb[i*3] = i<g? 0xFF:0;
		rgb[i*3+1] = i<r? 0xFF:0;
		rgb[i*3+2] = i<b? 0xFF:0;
	}
	if(gEnergyInited==0){
		gEnergyInited = 1;
		memcpy(rgb_store,rgb,180);
	}else{
		for(i=0;i<180;i++){
			if(rgb[i] > rgb_store[i]){
				rgb_store[i] = rgb[i];
			}else{
				if(rgb_store[i] > 10)rgb_store[i]-=10;
				else if(rgb_store[i] > 0)rgb_store[i]-=1;
			}
		}
	}
	I2c_set_color_buf(rgb_store,180,0);
	U1_printf("energy :%d %d %d \r\n",r,g,b);
}
void spectrum(int* fft_data)
{
	int i;
	for(i=0;i<60;i++){
		fft_data[i]/=255;
		if(fft_data[i] > 255)fft_data[i]= 255;
		rgb[i*3] = (u8)fft_data[i];
		rgb[i*3+1] = (u8)fft_data[i];
		rgb[i*3+2] = (u8)fft_data[i];
	}
	I2c_set_color_buf(rgb_store,180,0);
	U1_printf("spectrum\r\n");
}
void scroll(int* fft_data)
{
	int i;
	for(i=0;i<60;i++){
		fft_data[i]/=255;
		if(fft_data[i] > 255)fft_data[i]= 255;
		rgb[i*3] = (u8)fft_data[i];
		rgb[i*3+1] = 0;//(u8)fft_data[i];
		rgb[i*3+2] = 0;//(u8)fft_data[i];
	}
	I2c_set_color_buf(rgb_store,180,0);
	U1_printf("scroll\r\n");
}
void beat1(int* fft_data)
{
	int i;
	for(i=0;i<60;i++){
		fft_data[i]/=255;
		if(fft_data[i] > 255)fft_data[i]= 255;
		rgb[i*3] = 0;//(u8)fft_data[i];
		rgb[i*3+1] = 0;//(u8)fft_data[i];
		rgb[i*3+2] = (u8)fft_data[i];
	}
	I2c_set_color_buf(rgb_store,180,0);
	U1_printf("beat1\r\n");
}
#define BEAT_TH	10000
#define ONE_SENTENCE 15
//64 / 10 == 6
const int triangle_filter[9] = {1,25,50,75,100,75,50,25,1};
int melBank[10];
int melBank_store[10];
u8 gBeatInited = 0;
static u8 colorIndex = 0;
static u8 r,g,b;
const static u8 colorTab[3*7] = {0xFF,0x00,0x00, 0xFF,128,0x0, 0xFF,0xFF,0x0, 0x0,0xFF,0x0, 0x0,0xFF,0xFF, 0x0,0x0,0xFF, 128,0x0,0xFF};
void beat2(int *data)
{
	//闂備礁鎼粔鏉懨洪妶澶婇棷妞ゆ牜鍋為弲顒佹叏濮楋拷椤ゅ倿寮抽妷鈺傜厽婵犲﹤鍟幖鎰版煏閸パ勫枠鐎规洘绻堥弫鎾绘晸閿燂拷+闂備胶鍘ч崢鏍垝濞嗘挸闂柣鎴灻欢鐐垫喐瀹ュ棙鍙忛柨鐕傛嫹
	//濠碘槅鍋嗘晶妤呭垂閻熼偊鍟呭┑鍌氭啞閺咁剚鎱ㄥ鍡楀幐缂侇喖鐏氶幈銊︾節閸屾稑娅ｉ梺娲荤厜閹凤拷-闂備線娼уΛ妤咁敄閸曨厾绠旈柛灞惧焹閺嬪酣鏌嶉妷銊ョ毢闁荤喆鍨藉鍫曞煛閸屾粍鍣銈嗗姇閵堟悂骞嗛崟顓犵煓闁割煈鍠楅锟�
	u8 i;
	static int switch_color_cnt = 0;
	static int switch_color_cnt2 = 0;
	u8 r,g,b;
	int err;
	u8 bBeat = 0;
	//int *melBank_store = (int*)rgb_store;
	for(i=0;i<10;i++)melBank[i]=0;
	for(i=0;i<9;i++){
		melBank[0]+=triangle_filter[i]*data[i]/300;
	}
	for(i=0;i<9;i++){
		melBank[1]+=triangle_filter[i]*data[i+6]/300;
	}
	for(i=0;i<9;i++){
		melBank[2]+=triangle_filter[i]*data[i+2*6]/300;
	}
	for(i=0;i<9;i++){
		melBank[3]+=triangle_filter[i]*data[i+3*6]/300;
	}
	for(i=0;i<9;i++){
		melBank[4]+=triangle_filter[i]*data[i+4*6]/300;
	}
	for(i=0;i<9;i++){
		melBank[5]+=triangle_filter[i]*data[i+4*6]/300;
	}
	for(i=0;i<9;i++){
		melBank[6]+=triangle_filter[i]*data[i+4*6]/300;
	}
	for(i=0;i<9;i++){
		melBank[7]+=triangle_filter[i]*data[i+4*6]/300;
	}
	for(i=0;i<9;i++){
		melBank[8]+=triangle_filter[i]*data[i+4*6]/300;
	}
	for(i=0;i<9;i++){
		melBank[9]+=triangle_filter[i]*data[i+4*6]/300;
	}
	if(gBeatInited == 0){
		gBeatInited = 1;
		for(i=0;i<10;i++){
			melBank_store[i] = melBank[i];
		}
	}else{
		err = 0;
		for(i=0;i<10;i++){
			err += melBank_store[i] > melBank[i]? (melBank_store[i]-melBank[i]):(melBank[i]-melBank_store[i]) ;
		}
		U1_printf("err=%d\r\n",err);
		if(err > BEAT_TH)bBeat = 1;
	}
	switch_color_cnt++;
	if(bBeat == 1){
		if(switch_color_cnt > 100 || switch_color_cnt2 > ONE_SENTENCE){
			switch_color_cnt = 0;
			switch_color_cnt2 = 0;
			colorIndex ++;
			if(colorIndex == 7)colorIndex = 0;
		}
		r = colorTab[colorIndex*3];
		g = colorTab[colorIndex*3+1];
		b = colorTab[colorIndex*3+2];
		switch_color_cnt2 = 0;
	}else{
		switch_color_cnt2++;
		if(r > 20)r -=20;
		else{
			if(r > 0)r --;
		}
		if(g > 20)g -=20;
		else{
			if(g > 0)g --;
		}
		if(b > 20)b -=20;
		else{
			if(b > 0)b --;
		}
	}

	/*for(i=0;i<9;i++){
		U1_printf("%d ",melBank[i]);
	}
	U1_printf("\r\n");
	*/
	RGB_set_color_value(r,g,b,100);
}
#endif



//typedef unsigned char u8;
//typedef unsigned short u16;
//typedef unsigned int u32;
//typedef int s32;

#define MAX_IC_NUM	60
#define ENERGY_SUB_NUM	10
#define SPECTRUM_SUB_NUM	10
#define SCROLL_SPEED	5
#define BEAT_SUB_NUM	15
#define BEAT_TH 	2000
#define TIME_TO_SWITCH_COLOR	200
#define ONE_SENTENCE	15
#define PWM_MODE	0
#define IC_MODE		1
#define FLIP_COLOR_FRAME 50

//static u8 rgb_store[MAX_IC_NUM/2];//for scroll is enough
static u8 rgb_store[32*4];//for beat is enough

const static u8 triangle_filter[9] = {1,25,50,75,100,75,50,25,1};
const static u8 gf[5] = {6,30,50,30,6};
const static u8 gf_h[5] = {1,5,110,5,1};

//_attribute_custom_code_
const static u8 rgbTab[3*7] = {	0xFF,0x00,0x00,
								0xFF,128,0x0, 
								0xFF,0xFF,0x0, 
								0x0,0xFF,0x0,
								0x0,0x0,0xFF,
								0x0,0xFF,0xFF, 
								128,0x0,0xFF
							 };


static void makeEnergy(s32 r,s32 g,s32 b,u8* rgb,s32 ic_num)
{
	int i;
	unsigned char rr,gg,bb;
	if(ic_num < 5)return;

	rr = (r*ic_num+128)/255;
	gg = (g*ic_num+128)/255;
	bb = (b*ic_num+128)/255;
#if 0
	for(i=ic_num;i<(2*ic_num);i++){
		rgb[i*3] 	= (i-ic_num) < rr ? 0xFF:0;
		rgb[i*3+1] 	= (i-ic_num) < gg ? 0xFF:0;
		rgb[i*3+2] 	= (i-ic_num) < bb ? 0xFF:0;
		rgb[rr*3] = 128;
		rgb[gg*3+1] = 128;
		rgb[bb*3+2] = 128;
	}
	for(i=0;i<ic_num;i++){
		rgb[i*3] = rgb[(2*ic_num-1-i)*3];
		rgb[i*3+1] = rgb[(2*ic_num-1-i)*3+1];
		rgb[i*3+2] = rgb[(2*ic_num-1-i)*3+2];
	}
#else
	rgb_store[0] = 0xFF;
	rgb_store[1] = 0xFF;
	rgb_store[ic_num+2] = 0;
	rgb_store[ic_num+3] = 0;
	//r
	for(i=2;i<(2+ic_num);i++){
		rgb_store[i] = (i-2) < rr ? 0xFF:0;
	}
	for(i=2;i<(2+ic_num);i++){
		r = gf[0]*rgb_store[i-2] + gf[1]*rgb_store[i-1]+gf[2]*rgb_store[i]+gf[3]*rgb_store[i+1]+gf[4]*rgb_store[i+2];
		rgb_store[i] = (u8)(r/122);	
	}
	//debug(rgb_store,ic_num+4);
	for(i=ic_num;i<(2*ic_num);i++){
		rgb[i*3] = rgb_store[2+i-ic_num];
	}
	//g
	for(i=2;i<(2+ic_num);i++){
		rgb_store[i] = (i-2) < gg ? 0xFF:0;
	}
	for(i=2;i<(2+ic_num);i++){
		r = gf[0]*rgb_store[i-2] + gf[1]*rgb_store[i-1]+gf[2]*rgb_store[i]+gf[3]*rgb_store[i+1]+gf[4]*rgb_store[i+2];
		rgb_store[i] = (u8)(r/122);	
	}
	for(i=ic_num;i<(2*ic_num);i++){
		rgb[i*3+1] = rgb_store[2+i-ic_num];
	}
	//b
	for(i=2;i<(2+ic_num);i++){
		rgb_store[i] = (i-2) < bb ? 0xFF:0;
	}
	for(i=2;i<(2+ic_num);i++){
		r = gf[0]*rgb_store[i-2] + gf[1]*rgb_store[i-1]+gf[2]*rgb_store[i]+gf[3]*rgb_store[i+1]+gf[4]*rgb_store[i+2];
		rgb_store[i] = (u8)(r/122);	
	}
	for(i=ic_num;i<(2*ic_num);i++){
		rgb[i*3+2] = rgb_store[2+i-ic_num];
	}
	for(i=0;i<ic_num;i++){
		rgb[i*3] = rgb[(2*ic_num-1-i)*3];
		rgb[i*3+1] = rgb[(2*ic_num-1-i)*3+1];
		rgb[i*3+2] = rgb[(2*ic_num-1-i)*3+2];
	}
#endif	
}
static void makeScroll(s32 low,s32 mid,s32 high,u8* rgb,s32 ic_num,u8 *mainColor)
{
	static s32 i;

	//U1_printf("%d\t\t%d \t\t%d\r\n",low,mid,high);
	//鍚戝墠婊氬姩
	for(i=(ic_num-1);i>0;i--){
		rgb_store[i] = rgb_store[i-1];
	}
#if 0
	for(i=0;i<ic_num/10;i++){
		//U1_printf("%d\n",i);
		//if((u8)low > rgb_store[i])
		rgb_store[i] = (u8)high;
		//else {
		//	if(rgb_store[i] > 3)rgb_store[i]-=3;else if(rgb_store[i]>0)rgb_store[i]--;
		//}
	}
	//printf("\n");
	for(i=ic_num/10;i<ic_num/5;i++){
		//printf("%d\n",i);
		rgb_store[i] = (u8)mid;
	}
	//printf("\n");
	for(i=ic_num/5;i<ic_num/3;i++){
		//printf("%d\n",i);
		rgb_store[i] = (u8)low;
	}
#else
	for(i=0;i<ic_num/9;i++){
		//U1_printf("%d\n",i);
		rgb_store[i] = (u8)low;
	}
#endif
	for(i=0;i<ic_num;i++){
		rgb[(i+ic_num)*3] = rgb_store[i]* mainColor[0];
		rgb[(i+ic_num)*3+1] = rgb_store[i]* mainColor[1];
		rgb[(i+ic_num)*3+2] = rgb_store[i]* mainColor[2];
	}
	for(i=0;i<ic_num;i++){
		rgb[i*3] = rgb[(2*ic_num-i-1)*3];
		rgb[i*3+1] = rgb[(2*ic_num-i-1)*3+1];
		rgb[i*3+2] = rgb[(2*ic_num-i-1)*3+2];
	}
}
#if 0
static void make10Mel2LED(int* mel10,u8* rgb,int ic_num,u8 *mainColor)
{
	int i,j;
	static u8 gSpectrumInited = 0;
	int *melBank_store = (int*)rgb_store;
	int ic_per_mel = ic_num*2/10;

	if(gSpectrumInited == (u8)0){
		gSpectrumInited = 1;
		for(i=0;i<10;i++){
			melBank_store[i] = mel10[i];
		}
	}else{
		for(i=0;i<10;i++){
			if(mel10[i] > melBank_store[i])melBank_store[i] = mel10[i];
			if(melBank_store[i] > SPECTRUM_SUM_NUM){
				melBank_store[i] -= SPECTRUM_SUM_NUM;
			}else{
				if(melBank_store[i] > 0)melBank_store[i]--;
			}
			if(melBank_store[i] > 255)melBank_store[i] = 255;
			for(j=0;j<ic_per_mel;j++){
				rgb[(i*ic_per_mel+j)*3]=(u8)melBank_store[i]*mainColor[0]/255;
				rgb[(i*ic_per_mel+j)*3+1]=(u8)melBank_store[i]*mainColor[1]/255;
				rgb[(i*ic_per_mel+j)*3+2]=(u8)melBank_store[i]*mainColor[2]/255;
			}
		}
	}
}
#endif

void energy(s32* fft_data,u8* rgb,s32 ic_num)
{
 	static int ADJ = 100;
	static u8 adj_cnt = 0;
	static int adj_sum=0;
	static int aa = 0,bb = 0,cc=0;
	static u8 flip_color_cnt=0;
	static u8 flip = 0;
	int i,a,b,c;

	a = 0;
	b = 0;
	c = 0;
	
	for(i=1;i<10;i++){a+=fft_data[i];}//鏉╂瑩鍣烽弰顖炴付鐟曚礁鍟�閺�閫涚閺�鍦畱閿涘苯鎸ㄩ崗鑸垫Ц闂囷拷鐟曚礁濮為崗銉╁櫢閸欑姷娈戦崠鍝勭厵
	for(i=10;i<26;i++){b+=fft_data[i];}
	for(i=26;i<63;i++){c+=fft_data[i];}
	
/*
	for(i=1;i<5;i++){a+=fft_data[i];}//鏉╂瑩鍣烽弰顖炴付鐟曚礁鍟�閺�閫涚閺�鍦畱閿涘苯鎸ㄩ崗鑸垫Ц闂囷拷鐟曚礁濮為崗銉╁櫢閸欑姷娈戦崠鍝勭厵
	for(i=5;i<16;i++){b+=fft_data[i];}
	for(i=16;i<63;i++){c+=fft_data[i];}
*/	a/=(ADJ*2);
	b/=(ADJ*2);
	c/=ADJ;
	adj_cnt++;
	adj_sum+=(a+b+c);
	if(adj_cnt == (u8)50){
		adj_sum/=50;
		if(adj_sum > 600){
			ADJ*=2;
		}
		if(adj_sum < 100){
			if(ADJ > 10)ADJ/=2;
			if(ADJ < 8)ADJ = 8;
		}
		U1_printf("sum=%d\tADJ=%d\r\n",adj_sum,ADJ);
		adj_cnt = 0;
		adj_sum = 0;
	}
	
	if(a > 255)a=255;
	if(b > 255)b=255;
	if(c > 255)c=255;
	
	if(a > aa)aa = a;
	if(b > bb)bb = b;
	if(c > cc)cc = c;

	if(aa > 6*ENERGY_SUB_NUM)aa-=ENERGY_SUB_NUM;else if(aa > ENERGY_SUB_NUM)aa-=ENERGY_SUB_NUM/3;else if(aa > 0)aa--;
	if(bb > 6*ENERGY_SUB_NUM)bb-=ENERGY_SUB_NUM;else if(bb > ENERGY_SUB_NUM)bb-=ENERGY_SUB_NUM/3;else if(bb > 0)bb--;
	if(cc > 6*ENERGY_SUB_NUM)cc-=ENERGY_SUB_NUM;else if(cc > ENERGY_SUB_NUM)cc-=ENERGY_SUB_NUM/3;else if(cc > 0)cc--;
	
	//makeEnergy(bb,cc,aa,rgb,ic_num);

	flip_color_cnt++;
	if(flip_color_cnt == FLIP_COLOR_FRAME){
		flip_color_cnt=0;
		flip++;
		if(flip == 3)flip = 0;
	}
	if(flip == 0){
		makeEnergy(aa,bb,cc,rgb,ic_num);
	}else if(flip == 1){
		makeEnergy(cc,aa,bb,rgb,ic_num);
	}else if(flip == 2){
		makeEnergy(bb,cc,aa,rgb,ic_num);
	}
	libzled_send(rgb,ic_num*6,0);
}
void scroll(s32* fft_data,u8* rgb,s32 ic_num,u8* mainColor)
{
	static int ADJ = 100;
	static int adj_cnt = 0;
	static int adj_sum=0;
	int i,a,b,c;
	a = 0;
	b = 0;
	c = 0;
	for(i=1;i<11;i++){a+=fft_data[i];}//鏉╂瑩鍣烽弰顖炴付鐟曚礁鍟�閺�閫涚閺�鍦畱閿涘苯鎸ㄩ崗鑸垫Ц闂囷拷鐟曚礁濮為崗銉╁櫢閸欑姷娈戦崠鍝勭厵
	for(i=9;i<23;i++){b+=fft_data[i];}
	for(i=20;i<63;i++){c+=fft_data[i];}
	a/=ADJ;
	b/=ADJ;
	c/=ADJ;
	adj_cnt++;
	adj_sum+=a;//(a+b+c);
	if(adj_cnt == 50){
		//adj_sum/=50;
		if(adj_sum > 50){//500
			ADJ*=2;
		}
		if(adj_sum < 5){//60
			if(ADJ > 1)ADJ/=2;
		}
		if(gGain == 8)if(ADJ < 800)ADJ=800;
		if(gGain == 7)if(ADJ < 200)ADJ=200;

		U1_printf("sum=%d\tADJ=%d\r\n",adj_sum,ADJ);
		adj_cnt = 0;
		adj_sum = 0;
	}

	if(a > 255)a=255;
	if(b > 255)b=255;
	if(c > 255)c=255;

	makeScroll(a,b,c,rgb,ic_num,mainColor);
	libzled_send(rgb,ic_num*6,0);
}

#if 0
void beat_fft(s32* fft_data,u8* rgb,s32 ic_num,u8* colorTab,u8 mode)
{
	static u8 gBeatInited = 0;
	int i;
	int sum = 0;
	int diff = 0;
	int *lastFFT = (int*)rgb_store;
	if(gBeatInited == (u8)0){
		gBeatInited = 1;
		for(i=0;i<ic_num;i++)lastFFT[i] = 0;
	}
	for(i=0;i<ic_num;i++){
		sum+=fft_data[i];
		diff+= (lastFFT[i] > fft_data[i] ? (lastFFT[i] - fft_data[i]):(fft_data[i] - lastFFT[i]));
		lastFFT[i] = fft_data[i];
		//U1_printf("%d\t%d\r\n",sum,diff);
	}
	U1_printf("sum = %d\tdiff = %d\tsum/dif=%d\r\n",sum,diff,sum/diff);
}

void beat_fft(s32* fft_data,u8* rgb,s32 ic_num,u8* colorTab,u8 mode)
{
	u8 i;
	static u8 gBeatInited = 0;
	static u32 bBeat_last = 0;
	static u8 colorIndex = 0;
	int melBank[10];
	int err;

	u8 bBeat = 0;
	u8 mayBeat = 0;
	int *melBank_store = (int*)rgb_store;
	for(i=0;i<10;i++)melBank[i]=0;
	for(i=0;i<9;i++){
		melBank[0]+=triangle_filter[i]*fft_data[i]/300;
	}
	for(i=0;i<9;i++){
		melBank[1]+=triangle_filter[i]*fft_data[i+6]/300;
	}
	for(i=0;i<9;i++){
		melBank[2]+=triangle_filter[i]*fft_data[i+2*6]/300;
	}
	for(i=0;i<9;i++){
		melBank[3]+=triangle_filter[i]*fft_data[i+3*6]/300;
	}
	for(i=0;i<9;i++){
		melBank[4]+=triangle_filter[i]*fft_data[i+4*6]/300;
	}
	for(i=0;i<9;i++){
		melBank[5]+=triangle_filter[i]*fft_data[i+4*6]/300;
	}
	for(i=0;i<9;i++){
		melBank[6]+=triangle_filter[i]*fft_data[i+4*6]/300;
	}
	for(i=0;i<9;i++){
		melBank[7]+=triangle_filter[i]*fft_data[i+4*6]/300;
	}
	for(i=0;i<9;i++){
		melBank[8]+=triangle_filter[i]*fft_data[i+4*6]/300;
	}
	for(i=0;i<9;i++){
		melBank[9]+=triangle_filter[i]*fft_data[i+4*6]/300;
	}
	if(gBeatInited == (u8)0){
		gBeatInited = 1;
		for(i=0;i<10;i++){
			melBank_store[i] = melBank[i];
		}
	}else{
		err = 0;
		for(i=0;i<10;i++){
			err += melBank_store[i] > melBank[i]? (melBank_store[i]-melBank[i]):(melBank[i]-melBank_store[i]) ;
		}
		//U1_printf("err=%d\r\n",err);
		if(err > BEAT_TH){
			mayBeat = 1;
		 	if((bBeat_last & 0x0000000F) == 0){//鏉╂瑩鍣烽崘鍐茬暰娴滃棗顦跨亸鎴濇姎娑擄拷0閹靛秴鍘戠拋姝屽Ν婵傦拷
				bBeat = 1;
				if(mode == 1){
					colorIndex++;if(colorIndex==7)colorIndex=0;
					libzled_setConstColor(colorTab[colorIndex*3],colorTab[colorIndex*3+1],colorTab[colorIndex*3+2],0);
				}else{
					libzled_setConstColor(musicInfo.mainRGB[0],musicInfo.mainRGB[1],musicInfo.mainRGB[2],0);
				}
				U1_printf("err=%d index =%d r%d g%d b%d\r\n",err,colorIndex,colorTab[colorIndex*3],colorTab[colorIndex*3+1],colorTab[colorIndex*3+2]);
			}
		}
		bBeat_last <<=1;
		if(mayBeat)bBeat_last |= 1;
	}
}
#else
void beat_pcm(s32* pcm_data,u8* rgb,s32 ic_num,u8* colorTab,u8 mode)
{
	static int sum_100 = 0;
	static int cnt_100 = 0;
	static int avg_last[3];
	static int avg_100;
	static int th = 20;
	static int energy;
	static u8 colorIndex = 0;
	static u16 cnt_switch_color = 0;
	static u16 cnt_wait_in_silence = 0;
	int avg = 0;
	int i;
	//get avg
	for(i=0;i<128;i++){avg += pcm_data[i] > 0 ? pcm_data[i]:-pcm_data[i];}
	avg /= 128;
	//get avg_100
	cnt_100++;sum_100+=avg;
	if(cnt_100 == 100){
		avg_100 = sum_100/100;
		th = avg_100/5;
		if(th < 200)th = 200;
		if(gGain == 7)if(th <400)th=400;
		if(gGain == 8)if(th <600)th=500;
		cnt_100 = 0;
		sum_100 = 0;
		U1_printf("avg100 = %d th = %d\r\n",avg_100,th);
	}
	cnt_switch_color++;
	//get diff
	if(avg - avg_last[0] > th && avg - avg_last[1] > th && avg - avg_last[2] > th){

		U1_printf("beat: th=%d avg=%d cnt_wait_in_silence=%d\r\n",th,avg,cnt_wait_in_silence);
		if(cnt_switch_color > TIME_TO_SWITCH_COLOR || cnt_wait_in_silence > ONE_SENTENCE){
			cnt_switch_color = 0;
			colorIndex++;if(colorIndex==musicInfo.color_num)colorIndex=0;
			U1_printf("color = %d\r\n",colorIndex);
		}
		cnt_wait_in_silence = 0;
		energy = 255;
	}else{
		cnt_wait_in_silence++;
	}
	if(energy > BEAT_SUB_NUM)energy-=BEAT_SUB_NUM;else if(energy > 0)energy--;
	//U1_printf("energy=%d\r\n",energy);
	if(mode == 1){
		libzled_setConstColor(colorTab[colorIndex*3]*energy/255,colorTab[colorIndex*3+1]*energy/255,colorTab[colorIndex*3+2]*energy/255,0);
	}else{
		libzled_setConstColor(musicInfo.mainRGB[0]*energy/255,musicInfo.mainRGB[1]*energy/255,musicInfo.mainRGB[2]*energy/255,0);
	}
	avg_last[2] = avg_last[1];
	avg_last[1] = avg_last[0];
	avg_last[0] = avg;
	//printf("avg=%d\n",avg);
}
#endif
#if 0
void spectrum(s32* fft_data,u8* rgb,s32 ic_num,u8* mainColor)
{
	static int ADJ = 1;
	int i;
	int melBank[10];
	int sum = 0;

	for(i=0;i<10;i++)melBank[i]=0;
	for(i=0;i<9;i++){
		melBank[0]+=triangle_filter[i]*fft_data[i]/300;
	}
	for(i=0;i<9;i++){
		melBank[1]+=triangle_filter[i]*fft_data[i+6]/300;
	}
	for(i=0;i<9;i++){
		melBank[2]+=triangle_filter[i]*fft_data[i+2*6]/300;
	}
	for(i=0;i<9;i++){
		melBank[3]+=triangle_filter[i]*fft_data[i+3*6]/300;
	}
	for(i=0;i<9;i++){
		melBank[4]+=triangle_filter[i]*fft_data[i+4*6]/300;
	}
	for(i=0;i<9;i++){
		melBank[5]+=triangle_filter[i]*fft_data[i+5*6]/300;
	}
	for(i=0;i<9;i++){
		melBank[6]+=triangle_filter[i]*fft_data[i+6*6]/300;
	}
	for(i=0;i<9;i++){
		melBank[7]+=triangle_filter[i]*fft_data[i+7*6]/300;
	}
	for(i=0;i<9;i++){
		melBank[8]+=triangle_filter[i]*fft_data[i+8*6]/300;
	}
	for(i=0;i<9;i++){
		melBank[9]+=triangle_filter[i]*fft_data[i+9*6]/300;
	}

	for(i=0;i<10;i++){melBank[i] /= ADJ;sum +=melBank[i];}
	if(sum > 255*6){ADJ*=2;if(ADJ > 10000)ADJ = 10000;}
	if(sum < 255){ADJ/=2;if(ADJ < 1)ADJ = 1;}

	U1_printf("sum=%d ADJ=%d\n",sum,ADJ);
	for(i=0;i<10;i++){
		U1_printf("%d ",melBank[i]);
	}
	U1_printf("\r\n");
	make10Mel2LED(melBank,rgb,ic_num,mainColor);
	libzled_send(rgb,180,0);
}
#else
void spectrum(s32* fft_data,u8* rgb,s32 ic_num,u8* mainColor)
{
	static int ADJ = 1;
	static u8 adj_cnt = 0;
	static u32 adj_sum = 0;
	static u8 gSpectrumInited = 0;
	int i,sum=0;

	if(gSpectrumInited == 0){
		gSpectrumInited = 1;
		for(i=0;i<ic_num;i++){
			rgb_store[i] = 0;
		}
	}
	for(i=0;i<ic_num;i++){
		fft_data[i] /= ADJ;
		if(fft_data[i] > 255)fft_data[i]=255;
		sum +=fft_data[i];
	}
	sum/=ic_num;

	adj_sum+=sum;
	adj_cnt++;
	if(adj_cnt==100){
		//adj_sum/=100;
		if(adj_sum > 16000)ADJ*=2;
		if(adj_sum < 3000){ADJ/=2;if(ADJ==0)ADJ=1;}
		U1_printf("sum = %d ADJ=%d\r\n",adj_sum,ADJ);	
		adj_sum = 0;
		adj_cnt = 0;
	}
	//鐢变簬缁嗙矑搴︿笉澶燂紝闄ゆ帀鍚庨潰鐨勯鐜�
	/*for(i=0;i<ic_num;i++){
		fft_data[i] = fft_data[2*i] + fft_data[2*i+1]; 
		fft_data[i]/=2;	
	}*/
	for(i=2;i<ic_num;i++){
		fft_data[i] = gf_h[0]*fft_data[i-2] + gf_h[1]*fft_data[i-1] +gf_h[2]*fft_data[i] +gf_h[3]*fft_data[i+1] +gf_h[4]*fft_data[i+2]; 
		fft_data[i]/=122;
		if(fft_data[i] > 255)fft_data[i]=255;
	}
#if 1	
	for(i=0;i<ic_num;i++){
		if((u8)fft_data[i] > rgb_store[i]){
			rgb_store[i] = fft_data[i];
		}else{
			if(rgb_store[i] > SPECTRUM_SUB_NUM)rgb_store[i]-=SPECTRUM_SUB_NUM;else if(rgb_store[i] > 0)rgb_store[i]--;
		}
	}
	fft_data[0] = mainColor[0];
	fft_data[1] = mainColor[1];
	fft_data[2] = mainColor[2];
	
	for(i=0;i<ic_num;i++){
		rgb[(i+ic_num)*3] 	= fft_data[0]*rgb_store[i]/255;
		rgb[(i+ic_num)*3+1] = fft_data[1]*rgb_store[i]/255;
		rgb[(i+ic_num)*3+2] = fft_data[2]*rgb_store[i]/255;
	}
#else
	for(i=0;i<ic_num;i++){
		rgb[(i+ic_num)*3] 	= fft_data[0]*mainColor[i]/255;
		rgb[(i+ic_num)*3+1] = fft_data[1]*mainColor[i]/255;
		rgb[(i+ic_num)*3+2] = fft_data[2]*mainColor[i]/255;
	}
#endif	
	for(i=0;i<ic_num;i++){
		rgb[i*3] = rgb[(2*ic_num-1-i)*3];
		rgb[i*3+1] = rgb[(2*ic_num-1-i)*3+1];
		rgb[i*3+2] = rgb[(2*ic_num-1-i)*3+2];
	}
	libzled_send(rgb,ic_num*6,0);
}
#endif
void libMicLight_set(u8 musicMode,u8 sensitiveLevel,u8 color_num,u8 led_num,unsigned char* rgb)
{
	musicInfo.musicMode = musicMode;
	musicInfo.color_num=color_num;
	musicInfo.led_num = led_num/2;
	musicInfo.mainRGB[0] = rgb[0];
	musicInfo.mainRGB[1] = rgb[1];
	musicInfo.mainRGB[2] = rgb[2];
	memcpy(diy_rgb,rgb,3*color_num);
}

void libMicLight_stop()
{
	gpthreadruing = 0;
}

void libMicLight_start(void)
{
	gpthreadruing = 1;
}
u8 libMicLight_work_thread(void)
{
	u8 i;
	static u8 fft_filter_cnt = 0;
	int* audioBuf_avg3 = (int*)rgb;//浣跨敤rgb鍖哄煙 淇濆瓨64涓暟鎹�
	//8k/128 = 62 = 16ms
	if(clock_time_exceed(taskTick, 16000)){
		taskTick = clock_time();
		if(gpthreadruing){
			getAMIC_data_int(audioBuf ,128);
			if(musicInfo.musicMode == 0){//energy
				zfft(audioBuf);
				for(i=0;i<64;i++)audioBuf_avg3[i] += audioBuf[i];
				fft_filter_cnt++;
				if(fft_filter_cnt == 2){
					for(i=0;i<64;i++){audioBuf[i] = audioBuf_avg3[i]/2;}
					energy(audioBuf,rgb,musicInfo.led_num);
					for(i=0;i<64;i++){audioBuf_avg3[i] = 0;}
					fft_filter_cnt = 0;
				}
				//U1_printf("energy music mode\r\n");
			}else if (musicInfo.musicMode == 1){//spectrum
				zfft(audioBuf);
				spectrum(audioBuf,rgb,musicInfo.led_num,musicInfo.mainRGB);
				//U1_printf("spectrum mode\r\n");
			}else if (musicInfo.musicMode == 3){//rhythm
				beat_pcm(audioBuf,rgb,musicInfo.led_num,rgbTab,1);
				//zfft(audioBuf);
				//beat_fft(audioBuf,rgb,30,rgbTab,1);
				//U1_printf("beat music mode\r\n");
			}else if (musicInfo.musicMode == 2){//rolling
				zfft(audioBuf);
				scroll(audioBuf,rgb,musicInfo.led_num,musicInfo.mainRGB);
				//U1_printf("scroll music mode\r\n");
			}else if (musicInfo.musicMode == 100){//PWM 7 color
				//zfft(audioBuf);
				beat_pcm(audioBuf,rgb,musicInfo.led_num,rgbTab,1);
				//U1_printf("PWM music mode 1\r\n");
			}else if (musicInfo.musicMode == 101){//PWM constant color
				//zfft(audioBuf);
				beat_pcm(audioBuf,rgb,musicInfo.led_num,rgbTab,0);
				//U1_printf("PWM music mode 2\r\n");
			}else if(musicInfo.musicMode == 13){//diy rhythm mode,add by fn in 20180623
				beat_pcm(audioBuf,rgb,musicInfo.led_num,diy_rgb,1);
			}
		}
	}
}



