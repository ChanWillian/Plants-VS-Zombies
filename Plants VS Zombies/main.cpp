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

enum { PEN, SUNFLOWER, PLANTS_COUNT };	//ֲ�����������
enum { SBALL_DOWN, SBALL_GROUND, SBALL_COLLECT, SBALL_PRODUCT };	//����������˶�״̬
enum { GOING, WIN, FAIL };	//��Ϸ������״̬

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
	int blood;		//ֲ���Ѫ��
	int timer;		//��������ļ�ʱ��
};
struct plant pMap[3][9];	//Lawn map��Plants can be grown��

struct sunshineBall {
	int x, y;		//��������Ʈ������е����꣨X����ʼ�ղ��䣩
	int frameIndex;	//The sequence number of the currently displayed picture frame
	bool used;		//Whether the sunshine ball is in use
	int timer;		//the effective time of the sunshine ball
	float t;				//�������ƶ�����ʱ 0 ~ 1
	vector2 p1, p2, p3, p4;	//���������߿��ƽڵ�
	vector2 pCur;			//������ǰ��λ��
	float recNumMove;		//�������ƶ������ĵ���
	int status;				//������ǰ���˶�״̬
};
struct sunshineBall balls[10];//������أ��������̳߳أ�

struct zombie {
	int x, y;
	int frameIndex;	//��ʬͼƬ֡���
	bool used;	//��ʬ��ʹ��״̬
	int speed;	//��ʬ�ƶ��ٶ�
	int row;	//��ʬ�����к�
	int blood;	//��ʬ��Ѫ��
	bool dead;	//��ʬ������״̬
	bool eat;	//��ʬ��ֲ���״̬
};
struct zombie zms[10];

struct bullet {
	int x, y;
	int row;	//�ӵ������к�
	bool used;	//�ӵ���ʹ��״̬
	int speed;
	bool blast;		//�ӵ���ը״̬
	int frameIndex;	//�ӵ���ըͼƬ֡���
};
struct bullet bullets[30];

int ballCount = sizeof(balls) / sizeof(balls[0]);	//���õ�����������
int zmCount = sizeof(zms) / sizeof(zms[0]);			//���õĽ�ʬ����
int bltCount = sizeof(bullets) / sizeof(bullets[0]);//���õ�ֲ���ӵ�����

char imgUrl[64]; //ͼƬ·������

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
	srand(time(NULL));	//����������ӣ����������������

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
	f.lfQuality = ANTIALIASED_QUALITY;	//���������Ե���
	settextstyle(&f);
	setbkmode(TRANSPARENT);
	setcolor(BLACK);

	/*init game data*/
	memset(imgPlants, 0, sizeof(imgPlants));	//ȫ����ʼ��Ϊ0
	memset(pMap, -1, sizeof(pMap));				//ȫ����ʼ��Ϊ-1
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
			if (fileExist(imgUrl)) {	//��ͼƬ����
				imgPlants[i][j] = new IMAGE;	//�����ڴ�ռ�
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

	/*ͼƬ������С���ʵ���ӵ���ըЧ��*/
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
		if (zms[i].used) { //�����ڽ�ʬ��ʹ��
			int index = zms[i].frameIndex;
			IMAGE* img;
			if (zms[i].dead) { //����ʬ������
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
		putimage(x, y, &imgCards[i]);	//����ط���������������һ��ʱ�䣬��һ��ͼƬ�ͻ���Ⱦʧ�ܣ�
	}

	/*Render the selected plant card*/
	if (curPlant != -1) //������ֲ�ﱻѡ��
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
			if (pMap[i][j].type != -1) //������ֲ�ﱻ��ֲ
			{
				int x = pMap[i][j].x;	//ֲ���X����
				int y = pMap[i][j].y;	//ֲ���Y����
				int num = pMap[i][j].type;			//ֲ������
				int index = pMap[i][j].frameIndex;	//ֲ��ͼƬ֡����

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
				IMAGE* img = &imgBulletBlast[bullets[i].frameIndex];//�������¿����ڴ�ռ���÷��������ó���û��BUG
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
/// ��Ӧ�û����������Ĳ���
/// </summary>
void collectSunshine(ExMessage* msg)
{
	for (int i = 0; i < ballCount; i++) {
		if (balls[i].used) {	//������������ʹ��
			int x = balls[i].pCur.x;	//�������X����
			int y = balls[i].pCur.y;	//�������Y����

			if (msg->x > (x + 18) && msg->x < (x + 18) + 44
				&& msg->y >(y + 18) && msg->y < (y + 18) + 44) {	//�������򱻵��

				//balls[i].used = false;				//�޸�������ʹ��״̬
				balls[i].status = SBALL_COLLECT;	//�޸���������˶�״̬
				PlaySound("res/sunshine.wav", NULL, SND_FILENAME | SND_ASYNC);

				/*��Ϊ�������������ÿ��ƽڵ㣬ʵ���������ֱ���˶�*/
				balls[i].p1 = balls[i].pCur; //����������
				balls[i].p4 = vector2(150, 0); //��������յ�
				balls[i].t = 0;
				float distance = dis(balls[i].p1 - balls[i].p4);	//�����ľ���
				float off = 8;	//�������ƶ�����
				balls[i].recNumMove = 1.0 / (distance / off); //�������ƶ������ĵ���
				// speedΪ�������ƶ�Ƶ�ʣ�
				// s = v * t = (off * speed) * t; t=1ʱ��s = off * speed * 1; speed * 1Ϊ�������ƶ�������

				break;	//��������ѭ��
			}
		}
	}
}
/// <summary>
/// ��Ӧ�û��������
/// </summary>
void userClick() {
	ExMessage msg;
	static int status = 0;	//ֲ�￨Ƭѡ��״̬
	if (peekmessage(&msg))
	{
		if (msg.message == WM_LBUTTONDOWN) //���������������
		{
			if ((msg.x > 226)
				&& (msg.x < 226 + 65 * PLANTS_COUNT)
				&& (msg.y > 7) && (msg.y < 96)) {	//�������ֲ�￨Ƭ�����
				int index = (msg.x - 226) / 65;	//�����ֲ�￨Ƭ������ֵ
				status = 1;						//��ֲ�￨Ƭѡ��״̬Ϊ��
				curPlant = index;				//����ǰֲ������Ϊ��ǰֲ�￨Ƭ����
			}
			else collectSunshine(&msg);
		}
		else if (status == 1 && msg.message == WM_MOUSEMOVE) //�������ֲ�￨Ƭ���������������ƶ�
		{
			curX = msg.x;
			curY = msg.y;
		}
		else if (msg.message == WM_LBUTTONUP) {	//���������̧��
			if (status == 1) {	//������ֲ�￨Ƭ��ѡ��
				if (msg.x > 140 && msg.x < 876
					&& msg.y > 176 && msg.y < 489) {	//��ֲ�￨Ƭ�ƶ���ֲ���ͼ����

					int row = (msg.y - 176) / 102;
					int col = (msg.x - 140) / 81;

					if (pMap[row][col].type == -1) {	//������δ��ֲֲ��
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

						int x = 140 + col * 81;	//ֲ���X����
						int y = 176 + row * 102;	//ֲ���Y����

						memset(&pMap[row][col], 0, sizeof(pMap[row][col]));	//��ʼ��ֲ������
						pMap[row][col].x = x;
						pMap[row][col].y = y;
						pMap[row][col].type = curPlant;
						pMap[row][col].blood = 140;
					}
				}

				status = 0;				//��ֲ��ѡ��״̬Ϊ��
				curPlant = -1;			//�õ�ǰֲ������Ϊ��
			}
		}
	}
}
/// <summary>
/// ��������ֲ���ͼ
/// </summary>
void updateMap()
{
	//��������ֲ��ͼƬ֡���
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 9; j++) {
			int type = pMap[i][j].type;
			int index = pMap[i][j].frameIndex;

			if (type != -1) {	//������ֲ�ﱻ��ֲ
				index++;
				if (imgPlants[type][index] != NULL) pMap[i][j].frameIndex = index;
				else pMap[i][j].frameIndex = 0;
			}
		}
	}

}
/// <summary>
/// ��ʱ����������
/// </summary>
void createSunshineBall()
{
	/*��ʱ��������*/
	static int count = 0;
	static int fre = 128;
	count++;
	if (count >= fre)
	{
		fre = 100 + rand() % 40;
		count = 0;

		int i;
		for (i = 0; i < ballCount && balls[i].used; i++);	//��������أ�������ʹ�õ�������Ѱ��δ��ʹ�õ�������
		if (i >= ballCount) return;

		/*���������������*/
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

	/*���տ���ʱ��������*/
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 9; j++) {
			if (pMap[i][j].type == SUNFLOWER) { //��ֲ��Ϊ���տ�

				pMap[i][j].timer++;
				if (pMap[i][j].timer > 256) { //�����տ���ʱ���
					pMap[i][j].timer = 0;	//��ʱ������

					int k;
					for (k = 0; k < ballCount && balls[k].used; k++);	//Ѱ��δʹ�õ�������
					if (k < ballCount) {
						balls[k].used = true;
						balls[k].status = SBALL_PRODUCT;
						balls[k].p1 = vector2(pMap[i][j].x, pMap[i][j].y);
						int w = (rand() % 2 ? 1 : -1) * (100 + rand() % 50);
						balls[k].p4 = vector2(pMap[i][j].x + w,
							pMap[i][j].y + imgPlants[SUNFLOWER][0]->getheight() - imgSunshineBall[0].getheight());
						balls[k].p2 = vector2(balls[k].p1.x + (w * 0.3), balls[k].p1.y - 100); //���������߿��ƽڵ�1
						balls[k].p3 = vector2(balls[k].p1.x + (w * 0.7), balls[k].p1.y - 100); //���������߿��ƽڵ�2
						balls[k].recNumMove = 0.05;
						balls[k].t = 0;
					}
				}
			}
		}
	}
}
/// <summary>
/// �����������˶�״��
/// </summary>
void updateSunshineBall()
{
	/*��������������ͼƬ֡������*/
	for (int i = 0; i < ballCount; i++) {
		if (balls[i].used) {	//�������������ʹ��
			struct sunshineBall* ball = &balls[i];

			ball->frameIndex = (ball->frameIndex + 1) % 29;
			int status = ball->status;
			if (status == SBALL_DOWN) {	//���������ڽ���״̬
				ball->pCur = ball->p1 + ball->t * (ball->p4 - ball->p1);	//����������ǰλ��
				ball->t += ball->recNumMove;	//�����������ƶ�����ʱ
				if (ball->t >= 1) {	//�������ƶ����
					ball->timer = 0;
					ball->status = SBALL_GROUND;
				}
			}
			else if (status == SBALL_GROUND) { //�������������״̬
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
/// ��ʱ���ɽ�ʬ
/// </summary>
void createZombie() {
	if (createZCount >= ZM_MAX)
		return;

	static int count = 0;	//���ڶ�ʱ���ɽ�ʬ
	static int fre = 196;	//���ڶ�ʱ���ɽ�ʬ
	count++;

	if (count >= fre) {
		count = 0;
		fre = 256 + rand() % 64;

		int i;
		for (i = 0; i < zmCount && zms[i].used; i++);	//������ʬ���飬������ʹ�õĽ�ʬ��Ѱ��δ��ʹ�õĽ�ʬ
		if (i > zmCount) return;

		/*init zombie data*/
		zms[i].used = true;
		zms[i].x = WIN_WIDTH - 100;
		zms[i].row = rand() % 3;	//���ý�ʬ���к�
		zms[i].y = 172 + (zms[i].row + 1) * 100;	//���ý�ʬ��Y����
		zms[i].speed = 3;
		zms[i].blood = 100;
		zms[i].dead = false;
		zms[i].eat = false;

		createZCount++;
	}

}
/// <summary>
/// ��⽩ʬ��ֲ�����ײ��ֲ���ʬ = 1��n��
/// </summary>
void checkZm2Plt() {
	for (int i = 0; i < 3; i++) {

		for (int j = 0; j < 9; j++) {
			if (pMap[i][j].type == -1) continue;	//����ĳ����δ��ֲֲ���������Ѱ������ֲֲ�������

			for (int k = 0; k < zmCount; k++) {
				if (zms[k].used == false || zms[k].dead) continue; //����δʹ�û��������Ľ�ʬ��Ѱ����ʹ����δ�����Ľ�ʬ

				int row = zms[k].row;	//���к�Ϊ��ʬ�����к�

				if (i == row) { //�����ڽ�ʬ��ֲ����ͬһ��
					int x = pMap[i][j].x;	//ֲ���X����
					//int x = 140 + j * 81;	
					int x1 = x + 10;
					int x2 = x + 60;
					int x3 = zms[k].x + 80;

					if (x3 < x2 && x3 > x1) { //����ʬ��ֲ�﷢����ײ
						pMap[i][j].blood -= 2;

						if (zms[k].eat == false) { //����ʬ��ֲ��״̬Ϊ��
							zms[k].eat = true;
							zms[k].speed = 0;
							zms[k].frameIndex = 0;
						}

						if (pMap[i][j].blood <= 0) { //��ֲ��������
							pMap[i][j].type = -1;

							for (int i = 0; i < zmCount; i++) { //
								if (zms[i].row == row && zms[i].eat) {
									zms[i].eat = false;
									zms[i].speed = 3;
									zms[i].frameIndex = 0;
								}
							}

							break;	//��������ѭ��
						}
					}
				}
			}
		}
	}
}
/// <summary>
/// �����ӵ����˶�״��
/// </summary>
void updateZombie() {
	/*��ʱ���½�ʬX����*/
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

	checkZm2Plt();//����Ƿ���ڽ�ʬ��ֲ�﷢����ײ

	/*��ʱ���½�ʬ��ͼƬ֡*/
	static int count_index = 0;
	count_index++;
	if (count_index >= 3) {
		count_index = 0;
		for (int i = 0; i < zmCount; i++) {
			if (zms[i].used) {	//�����ڽ�ʬ��ʹ��
				if (zms[i].dead) {	//����ʬ������
					zms[i].speed = 0;
					zms[i].frameIndex++;
					if (zms[i].frameIndex >= 20) { //����ʬ����ͼƬ֡���ڵ���20������ʬ���������������
						zms[i].used = false;	//�ý�ʬʹ��״̬Ϊ��
						killZCount++;
						if (killZCount >= ZM_MAX)
							gameStatus = WIN;
					}
				}
				else if (zms[i].eat) {	//����ʬ�ڳ�ֲ��
					zms[i].frameIndex = (zms[i].frameIndex + 1) % imgZECount;
				}
				else {	//����ʬ������
					zms[i].frameIndex = (zms[i].frameIndex + 1) % imgZCount;
				}
			}
		}
	}
}
/// <summary>
/// ��ʱ�����ӵ�
/// </summary>
void createBullet()
{
	bool zmLines[3] = { 0 };	//ĳ�н�ʬ����״̬��ĳ���Ƿ���ڱ�ʹ����δ�����Ľ�ʬ
	int dangerX = WIN_WIDTH - imgZombie[0].getwidth();
	for (int i = 0; i < zmCount; i++) {
		if (zms[i].used
			&& zms[i].dead == false
			&& zms[i].x < dangerX) //�����ڱ�ʹ�á�δ�����ҽ���ֲ�������Χ�Ľ�ʬ
			zmLines[zms[i].row] = true;	//��ĳ�н�ʬ����״̬Ϊ��
	}

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 9; j++) {
			if (pMap[i][j].type == PEN && zmLines[i] == true) {	//��ֲ�������������������д��ڽ�ʬ
				static int count = 0;
				count++;
				if (count > 40) {
					count = 0;

					int k;
					for (k = 0; k < bltCount && bullets[k].used; k++); //�����ӵ����飬�����ѱ�ʹ�õ��ӵ���Ѱ��δ��ʹ�õ��ӵ�
					if (k < bltCount) {
						int pltX = 140 + j * 81;	//ֲ���X����
						int pltY = 176 + i * 102;	//ֲ���Y����

						/*��ʼ���ӵ�*/
						bullets[k].used = true;
						bullets[k].x = pltX + 62;
						bullets[k].y = pltY + 10;
						bullets[k].row = i; //���ӵ������к�Ϊֲ�������к�
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
/// ����ӵ��ͽ�ʬ����ײ���ӵ�����ʬ = 1��1��
/// </summary>
void checkBlt2Zm() {
	for (int i = 0; i < bltCount; i++) {
		if (bullets[i].used == false || bullets[i].blast == true) //����δʹ�û��ѱ�ը���ӵ���Ѱ����ʹ�ò���δ��ը���ӵ�
			continue;

		for (int j = 0; j < zmCount; j++) {
			if (zms[j].used == false || zms[j].dead)	//����δʹ�û��������Ľ�ʬ��Ѱ����ʹ����δ�����Ľ�ʬ
				continue;

			if (bullets[i].row == zms[j].row) { //�����ڽ�ʬ��ֲ���ӵ���ͬһ��
				int x = bullets[i].x;
				int x1 = zms[j].x + 80;
				int x2 = zms[j].x + 110;

				if (x > x1 && x < x2) { //�������ӵ��ͽ�ʬ������ײ
					bullets[i].blast = true;
					bullets[i].speed = 0;

					zms[j].blood -= 10;

					if (zms[j].blood <= 0) {	//����ʬѪ��Ϊ0
						zms[j].dead = true;
						zms[j].speed = 0;
						zms[j].frameIndex = 0;	//�ý�ʬ����ͼƬ֡���Ϊ0
					}

					break;	//��������ѭ��
				}
			}
		}
	}

}
/// <summary>
/// �����ӵ����˶�״̬
/// </summary>
void updateBullet()
{
	for (int i = 0; i < bltCount; i++) {
		if (bullets[i].used == true) {	//�������ӵ���ʹ��
			bullets[i].x += bullets[i].speed; //�����ӵ���X����
			if (bullets[i].x > WIN_WIDTH) {
				bullets[i].used = false;
			}

			checkBlt2Zm(); //����Ƿ�����ӵ��ͽ�ʬ������ײ

			if (bullets[i].blast == true) {	//�������ӵ��ͽ�ʬ������ײ
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
				&& msg.y > 75 && msg.y < 75 + 140) {	//��������˵���
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
/// ��������
/// </summary>
void viewScene() {
	int xMin = -(imgBg.getwidth() - WIN_WIDTH); //- (1400 - 900) = -500
	vector2 points[9] = {
		{550, 80}, {530, 160}, {630, 170}, {530, 200}, {515, 270},
		{563, 370}, {605, 340}, {705, 280}, {690, 340} }; //��ʬ��9��վλ

	int index[9]; //9����ʬ����ʼͼƬ֡���
	for (int i = 0; i < 9; i++) {
		index[i] = rand() % 11;
	}

	/*����1���л�������2*/
	int count = 0; //��ʱ������
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

	/*����2����ά��1S*/
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

	/*����2���л�������1*/
	count = 0; //��ʱ������
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
/// ֲ�￨Ƭ�»�����
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
/// �ж���Ϸ�Ƿ����
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

	int timer = 0;	//��¼������һ����Ϸ��������ѹ�ȥ�೤ʱ��
	bool flag = true;	//true: ������Ϸ���棻 false: ��������Ϸ����


	while (1) {
		userClick();

		mciSendString("Play res/bg.wav", 0, 0, 0); //������Ϸ��������

		timer += getDelay();
		/*ÿ10�������һ����Ϸ����*/
		if (timer > 10) {
			flag = true;
			timer = 0;
		}

		if (flag) {
			flag = false;

			updateGame();
			updateWindow();
			if (checkOver()) {
				mciSendString("Stop res/bg.wav", 0, 0, 0); //������Ϸ��������
				break;
			}
		}
	}

	system("pause");

	return 0;
}

