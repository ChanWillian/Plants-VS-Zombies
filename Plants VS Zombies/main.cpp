#include <stdio.h>
#include <graphics.h> //need install easyx
#include <time.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#include "math.h"
#include "tools.h"
#include "vector2.h"

#define WIN_WIDTH 900 
#define WIN_HEIGHT 600
#define ZM_MAX	10

enum { PEN, SUNFLOWER, PLANTS_COUNT };	//植物的两个种类
enum { SBALL_DOWN, SBALL_GROUND, SBALL_COLLECT, SBALL_PRODUCT };	//阳光的四种运动状态
enum { GOING, WIN, FAIL };	//游戏的三种状态

IMAGE imgBg;
IMAGE imgBar;
IMAGE imgCards[PLANTS_COUNT];
IMAGE* imgPlants[PLANTS_COUNT][20];	//Learn
IMAGE imgSunshineBall[29];
IMAGE imgZombie[22];
IMAGE imgZmDead[20];
IMAGE imgZmEat[21];
IMAGE imgZmStand[11];
IMAGE imgBulletNormal;
IMAGE imgBulletBlast[4];

int imgSBCount = 29;
int imgZCount = 22;
int imgZDCount = 20;
int imgZSCount = 11;
int imgZECount = 21;
int imgBBCount = 4;

int killZCount;	//the count of killed zombie
int createZCount; //the count of created zombie
int gameStatus;	//game status

int curX, curY;			//the coordinates of the selected plant during movement
int curPlant = -1;		//the index of the selected plant;
//-1: no plants are selected; 0: the 0th plants is selected
int sunshineSum = 150;	//initial sunline value

struct plant {
	int x, y;
	int type;		//-1: no plants are planted; 0: the 0th plant is planted
	int frameIndex;	//The sequence number of the currently displayed picture frame;
	//Constantly change images to achieve animation effects
	int blood;		//植物的血量
	int timer;		//产生阳光的计时器
};
struct plant pMap[3][9];	//Lawn map（Plants can be grown）

struct sunshineBall {
	int x, y;		//阳光球在飘落过程中的坐标（X坐标始终不变）
	int frameIndex;	//The sequence number of the currently displayed picture frame
	bool used;		//Whether the sunshine ball is in use
	int timer;		//the effective time of the sunshine ball
	float t;				//阳光球移动总用时 0 ~ 1
	vector2 p1, p2, p3, p4;	//贝塞尔曲线控制节点
	vector2 pCur;			//阳光球当前的位置
	float recNumMove;		//阳光球移动次数的倒数
	int status;				//阳光球当前的运动状态
};
struct sunshineBall balls[10];//阳光球池（类似于线程池）

struct zombie {
	int x, y;
	int frameIndex;	//僵尸图片帧序号
	bool used;	//僵尸被使用状态
	int speed;	//僵尸移动速度
	int row;	//僵尸所在行号
	int blood;	//僵尸的血量
	bool dead;	//僵尸的死亡状态
	bool eat;	//僵尸吃植物的状态
};
struct zombie zms[10];

struct bullet {
	int x, y;
	int row;	//子弹所在行号
	bool used;	//子弹被使用状态
	int speed;
	bool blast;		//子弹爆炸状态
	int frameIndex;	//子弹爆炸图片帧序号
};
struct bullet bullets[30];

int ballCount = sizeof(balls) / sizeof(balls[0]);	//可用的阳光球总数
int zmCount = sizeof(zms) / sizeof(zms[0]);			//可用的僵尸总数
int bltCount = sizeof(bullets) / sizeof(bullets[0]);//可用的植物子弹总数

char imgUrl[64]; //图片路径变量

/// <summary>
/// determine whether the file exists
/// </summary>
/// <param name="imgUrl">file path</param>
/// <returns></returns>
bool fileExist(const char* imgUrl) {
	FILE* fp = fopen(imgUrl, "r");
	if (fp == NULL) {
		return false;
	}
	else {
		fclose(fp);
		return true;
	}
}
/// <summary>
/// init game data and game's image source
/// </summary>
void gameInit()
{
	srand(time(NULL));	//设置随机种子，用于随机生成数字

	//initgraph(WIN_WIDTH, WIN_HEIGHT, EX_SHOWCONSOLE);	//create game windows
	initgraph(WIN_WIDTH, WIN_HEIGHT);	//create game windows

	killZCount = 0;
	createZCount = 0;
	gameStatus = GOING;

	/*init font*/
	LOGFONT f;
	gettextstyle(&f);
	f.lfHeight = 30;
	strcpy(f.lfFaceName, "Segoe UI Black");
	f.lfQuality = ANTIALIASED_QUALITY;	//消除字体边缘锯齿
	settextstyle(&f);
	setbkmode(TRANSPARENT);
	setcolor(BLACK);

	/*init game data*/
	memset(imgPlants, 0, sizeof(imgPlants));	//全部初始化为0
	memset(pMap, -1, sizeof(pMap));				//全部初始化为-1
	memset(balls, 0, sizeof(balls));
	memset(zms, 0, sizeof(zms));
	memset(bullets, 0, sizeof(bullets));

	/*init background image*/
	loadimage(&imgBg, "res/bg.jpg");
	loadimage(&imgBar, "res/bar.png");

	/*init plant cards && init plant*/
	for (int i = 0; i < PLANTS_COUNT; i++) {
		sprintf_s(imgUrl, sizeof(imgUrl), "res/Cards/card_%d.png", i + 1);	//generate image url of plant card
		loadimage(&imgCards[i], imgUrl);									//init plant card's image

		for (int j = 0; j < 20; j++) {
			sprintf_s(imgUrl, sizeof(imgUrl), "res/zhiwu/%d/%d.png", i, j + 1);
			if (fileExist(imgUrl)) {	//若图片存在
				imgPlants[i][j] = new IMAGE;	//分配内存空间
				loadimage(imgPlants[i][j], imgUrl);
			}
			else break;
		}
	}

	//init sunshineBall image
	for (int i = 0; i < imgSBCount; i++) {
		sprintf_s(imgUrl, sizeof(imgUrl), "res/sunshine/%d.png", i + 1);
		loadimage(&imgSunshineBall[i], imgUrl);
	}

	//init zombies
	for (int i = 0; i < imgZCount; i++) {
		sprintf_s(imgUrl, sizeof(imgUrl), "res/zm/%d.png", i + 1);
		loadimage(&imgZombie[i], imgUrl);
	}

	//init plant's bullet
	loadimage(&imgBulletNormal, "res/bullets/bullet_normal.png");

	/*图片比例由小变大，实现子弹爆炸效果*/
	loadimage(&imgBulletBlast[3], "res/bullets/bullet_blast.png");
	for (int i = 0; i < 3; i++) {
		float k = (i + 1) * 0.2;
		loadimage(&imgBulletBlast[i], "res/bullets/bullet_blast.png",
			imgBulletBlast[3].getwidth() * k,
			imgBulletBlast[3].getheight() * k, true);
	}

	/*init zombie's dead image*/
	for (int i = 0; i < imgZDCount; i++) {
		sprintf_s(imgUrl, sizeof(imgUrl), "res/zm_dead/%d.png", i + 1);
		loadimage(&imgZmDead[i], imgUrl);
	}

	for (int i = 0; i < imgZECount; i++) {
		sprintf_s(imgUrl, sizeof(imgUrl), "res/zm_eat/%d.png", i + 1);
		loadimage(&imgZmEat[i], imgUrl);
	}

	/*init zombie_stand image*/
	for (int i = 0; i < imgZSCount; i++) {
		sprintf_s(imgUrl, sizeof(imgUrl), "res/zm_stand/%d.png", i + 1);
		loadimage(&imgZmStand[i], imgUrl);
	}

}
/// <summary>
/// render zombie screen
/// </summary>
void drawZombie()
{
	for (int i = 0; i < zmCount; i++)
		if (zms[i].used) { //若存在僵尸被使用
			int index = zms[i].frameIndex;
			IMAGE* img;
			if (zms[i].dead) { //若僵尸已死亡
				img = &imgZmDead[index];
			}
			else if (zms[i].eat) {
				img = &imgZmEat[index];
			}
			else {
				img = &imgZombie[index];
			}

			putimagePNG(
				zms[i].x,
				(zms[i].y - img->getheight()),
				img);
		}
}
/// <summary>
/// render sunshineBall screen
/// </summary>
void drawSBall() {
	for (int i = 0; i < ballCount; i++) {
		if (balls[i].used) {
			IMAGE* img = &imgSunshineBall[balls[i].frameIndex];
			putimagePNG(balls[i].pCur.x, balls[i].pCur.y, img);
		}
	}

	/*Render sunline value*/
	char scoreText[8];
	sprintf_s(scoreText, sizeof(scoreText), "%d", sunshineSum);
	outtextxy(168, 70, scoreText);
}
/// <summary>
/// render plantCard screen
/// </summary>
void drawPltCard() {
	/*Render plant card*/
	for (int i = 0; i < PLANTS_COUNT; i++) {
		int x = 226 + i * 65;
		int y = 6;
		putimage(x, y, &imgCards[i]);	//这个地方不懂；程序运行一段时间，第一张图片就会渲染失败；
	}

	/*Render the selected plant card*/
	if (curPlant != -1) //若存在植物被选中
	{
		IMAGE* img = imgPlants[curPlant][0];
		int widthOffset = img->getwidth() / 2;
		int heightOffset = img->getheight() / 2;
		putimagePNG(curX - widthOffset, curY - heightOffset, img);
	}
}
/// <summary>
/// render plant being grown
/// </summary>
void drawPlant() {
	/*Render the plant being grown*/
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			if (pMap[i][j].type != -1) //若存在植物被种植
			{
				int x = pMap[i][j].x;	//植物的X坐标
				int y = pMap[i][j].y;	//植物的Y坐标
				int num = pMap[i][j].type;			//植物索引
				int index = pMap[i][j].frameIndex;	//植物图片帧索引

				putimagePNG(x, y, imgPlants[num][index]);
			}
		}
	}
}
/// <summary>
/// render bullets of plant that can shoot
/// </summary>
void drawBullet() {
	/*Render plant's bullet*/
	for (int i = 0; i < bltCount; i++) {
		if (bullets[i].used) {
			if (bullets[i].blast) {
				IMAGE* img = &imgBulletBlast[bullets[i].frameIndex];//此种重新开辟内存空间的用法，可以让程序没有BUG
				putimagePNG(bullets[i].x, bullets[i].y, img);
			}
			else {
				IMAGE* img = &imgBulletNormal;
				putimagePNG(bullets[i].x, bullets[i].y, img);
			}
		}
	}
}
/// <summary>
/// update game screen
/// </summary>
void updateWindow()
{
	BeginBatchDraw();// start double-cache

	putimage(-112, 0, &imgBg);
	putimagePNG(140, 0, &imgBar);

	/*Render plant card*/
	/*Render the selected plant card*/
	drawPltCard();

	/*Render the plant being grown*/
	drawPlant();

	/*Render generated or clicked sunline balls*/
	/*Render sunline value*/
	drawSBall();

	/*Render Zombie*/
	drawZombie();

	/*Render plant's bullet*/
	drawBullet();

	EndBatchDraw();//end double-cache
}
/// <summary>
/// 响应用户点击阳光球的操作
/// </summary>
void collectSunshine(ExMessage* msg)
{
	for (int i = 0; i < ballCount; i++) {
		if (balls[i].used) {	//若存在阳光球被使用
			int x = balls[i].pCur.x;	//阳光球的X坐标
			int y = balls[i].pCur.y;	//阳光球的Y坐标

			if (msg->x > (x + 18) && msg->x < (x + 18) + 44
				&& msg->y >(y + 18) && msg->y < (y + 18) + 44) {	//若阳光球被点击

				//balls[i].used = false;				//修改阳光球使用状态
				balls[i].status = SBALL_COLLECT;	//修改阳光球的运动状态
				PlaySound("res/sunshine.wav", NULL, SND_FILENAME | SND_ASYNC);

				/*不为贝塞尔曲线设置控制节点，实现阳光球的直线运动*/
				balls[i].p1 = balls[i].pCur; //阳光球的起点
				balls[i].p4 = vector2(150, 0); //阳光球的终点
				balls[i].t = 0;
				float distance = dis(balls[i].p1 - balls[i].p4);	//两点间的距离
				float off = 8;	//阳光球移动步长
				balls[i].recNumMove = 1.0 / (distance / off); //阳光球移动次数的倒数
				// speed为阳光球移动频率；
				// s = v * t = (off * speed) * t; t=1时，s = off * speed * 1; speed * 1为阳光球移动次数；

				break;	//跳出本层循环
			}
		}
	}
}
/// <summary>
/// 响应用户点击操作
/// </summary>
void userClick() {
	ExMessage msg;
	static int status = 0;	//植物卡片选中状态
	if (peekmessage(&msg))
	{
		if (msg.message == WM_LBUTTONDOWN) //如果鼠标左键被按下
		{
			if ((msg.x > 226)
				&& (msg.x < 226 + 65 * PLANTS_COUNT)
				&& (msg.y > 7) && (msg.y < 96)) {	//如果存在植物卡片被点击
				int index = (msg.x - 226) / 65;	//计算出植物卡片的索引值
				status = 1;						//置植物卡片选中状态为真
				curPlant = index;				//至当前植物索引为当前植物卡片索引
			}
			else collectSunshine(&msg);
		}
		else if (status == 1 && msg.message == WM_MOUSEMOVE) //如果存在植物卡片被点击并且鼠标在移动
		{
			curX = msg.x;
			curY = msg.y;
		}
		else if (msg.message == WM_LBUTTONUP) {	//如果鼠标左键抬起
			if (status == 1) {	//若存在植物卡片被选中
				if (msg.x > 140 && msg.x < 876
					&& msg.y > 176 && msg.y < 489) {	//若植物卡片移动至植物地图区域

					int row = (msg.y - 176) / 102;
					int col = (msg.x - 140) / 81;

					if (pMap[row][col].type == -1) {	//若土壤未种植植物
						int price = 0;
						if (curPlant == PEN) {
							price = 100;
						}
						else if (curPlant == SUNFLOWER) {
							price = 50;
						}

						if (sunshineSum < price) {
							curPlant = -1;
							return;
						}
						else {
							sunshineSum -= price;
						}

						int x = 140 + col * 81;	//植物的X坐标
						int y = 176 + row * 102;	//植物的Y坐标

						memset(&pMap[row][col], 0, sizeof(pMap[row][col]));	//初始化植物属性
						pMap[row][col].x = x;
						pMap[row][col].y = y;
						pMap[row][col].type = curPlant;
						pMap[row][col].blood = 140;
					}
				}

				status = 0;				//置植物选中状态为假
				curPlant = -1;			//置当前植物索引为假
			}
		}
	}
}
/// <summary>
/// 持续更新植物地图
/// </summary>
void updateMap()
{
	//持续更换植物图片帧序号
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 9; j++) {
			int type = pMap[i][j].type;
			int index = pMap[i][j].frameIndex;

			if (type != -1) {	//若存在植物被种植
				index++;
				if (imgPlants[type][index] != NULL) pMap[i][j].frameIndex = index;
				else pMap[i][j].frameIndex = 0;
			}
		}
	}

}
/// <summary>
/// 定时产生阳光球
/// </summary>
void createSunshineBall()
{
	/*定时掉落阳光*/
	static int count = 0;
	static int fre = 128;
	count++;
	if (count >= fre)
	{
		fre = 100 + rand() % 40;
		count = 0;

		int i;
		for (i = 0; i < ballCount && balls[i].used; i++);	//遍历阳光池，跳过被使用的阳光球，寻找未被使用的阳光球
		if (i >= ballCount) return;

		/*设置阳光球的属性*/
		balls[i].used = true;
		balls[i].frameIndex = 0;
		balls[i].timer = 0;
		balls[i].status = SBALL_DOWN;
		balls[i].p1 = vector2(188 + rand() % (788 - 188), 60);
		balls[i].p4 = vector2(balls[i].p1.x, 200 + (rand() % 4) * 90);
		balls[i].t = 0;
		int off = 2;
		float distance = balls[i].p4.y - balls[i].p1.y;
		balls[i].recNumMove = 1.0 / (distance / off);
	}

	/*向日葵定时生成阳光*/
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 9; j++) {
			if (pMap[i][j].type == SUNFLOWER) { //若植物为向日葵

				pMap[i][j].timer++;
				if (pMap[i][j].timer > 256) { //若向日葵计时完成
					pMap[i][j].timer = 0;	//计时器清零

					int k;
					for (k = 0; k < ballCount && balls[k].used; k++);	//寻找未使用的阳光球
					if (k < ballCount) {
						balls[k].used = true;
						balls[k].status = SBALL_PRODUCT;
						balls[k].p1 = vector2(pMap[i][j].x, pMap[i][j].y);
						int w = (rand() % 2 ? 1 : -1) * (100 + rand() % 50);
						balls[k].p4 = vector2(pMap[i][j].x + w,
							pMap[i][j].y + imgPlants[SUNFLOWER][0]->getheight() - imgSunshineBall[0].getheight());
						balls[k].p2 = vector2(balls[k].p1.x + (w * 0.3), balls[k].p1.y - 100); //贝塞尔曲线控制节点1
						balls[k].p3 = vector2(balls[k].p1.x + (w * 0.7), balls[k].p1.y - 100); //贝塞尔曲线控制节点2
						balls[k].recNumMove = 0.05;
						balls[k].t = 0;
					}
				}
			}
		}
	}
}
/// <summary>
/// 更新阳光球运动状况
/// </summary>
void updateSunshineBall()
{
	/*持续更新阳光球图片帧和坐标*/
	for (int i = 0; i < ballCount; i++) {
		if (balls[i].used) {	//如果存在阳光球被使用
			struct sunshineBall* ball = &balls[i];

			ball->frameIndex = (ball->frameIndex + 1) % 29;
			int status = ball->status;
			if (status == SBALL_DOWN) {	//若阳光球处于降落状态
				ball->pCur = ball->p1 + ball->t * (ball->p4 - ball->p1);	//更新阳光球当前位置
				ball->t += ball->recNumMove;	//更新阳光球移动总用时
				if (ball->t >= 1) {	//阳光球移动完毕
					ball->timer = 0;
					ball->status = SBALL_GROUND;
				}
			}
			else if (status == SBALL_GROUND) { //若阳光球处于落地状态
				ball->timer++;
				if (ball->timer >= 200) {
					ball->timer = 0;
					ball->used = false;
				}
			}
			else if (status == SBALL_COLLECT) {
				ball->t += ball->recNumMove;
				ball->pCur = ball->p1 + ball->t * (ball->p4 - ball->p1);
				if (ball->t >= 1) {
					ball->used = false;
					sunshineSum += 25;
				}
			}
			else if (status == SBALL_PRODUCT) {
				ball->t += ball->recNumMove;
				ball->pCur = calcBezierPoint(ball->t, ball->p1, ball->p2, ball->p3, ball->p4);
				if (ball->t >= 1) {
					ball->status = SBALL_GROUND;
					ball->timer = 0;
				}
			}
		}
	}
}
/// <summary>
/// 定时生成僵尸
/// </summary>
void createZombie() {
	if (createZCount >= ZM_MAX)
		return;

	static int count = 0;	//用于定时生成僵尸
	static int fre = 196;	//用于定时生成僵尸
	count++;

	if (count >= fre) {
		count = 0;
		fre = 256 + rand() % 64;

		int i;
		for (i = 0; i < zmCount && zms[i].used; i++);	//遍历僵尸数组，跳过被使用的僵尸，寻找未被使用的僵尸
		if (i > zmCount) return;

		/*init zombie data*/
		zms[i].used = true;
		zms[i].x = WIN_WIDTH - 100;
		zms[i].row = rand() % 3;	//设置僵尸的行号
		zms[i].y = 172 + (zms[i].row + 1) * 100;	//设置僵尸的Y坐标
		zms[i].speed = 3;
		zms[i].blood = 100;
		zms[i].dead = false;
		zms[i].eat = false;

		createZCount++;
	}

}
/// <summary>
/// 检测僵尸和植物的碰撞（植物：僵尸 = 1：n）
/// </summary>
void checkZm2Plt() {
	for (int i = 0; i < 3; i++) {

		for (int j = 0; j < 9; j++) {
			if (pMap[i][j].type == -1) continue;	//跳过某行中未种植植物的土壤，寻找已种植植物的土壤

			for (int k = 0; k < zmCount; k++) {
				if (zms[k].used == false || zms[k].dead) continue; //跳过未使用或已死亡的僵尸，寻找已使用且未死亡的僵尸

				int row = zms[k].row;	//置行号为僵尸所在行号

				if (i == row) { //若存在僵尸和植物在同一行
					int x = pMap[i][j].x;	//植物的X坐标
					//int x = 140 + j * 81;	
					int x1 = x + 10;
					int x2 = x + 60;
					int x3 = zms[k].x + 80;

					if (x3 < x2 && x3 > x1) { //若僵尸和植物发生碰撞
						pMap[i][j].blood -= 2;

						if (zms[k].eat == false) { //若僵尸吃植物状态为假
							zms[k].eat = true;
							zms[k].speed = 0;
							zms[k].frameIndex = 0;
						}

						if (pMap[i][j].blood <= 0) { //若植物已死亡
							pMap[i][j].type = -1;

							for (int i = 0; i < zmCount; i++) { //
								if (zms[i].row == row && zms[i].eat) {
									zms[i].eat = false;
									zms[i].speed = 3;
									zms[i].frameIndex = 0;
								}
							}

							break;	//跳出本次循环
						}
					}
				}
			}
		}
	}
}
/// <summary>
/// 更新子弹的运动状况
/// </summary>
void updateZombie() {
	/*定时更新僵尸X坐标*/
	static int count = 0;
	count++;
	if (count >= 3) {
		count = 0;
		for (int i = 0; i < zmCount; i++) {
			if (zms[i].used) {
				zms[i].x -= zms[i].speed;
				if (zms[i].x < 58) {
					gameStatus = FAIL;
				}
			}
		}
	}

	checkZm2Plt();//检测是否存在僵尸和植物发生碰撞

	/*定时更新僵尸的图片帧*/
	static int count_index = 0;
	count_index++;
	if (count_index >= 3) {
		count_index = 0;
		for (int i = 0; i < zmCount; i++) {
			if (zms[i].used) {	//若存在僵尸被使用
				if (zms[i].dead) {	//若僵尸已死亡
					zms[i].speed = 0;
					zms[i].frameIndex++;
					if (zms[i].frameIndex >= 20) { //若僵尸死亡图片帧大于等于20，即僵尸死亡动画播放完成
						zms[i].used = false;	//置僵尸使用状态为假
						killZCount++;
						if (killZCount >= ZM_MAX)
							gameStatus = WIN;
					}
				}
				else if (zms[i].eat) {	//若僵尸在吃植物
					zms[i].frameIndex = (zms[i].frameIndex + 1) % imgZECount;
				}
				else {	//若僵尸在行走
					zms[i].frameIndex = (zms[i].frameIndex + 1) % imgZCount;
				}
			}
		}
	}
}
/// <summary>
/// 定时产生子弹
/// </summary>
void createBullet()
{
	bool zmLines[3] = { 0 };	//某行僵尸存在状态：某行是否存在被使用且未死亡的僵尸
	int dangerX = WIN_WIDTH - imgZombie[0].getwidth();
	for (int i = 0; i < zmCount; i++) {
		if (zms[i].used
			&& zms[i].dead == false
			&& zms[i].x < dangerX) //若存在被使用、未死亡且进入植物射击范围的僵尸
			zmLines[zms[i].row] = true;	//置某行僵尸存在状态为真
	}

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 9; j++) {
			if (pMap[i][j].type == PEN && zmLines[i] == true) {	//若植物可射击，并且其所在行存在僵尸
				static int count = 0;
				count++;
				if (count > 40) {
					count = 0;

					int k;
					for (k = 0; k < bltCount && bullets[k].used; k++); //遍历子弹数组，跳过已被使用的子弹，寻找未被使用的子弹
					if (k < bltCount) {
						int pltX = 140 + j * 81;	//植物的X坐标
						int pltY = 176 + i * 102;	//植物的Y坐标

						/*初始化子弹*/
						bullets[k].used = true;
						bullets[k].x = pltX + 62;
						bullets[k].y = pltY + 10;
						bullets[k].row = i; //置子弹所在行号为植物所在行号
						bullets[k].speed = 10;

						bullets[k].blast = false;
						bullets[k].frameIndex = 0;
					}
				}
			}
		}
	}
}
/// <summary>
/// 检测子弹和僵尸的碰撞（子弹：僵尸 = 1：1）
/// </summary>
void checkBlt2Zm() {
	for (int i = 0; i < bltCount; i++) {
		if (bullets[i].used == false || bullets[i].blast == true) //跳过未使用或已爆炸的子弹，寻找已使用并且未爆炸的子弹
			continue;

		for (int j = 0; j < zmCount; j++) {
			if (zms[j].used == false || zms[j].dead)	//跳过未使用或已死亡的僵尸，寻找已使用且未死亡的僵尸
				continue;

			if (bullets[i].row == zms[j].row) { //若存在僵尸和植物子弹在同一列
				int x = bullets[i].x;
				int x1 = zms[j].x + 80;
				int x2 = zms[j].x + 110;

				if (x > x1 && x < x2) { //若存在子弹和僵尸发生碰撞
					bullets[i].blast = true;
					bullets[i].speed = 0;

					zms[j].blood -= 10;

					if (zms[j].blood <= 0) {	//若僵尸血量为0
						zms[j].dead = true;
						zms[j].speed = 0;
						zms[j].frameIndex = 0;	//置僵尸死亡图片帧序号为0
					}

					break;	//跳出本层循环
				}
			}
		}
	}

}
/// <summary>
/// 更新子弹的运动状态
/// </summary>
void updateBullet()
{
	for (int i = 0; i < bltCount; i++) {
		if (bullets[i].used == true) {	//若存在子弹被使用
			bullets[i].x += bullets[i].speed; //更新子弹的X坐标
			if (bullets[i].x > WIN_WIDTH) {
				bullets[i].used = false;
			}

			checkBlt2Zm(); //检测是否存在子弹和僵尸发生碰撞

			if (bullets[i].blast == true) {	//若存在子弹和僵尸发生碰撞
				bullets[i].frameIndex++;
				if (bullets[i].frameIndex >= imgBBCount) bullets[i].used = false;
			}
		}
	}
}
/// <summary>
/// update game data
/// </summary>
void updateGame()
{
	static int count = 0;
	count++;
	if (count >= 4) {
		count = 0;
		updateMap();//update plant map

		createSunshineBall();//create SunshineBall
		updateSunshineBall();//update SunshineBall

		createZombie();//create zombie
		createBullet();//create bullet

		updateBullet();
		updateZombie();//update zombie
	}
	updateSunshineBall();//update SunshineBall
}
/// <summary>
/// start screem of game
/// </summary>
void startUI()
{
	IMAGE imgBg, imgMenu1, imgMenu2;
	loadimage(&imgBg, "res/menu.png");
	loadimage(&imgMenu1, "res/menu1.png");
	loadimage(&imgMenu2, "res/menu2.png");
	int flag = 0;

	while (1) {
		BeginBatchDraw();
		putimagePNG(0, 0, &imgBg);
		putimagePNG(474, 75, flag ? &imgMenu2 : &imgMenu1);

		ExMessage msg;
		if (peekmessage(&msg)) {
			if (msg.message == WM_LBUTTONDOWN
				&& msg.x > 474 && msg.x < 474 + 300
				&& msg.y > 75 && msg.y < 75 + 140) {	//若鼠标点击菜单项
				flag = 1;
			}
			else if (msg.message == WM_LBUTTONUP && flag == 1) {
				EndBatchDraw();
				break;
			}
		}

		EndBatchDraw();
	}
}
/// <summary>
/// 场景调度
/// </summary>
void viewScene() {
	int xMin = -(imgBg.getwidth() - WIN_WIDTH); //- (1400 - 900) = -500
	vector2 points[9] = {
		{550, 80}, {530, 160}, {630, 170}, {530, 200}, {515, 270},
		{563, 370}, {605, 340}, {705, 280}, {690, 340} }; //僵尸的9个站位

	int index[9]; //9个僵尸的起始图片帧序号
	for (int i = 0; i < 9; i++) {
		index[i] = rand() % 11;
	}

	/*场景1逐渐切换至场景2*/
	int count = 0; //定时器变量
	for (int x = 0; x >= xMin; x -= 2) {
		BeginBatchDraw();
		putimage(x, 0, &imgBg);

		count++;
		for (int i = 0; i < 9; i++) {
			putimagePNG(points[i].x - xMin + x,
				points[i].y,
				&imgZmStand[index[i]]);

			if (count >= 10) {
				index[i] = (index[i] + 1) % 11;
			}
		}
		if (count >= 10) count = 0;

		Sleep(5);
		EndBatchDraw();
	}

	/*场景2画面维持1S*/
	for (int i = 0; i < 20; i++) {
		BeginBatchDraw();
		putimage(xMin, 0, &imgBg);

		for (int j = 0; j < 9; j++) {
			putimagePNG(points[j].x, points[j].y, &imgZmStand[index[j]]);
			index[j] = (index[j] + 1) % 11;
		}

		Sleep(50);
		EndBatchDraw();
	}

	/*场景2逐渐切换至场景1*/
	count = 0; //定时器变量
	for (int x = xMin; x <= -112; x += 2) {
		BeginBatchDraw();
		putimage(x, 0, &imgBg);

		count++;
		for (int i = 0; i < 9; i++) {
			putimagePNG(points[i].x - xMin + x,
				points[i].y,
				&imgZmStand[index[i]]);

			if (count >= 10) {
				index[i] = (index[i] + 1) % 11;
			}
		}
		if (count >= 10) count = 0;

		Sleep(5);
		EndBatchDraw();
	}

}
/// <summary>
/// 植物卡片下滑出现
/// </summary>
void barsDown() {
	int yMin = -(imgBar.getheight());

	for (int y = yMin; y <= 0; y += 1) {
		BeginBatchDraw();
		putimage(-112, 0, &imgBg);
		putimagePNG(140, y, &imgBar);

		for (int i = 0; i < PLANTS_COUNT; i++) {
			int x = 226 + i * 65;
			int destY = 6;
			putimagePNG(x, y + 6, &imgCards[i]);
		}

		Sleep(5);
		EndBatchDraw();
	}
}
/// <summary>
/// 判断游戏是否结束
/// </summary>
/// <returns>ret</returns>
bool checkOver() {
	bool ret = false;
	if (gameStatus == WIN) {
		PlaySound("res/win.wav", NULL, SND_FILENAME | SND_ASYNC);
		Sleep(2000);
		loadimage(0, "res/win.png");
		ret = true;
	}
	else if (gameStatus == FAIL) {
		PlaySound("res/lose.wav", NULL, SND_FILENAME | SND_ASYNC);
		Sleep(2000);
		loadimage(0, "res/fail.png");
		ret = true;
	}

	return ret;
}

int main(void)
{
	gameInit(); /*init game data*/

	startUI(); 

	viewScene(); 

	barsDown();

	int timer = 0;	//记录距离上一次游戏画面更新已过去多长时间
	bool flag = true;	//true: 更新游戏画面； false: 不更新游戏画面


	while (1) {
		userClick();

		mciSendString("Play res/bg.wav", 0, 0, 0); //播放游戏背景音乐

		timer += getDelay();
		/*每10毫秒更新一次游戏画面*/
		if (timer > 10) {
			flag = true;
			timer = 0;
		}

		if (flag) {
			flag = false;

			updateGame();
			updateWindow();
			if (checkOver()) {
				mciSendString("Stop res/bg.wav", 0, 0, 0); //播放游戏背景音乐
				break;
			}
		}
	}

	system("pause");

	return 0;
}

