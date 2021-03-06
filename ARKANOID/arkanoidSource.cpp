#include <time.h>
#include <math.h>
#include <stdlib.h>
#include "glut.h"
#include "glm/gtc/constants.hpp"

#define WINDOW_WIDTH	(600)
#define WINDOW_HEIGHT	(700)

#define BLOCK_WIDTH		(50)
#define BLOCK_HEIGHT	(25)

#define WALL_COLUMN_MAX	(24)
#define WALL_ROW_MAX	(28)

#define BLOCK_COLUMN_MAX	(11)
#define BLOCK_ROW_MAX		(27)

#define PADDLE_WIDTH	(BLOCK_WIDTH * 2)

#define ITEM_MAX	(7)

#define BALL_MAX	(5)

//ブロックの種類
enum {
	TYPE_BLOCK_ORANGE,
	TYPE_BLOCK_GREEN,
	TYPE_BLOCK_LBLUE,
	TYPE_BLOCK_BLUE,
	TYPE_BLOCK_RED,
	TYPE_BLOCK_PINK,
	TYPE_BLOCK_SILVER,
	TYPE_BLOCK_GOLD,
	TYPE_BLOCK_NONE,
	TYPE_BLOCK_MAX
};

//時間制限
typedef struct {
	clock_t lastUpdate;	 //最後の計測時間
	clock_t nowUpdate;	 //最新の計測時間
	float updateInterval;//時間間隔
}CLOCK;

CLOCK clockMode;

//2次元ベクトル
typedef struct {
	float x, y;
}VEC2;

//パドル
typedef struct {
	VEC2 position;			//座標
	int itemNumber;			//番号ごとに能力変化

	bool pinkEscape;		//画面外への脱出フラグ
	bool pinkOpen;			//画面外への扉開閉フラグ
	bool greenCatch;		//パドルにボール張り付くフラグ
	float greenDifference;	//パドルに対するボールの距離

	int life;				//パドルの残機
	int blockCount;			//画面上のブロックの数
	float width;			//パドルの幅
	float height;			//パドルの高さ
}PADDLE;

PADDLE paddle;

//ボール
typedef struct {
	VEC2 position;		//座標
	VEC2 lastPosition;	//古い座標
	VEC2 velocity;		//速度
	VEC2 speed;			//速さ
	bool fall;			//落下フラグ
	bool bullet;		//銃弾フラグ
	bool redShot;		//発射フラグ
}BALL;

BALL ball[BALL_MAX];

//ブロック
typedef struct {
	int type;		//ブロックの色分け
	int durability;	//ブロックの耐久値
	int itemNumber;	//番号を持つブロックを壊すとアイテムが出てくる
}BLOCK;

BLOCK blocks[BLOCK_ROW_MAX][BLOCK_COLUMN_MAX];

//アイテム
typedef struct {
	VEC2 position;	//座標
	VEC2 velocity;	//速度
	bool itemFall;	//落下中フラグ
	int itemNumber;	//アイテムがパドルに当たれば番号を渡す
}ITEM;

ITEM item;

//キーボード256個分のバイト配列
bool keysPressed[256];

//能力変化前の状態
void playerNormalMode(void) {
	int count = 0;
	for (int i = 0;i < BALL_MAX;i++) {
		ball[i].bullet = false;
		ball[i].redShot = false;

		if (!ball[i].fall) count++;
		if (count >= 2) ball[i].fall = true;
	}
	paddle.width = BLOCK_WIDTH * 2;
	paddle.greenCatch = false;
}

//ボールがブロックと衝突したかの判定
bool ballHitsBlock(int _ballNumber, int* _columnNumber, int* _rowNumber) {
	int x = ((int)ball[_ballNumber].position.x - BLOCK_WIDTH / 2) / BLOCK_WIDTH;
	int y = ((int)ball[_ballNumber].position.y - BLOCK_HEIGHT) / BLOCK_HEIGHT;

	if ((blocks[y][x].type == TYPE_BLOCK_NONE) ||
		(y < 0) || (y >= BLOCK_ROW_MAX) ||
		(x < 0) || (x >= BLOCK_COLUMN_MAX)) return false;
	else {
		*_columnNumber = x;
		*_rowNumber = y;
		return true;
	}
}

//ボールの軌道がどの線分(ブロックの辺)と交差してるかの判定
bool ballCrossLine(int _ballNumber, int _columnNumber, int _rowNumber, VEC2 _blockStartPoint, VEC2 _blockEndPoint) {
	//2つの線分が交差してるか外積を使って判定

	VEC2 ballStartToBallEnd = {
		ball[_ballNumber].position.x - ball[_ballNumber].lastPosition.x,
		ball[_ballNumber].position.y - ball[_ballNumber].lastPosition.y
	};

	VEC2 blockStartToBlockEnd = {
		_blockEndPoint.x - _blockStartPoint.x,
		_blockEndPoint.y - _blockStartPoint.y
	};

	VEC2 ballStartToBlockStart = {
		_blockStartPoint.x - ball[_ballNumber].lastPosition.x,
		_blockStartPoint.y - ball[_ballNumber].lastPosition.y
	};

	VEC2 ballStartToBlockEnd = {
		_blockEndPoint.x - ball[_ballNumber].lastPosition.x,
		_blockEndPoint.y - ball[_ballNumber].lastPosition.y
	};

	VEC2 blockStartToBallStart = {
		ball[_ballNumber].lastPosition.x - _blockStartPoint.x,
		ball[_ballNumber].lastPosition.y - _blockStartPoint.y
	};

	VEC2 blockStartToBallEnd = {
		ball[_ballNumber].position.x - _blockStartPoint.x,
		ball[_ballNumber].position.y - _blockStartPoint.y
	};

	VEC2 differencePoints = {
		_blockEndPoint.x - _blockStartPoint.x,
		_blockEndPoint.y - _blockStartPoint.y
	};

	float BaStToBlStCrossBaStToBaEn = ballStartToBlockStart.x * ballStartToBallEnd.y
		- ballStartToBlockStart.y * ballStartToBallEnd.x;

	float BaStToBlEnCrossBaStToBaEn = ballStartToBlockEnd.x * ballStartToBallEnd.y
		- ballStartToBlockEnd.y * ballStartToBallEnd.x;

	float BlStToBaStCrossBlStToBlEn = blockStartToBallStart.x * blockStartToBlockEnd.y
		- blockStartToBallStart.y * blockStartToBlockEnd.x;

	float BlStToBaEnCrossBlStToBlEn = blockStartToBallEnd.x * blockStartToBlockEnd.y
		- blockStartToBallEnd.y * blockStartToBlockEnd.x;

	if ((BaStToBlStCrossBaStToBaEn * BaStToBlEnCrossBaStToBaEn <= 0) &&
		(BlStToBaStCrossBlStToBlEn * BlStToBaEnCrossBlStToBlEn <= 0))
	{
		ball[_ballNumber].position = ball[_ballNumber].lastPosition;

		if (differencePoints.x == 0) ball[_ballNumber].velocity.x *= -1;

		else if (differencePoints.y == 0) ball[_ballNumber].velocity.y *= -1;

		return true;
	}
	else return false;
}

//ボールを跳ね返す
void ballRebound(int _ballNumber, int _columnNumber, int _rowNumber) {
	//ブロックの4点を決める

	VEC2 upperLeftPoint = {
		(float)BLOCK_WIDTH / 2 + _columnNumber * BLOCK_WIDTH,
		(float)BLOCK_HEIGHT + _rowNumber * BLOCK_HEIGHT
	};
	VEC2 lowerLeftPoint = {
		upperLeftPoint.x,
		upperLeftPoint.y + (float)BLOCK_HEIGHT,
	};

	VEC2 upperRightPoint = {
		upperLeftPoint.x + (float)BLOCK_WIDTH,
		upperLeftPoint.y
	};
	VEC2 lowerRightPoint = {
		upperRightPoint.x,
		lowerLeftPoint.y
	};

	if (ballCrossLine(_ballNumber, _columnNumber, _rowNumber, upperLeftPoint, upperRightPoint))
		return;
	else if (ballCrossLine(_ballNumber, _columnNumber, _rowNumber, lowerLeftPoint, lowerRightPoint))
		return;
	else if (ballCrossLine(_ballNumber, _columnNumber, _rowNumber, upperLeftPoint, lowerLeftPoint))
		return;
	else if (ballCrossLine(_ballNumber, _columnNumber, _rowNumber, upperRightPoint, lowerRightPoint))
		return;
}

//時間管理
bool timePasses(clock_t _lastClock, clock_t _nowClock, float _interval) {
	_nowClock = clock();
	if (_nowClock >= _lastClock + _interval) return true;
	else return false;
}

//ブロックの色を選択
void color(int _colorType) {
	if (_colorType == TYPE_BLOCK_ORANGE)glColor3ub(0xff, 0x8c, 0x00);
	if (_colorType == TYPE_BLOCK_GREEN)	glColor3ub(0x00, 0xff, 0x00);
	if (_colorType == TYPE_BLOCK_LBLUE) glColor3ub(0x00, 0xff, 0xff);
	if (_colorType == TYPE_BLOCK_BLUE)	glColor3ub(0x00, 0x00, 0xff);
	if (_colorType == TYPE_BLOCK_RED)	glColor3ub(0xff, 0x00, 0x00);
	if (_colorType == TYPE_BLOCK_PINK)	glColor3ub(0xff, 0x69, 0xb4);
	if (_colorType == TYPE_BLOCK_SILVER)glColor3ub(0xc0, 0xc0, 0xc0);
	if (_colorType == TYPE_BLOCK_GOLD)	glColor3ub(0xe6, 0xb4, 0x22);
	if (_colorType == TYPE_BLOCK_NONE)	glColor3ub(0x00, 0x00, 0x8b);
}

//プログラムの初期化
void Init(void) {
	//ブロックの初期化
	paddle.blockCount = 0;
	for (int y = 0;y < BLOCK_ROW_MAX;y++)
		for (int x = 0;x < BLOCK_COLUMN_MAX;x++) {
			//ブロックの色分け
			if ((y >= 3) && (y <= 8)) {
				if (y == 3) blocks[y][x].type = TYPE_BLOCK_SILVER;
				if (y == 4) blocks[y][x].type = TYPE_BLOCK_RED;
				if (y == 5) blocks[y][x].type = TYPE_BLOCK_BLUE;
				if (y == 6) blocks[y][x].type = TYPE_BLOCK_ORANGE;
				if (y == 7) blocks[y][x].type = TYPE_BLOCK_PINK;
				if (y == 8) blocks[y][x].type = TYPE_BLOCK_GREEN;

				paddle.blockCount++;
			}
			else blocks[y][x].type = TYPE_BLOCK_NONE;

			if (blocks[y][x].type == TYPE_BLOCK_SILVER) blocks[y][x].durability = 2;
			else if (blocks[y][x].type == TYPE_BLOCK_GOLD) blocks[y][x].durability = 1000;
			else blocks[y][x].durability = 1;

			blocks[y][x].itemNumber = -1;
		}

	//アイテムの初期化
	for (int i = TYPE_BLOCK_ORANGE;i <= TYPE_BLOCK_SILVER;i++) {
		//ランダムでブロックにアイテムナンバーを渡す
		while (1) {
			int rowNumber = rand() % BLOCK_ROW_MAX;
			int columnNumber = rand() % BLOCK_COLUMN_MAX;

			if ((blocks[rowNumber][columnNumber].itemNumber == -1) &&
				(blocks[rowNumber][columnNumber].type != TYPE_BLOCK_NONE))
			{
				blocks[rowNumber][columnNumber].itemNumber = i;
				break;
			}
		}
	}

	item.itemFall = false;
	item.itemNumber = -1;

	//パドルの初期化
	paddle.position = {
		12 * BLOCK_WIDTH / 2,
		25 * BLOCK_HEIGHT
	};
	paddle.life = 2;
	paddle.itemNumber = -1;

	paddle.pinkEscape = false;
	paddle.pinkOpen = false;
	paddle.greenCatch = false;

	paddle.width = BLOCK_WIDTH * 2;
	paddle.height = BLOCK_HEIGHT;

	//ボールの初期化
	for (int i = 0;i < BALL_MAX;i++) {
		ball[i].bullet = false;
		ball[i].redShot = false;
		ball[i].speed = { 0.65f, 0.65f };
		ball[i].position = {
			12 * BLOCK_WIDTH / 2,
			25 * BLOCK_HEIGHT
		};
		ball[i].velocity = {
			rand() % 2 == 0 ? ball[i].speed.x : -ball[i].speed.x,
			-ball[i].speed.y
		};
		if (i == 0) ball[i].fall = false;
		else ball[i].fall = true;
	}

	//時間
	clockMode.updateInterval = 1000 / 60;
}

//ブロックやパドルの描画
void Display(void) {
	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//背景の描画
	for (int y = 0;y < BLOCK_ROW_MAX;y++) {
		for (int x = 0;x < BLOCK_COLUMN_MAX;x++) {
			if (blocks[y][x].type != TYPE_BLOCK_NONE) continue;

			for (int i = 0;i < 2;i++) {
				int type = 0;
				if (i == 0) {
					type = GL_QUADS;
					color(blocks[y][x].type);
				}
				if ((i == 1) && (blocks[y][x].type != TYPE_BLOCK_NONE)) {
					type = GL_LINE_LOOP;
					glLineWidth(1);
					glColor3ub(0x00, 0x00, 0x00);
				}

				glPushMatrix();
				{
					glTranslatef(
						(GLfloat)x * BLOCK_WIDTH + BLOCK_WIDTH / 2,
						(GLfloat)y * BLOCK_HEIGHT + BLOCK_HEIGHT,
						0);
					glBegin(type);
					{
						glVertex2f(0, 0);
						glVertex2f(0, BLOCK_HEIGHT);
						glVertex2f(BLOCK_WIDTH, BLOCK_HEIGHT);
						glVertex2f(BLOCK_WIDTH, 0);
					}
					glEnd();
				}
				glPopMatrix();
			}
		}
	}

	//ブロックの描画
	for (int y = 0;y < BLOCK_ROW_MAX;y++) {
		for (int x = 0;x < BLOCK_COLUMN_MAX;x++) {
			if (blocks[y][x].type == TYPE_BLOCK_NONE) continue;

			for (int i = 0;i < 2;i++) {
				int type = 0;
				if (i == 0) {
					type = GL_QUADS;
					color(blocks[y][x].type);
				}
				if ((i == 1) && (blocks[y][x].type != TYPE_BLOCK_NONE)) {
					type = GL_LINE_LOOP;
					glLineWidth(1);
					glColor3ub(0x00, 0x00, 0x00);
				}

				glPushMatrix();
				{
					glTranslatef(
						(GLfloat)x * BLOCK_WIDTH + BLOCK_WIDTH / 2,
						(GLfloat)y * BLOCK_HEIGHT + BLOCK_HEIGHT,
						0);
					glBegin(type);
					{
						glVertex2f(0, 0);
						glVertex2f(0, BLOCK_HEIGHT);
						glVertex2f(BLOCK_WIDTH, BLOCK_HEIGHT);
						glVertex2f(BLOCK_WIDTH, 0);
					}
					glEnd();
				}
				glPopMatrix();
			}

			if ((blocks[y][x].type == TYPE_BLOCK_SILVER) || (blocks[y][x].type == TYPE_BLOCK_GOLD))
			{
				glPushMatrix();
				{
					glTranslatef(
						(GLfloat)x * BLOCK_WIDTH + BLOCK_WIDTH / 2 + BLOCK_WIDTH - 2,
						(GLfloat)y * BLOCK_HEIGHT + BLOCK_HEIGHT + 2,
						0);
					glLineWidth(3);
					glBegin(GL_LINE_STRIP);
					{
						glVertex2f(0, 0);
						glVertex2f(0, BLOCK_HEIGHT - 4);
						glVertex2f(-(BLOCK_WIDTH - 4), BLOCK_HEIGHT - 4);
					}
					glEnd();
				}
				glPopMatrix();
			}
		}
	}

	//パドルの描画
	for (int j = 0;j < 3;j++)
		for (int i = 0;i < 2;i++) {
			GLfloat x = (GLfloat)paddle.position.x;
			GLfloat y = (GLfloat)paddle.position.y;
			glPushMatrix();
			{
				int type = 0;
				if (i == 0) {
					type = GL_QUADS;
					if (j == 0) {
						glTranslatef(x - paddle.width / 2, y, 0);
						glColor3ub(0xc0, 0xc0, 0xc0);
					}
					else {
						if (paddle.itemNumber < 0) glColor3ub(0xff, 0xff, 0xff);
						else color(paddle.itemNumber);
						if (j == 1) glTranslatef(x - paddle.width / 2, y, 0);
						if (j == 2) glTranslatef(x + paddle.width / 2 - BLOCK_WIDTH / 2, y, 0);
					}
				}
				if (i == 1) {
					type = GL_LINE_LOOP;
					glLineWidth(1);
					if (j == 0) glTranslatef(x - paddle.width / 2, y, 0);
					if (j == 1) glTranslatef(x - paddle.width / 2, y, 0);
					if (j == 2) glTranslatef(x + paddle.width / 2 - BLOCK_WIDTH / 2, y, 0);
					glColor3ub(0x00, 0x00, 0x00);
				}
				glBegin(type);
				{
					glVertex2f(0, 0);
					if (j == 0) {
						glVertex2f((GLfloat)paddle.width, 0);
						glVertex2f((GLfloat)paddle.width, (GLfloat)paddle.height);
					}
					else {
						glVertex2f(BLOCK_WIDTH / 2, 0);
						glVertex2f(BLOCK_WIDTH / 2, (GLfloat)paddle.height);
					}
					glVertex2f(0, (GLfloat)paddle.height);
				}
				glEnd();
			}
			glPopMatrix();
		}

	//アイテム	
	if (item.itemFall) {
		color(item.itemNumber);

		GLfloat itemX = item.position.x;
		GLfloat itemY = item.position.y;

		for (int i = 0;i < 2;i++) {
			glPushMatrix();
			{
				if (i == 0)
					glTranslatef(itemX - (GLfloat)BLOCK_WIDTH / 4, itemY + (GLfloat)BLOCK_HEIGHT / 2, 0);
				if (i == 1)
					glTranslatef(itemX + (GLfloat)BLOCK_WIDTH / 4, itemY + (GLfloat)BLOCK_HEIGHT / 2, 0);

				glBegin(GL_TRIANGLE_FAN);
				{
					glVertex2f(0, 0);
					GLfloat radius = BLOCK_HEIGHT / 2;
					int n = 32;
					for (int i = 0;i <= n;i++) {
						float r = glm::pi<float>() * 2 * i / n;
						glVertex2f(radius * cosf(r), -radius * sinf(r));
					}
				}
				glEnd();
			}
			glPopMatrix();
		}

		glPushMatrix();
		{
			glTranslatef(itemX - BLOCK_WIDTH / 4, itemY, 0);
			glBegin(GL_QUADS);
			{
				glVertex2f(0, 0);
				glVertex2f(0, BLOCK_HEIGHT);
				glVertex2f(BLOCK_WIDTH / 2, BLOCK_HEIGHT);
				glVertex2f(BLOCK_WIDTH / 2, 0);
			}
			glEnd();
		}
		glPopMatrix();
	}

	//ボールの描画
	for (int i = 0;i < BALL_MAX;i++) {
		if (ball[i].fall) continue;

		if ((ball[i].bullet) && (!ball[i].redShot)) continue;

		glPushMatrix();
		{
			glTranslatef(
				(GLfloat)ball[i].position.x,
				(GLfloat)ball[i].position.y,
				0);

			glColor3ub(0xff, 0xff, 0x00);

			glBegin(GL_TRIANGLE_FAN);
			{
				glVertex2f(0, 0);
				GLfloat radius = BLOCK_HEIGHT / 4;
				int n = 32;
				for (int i = 0;i <= n;i++) {
					float r = glm::pi<float>() * 2 * i / n;
					glVertex2f(radius * cosf(r), -radius * sinf(r));
				}
			}
			glEnd();
		}
		glPopMatrix();
	}

	//ミニパドル
	for (int k = 0;k < paddle.life;k++) {
		GLfloat width = BLOCK_WIDTH / 2;
		GLfloat height = BLOCK_HEIGHT / 2;
		GLfloat x = BLOCK_WIDTH / 2 + width / 2;
		GLfloat y = (WALL_ROW_MAX - 1) * BLOCK_HEIGHT;

		x = x + (width + 3) * k;

		for (int j = 0;j < 3;j++)
			for (int i = 0;i < 2;i++) {
				glPushMatrix();
				{
					int type = 0;
					if (i == 0) {
						type = GL_QUADS;
						if (j == 0) {
							glTranslatef(x - width / 2, y, 0);
							glColor3ub(0xc0, 0xc0, 0xc0);
						}
						else {
							glColor3ub(0xff, 0x00, 0x00);
							if (j == 1) glTranslatef(x - width / 2, y, 0);
							if (j == 2) glTranslatef(x + width / 2 - width / 4, y, 0);
						}
					}
					if (i == 1) {
						type = GL_LINE_LOOP;
						glLineWidth(1);
						if (j == 0) glTranslatef(x - width / 2, y, 0);
						if (j == 1) glTranslatef(x - width / 2, y, 0);
						if (j == 2) glTranslatef(x + width / 2 - width / 4, y, 0);
						glColor3ub(0x00, 0x00, 0x00);
					}
					glBegin(type);
					{
						glVertex2f(0, 0);
						if (j == 0) {
							glVertex2f((GLfloat)width, 0);
							glVertex2f((GLfloat)width, (GLfloat)height);
						}
						else {
							glVertex2f(width / 4, 0);
							glVertex2f(width / 4, (GLfloat)height);
						}
						glVertex2f(0, (GLfloat)height);
					}
					glEnd();
				}
				glPopMatrix();
			}
	}

	//壁の描画
	for (int x = 0;x < WALL_COLUMN_MAX;x++) {
		for (int i = 0;i < 2;i++) {
			int type = 0;
			if (i == 0) {
				type = GL_QUADS;
				glColor3ub(0xc0, 0xc0, 0xc0);
			}
			if (i == 1) {
				type = GL_LINE_LOOP;
				glColor3ub(0x00, 0x00, 0x00);
				glLineWidth(1);
			}

			glPushMatrix();
			{
				glTranslatef((GLfloat)x * BLOCK_WIDTH / 2, 0, 0);
				glBegin(type);
				{
					glVertex2f(0, 0);
					glVertex2f(0, BLOCK_HEIGHT);
					glVertex2f(BLOCK_WIDTH / 2, BLOCK_HEIGHT);
					glVertex2f(BLOCK_WIDTH / 2, 0);
				}
				glEnd();
			}
			glPopMatrix();
		}
	}

	for (int y = 0;y < WALL_ROW_MAX;y++) {
		GLfloat wallWidth = BLOCK_WIDTH / 2;
		GLfloat wallHeight = BLOCK_HEIGHT;
		for (int x = 0;x < WALL_COLUMN_MAX;x += WALL_COLUMN_MAX - 1) {
			if ((paddle.pinkEscape) &&
				(y == 25) && (x == 23))
			{
				if (!paddle.pinkOpen) wallWidth = BLOCK_WIDTH / 6;
				else continue;
			}

			for (int i = 0;i < 2;i++) {
				int type = 0;
				if (i == 0) {
					type = GL_QUADS;
					if ((paddle.pinkEscape) &&
						(y == 25) && (x == 23) &&
						(!paddle.pinkOpen))
					{
						glColor3ub(0xff, 0xff, 0xff);
					}
					else glColor3ub(0xc0, 0xc0, 0xc0);
				}
				if (i == 1) {
					type = GL_LINE_LOOP;
					glLineWidth(1);
					glColor3ub(0x00, 0x00, 0x00);
				}

				glPushMatrix();
				{
					glTranslatef((GLfloat)x * BLOCK_WIDTH / 2, (GLfloat)y * BLOCK_HEIGHT, 0);
					glBegin(type);
					{
						glVertex2f(0, 0);
						glVertex2f(0, wallHeight);
						glVertex2f(wallWidth, wallHeight);
						glVertex2f(wallWidth, 0);
					}
					glEnd();
				}
				glPopMatrix();
			}
		}
	}

	glutSwapBuffers();
}

//ボールやブロック、アイテムの挙動を処理する
void Idle(void) {
	if (timePasses(clockMode.lastUpdate, clockMode.nowUpdate, clockMode.updateInterval)) {

		//ボール
		int ballFallCount = 0;//ボールカウントの初期化
		for (int i = 0;i < BALL_MAX;i++) {
			//落下か弾ならカウント
			if ((ball[i].fall) || (ball[i].bullet)) {
				ballFallCount++;
				//落下のボールは除外
				if (ball[i].fall) continue;
			}

			//現在の座標を保存
			ball[i].lastPosition = ball[i].position;

			if ((paddle.itemNumber == TYPE_BLOCK_GREEN) || (paddle.itemNumber == TYPE_BLOCK_RED)) {
				//パドルが緑ならボールがパドルに張り付く
				if (paddle.itemNumber == TYPE_BLOCK_GREEN) {
					//パドルに対するボールの距離を計算
					if (paddle.greenCatch) {
						ball[i].position.x = paddle.position.x + paddle.greenDifference;
						ball[i].position.y = paddle.position.y;
						continue;
					}
					//張り付いてないならボールを飛ばす
					else {
						ball[i].position.x += ball[i].velocity.x;
						ball[i].position.y += ball[i].velocity.y;
					}
				}
				//パドルが赤ならボールが銃弾になる
				if (paddle.itemNumber == TYPE_BLOCK_RED) {
					//銃弾のボール
					if (ball[i].bullet) {
						//銃弾は左クリックで飛ぶ
						if (ball[i].redShot) {
							ball[i].position.x += ball[i].velocity.x;
							ball[i].position.y += ball[i].velocity.y;
						}
						else continue;
					}
					//普通のボール
					else {
						ball[i].position.x += ball[i].velocity.x;
						ball[i].position.y += ball[i].velocity.y;
					}
				}
			}
			else {
				//パドルがその他の色なら
				ball[i].position.x += ball[i].velocity.x;
				ball[i].position.y += ball[i].velocity.y;
			}

			//ボールがブロックに当たれば
			int columnNumber, rowNumber;
			if (ballHitsBlock(i, &columnNumber, &rowNumber)) {
				//ブロックの耐久値を下げる
				blocks[rowNumber][columnNumber].durability--;

				//銃弾のボールなら跳ね返さず銃弾を消す
				if (ball[i].bullet) ball[i].redShot = false;
				//普通のボールなら跳ね返す
				else ballRebound(i, columnNumber, rowNumber);

				//ブロックの耐久値が0なら
				if (blocks[rowNumber][columnNumber].durability <= 0) {
					//ブロックを消す
					blocks[rowNumber][columnNumber].type = TYPE_BLOCK_NONE;
					//ブロックの数を減らす
					paddle.blockCount--;

					//ブロックの中にアイテムが隠されていれば
					if ((blocks[rowNumber][columnNumber].itemNumber >= 0) && (!item.itemFall)) {
						//trueの間は別のアイテムは落ちてこない
						item.itemFall = true;
						//ブロックの持つアイテム番号を受け取り、パドルに当たれば番号を渡す
						item.itemNumber = blocks[rowNumber][columnNumber].itemNumber;
						//座標
						item.position = {
							(float)BLOCK_WIDTH / 2 + columnNumber * BLOCK_WIDTH + BLOCK_WIDTH / 2,
							(float)BLOCK_HEIGHT + rowNumber * BLOCK_HEIGHT
						};
						//速度
						item.velocity = { 0,0.25f };
					}
				}
			}

			//壁
			{
				//壁に当たれば跳ね返す
				if (ball[i].position.x > 23 * BLOCK_WIDTH / 2) {
					ball[i].position.x = 23 * BLOCK_WIDTH / 2;
					ball[i].velocity.x *= -1;
				}
				if (ball[i].position.x < BLOCK_WIDTH / 2) {
					ball[i].position.x = BLOCK_WIDTH / 2;
					ball[i].velocity.x *= -1;
				}
				if (ball[i].position.y < BLOCK_HEIGHT) {
					//銃弾のボールが当たれば消す
					if (ball[i].bullet) ball[i].redShot = false;
					else {
						ball[i].position.y = BLOCK_HEIGHT;
						ball[i].velocity.y *= -1;
					}
				}
				//落下をカウントする
				if (ball[i].position.y > 28 * BLOCK_HEIGHT) {
					ball[i].fall = true;
					ballFallCount++;
				}
			}

			//パドルとボールが当たれば
			if ((ball[i].position.y >= paddle.position.y) &&
				(ball[i].position.y <= paddle.position.y + paddle.height) &&
				(ball[i].position.x >= paddle.position.x - paddle.width / 2) &&
				(ball[i].position.x <= paddle.position.x + paddle.width / 2))
			{

				//ボールを跳ね返す
				ball[i].velocity.y = -ball[i].speed.y;
				//パドルの右側と当たれば、右側へ跳ね返す
				if ((ball[i].position.x >= paddle.position.x) && (ball[i].velocity.x < 0))
					ball[i].velocity.x = ball[i].speed.x;
				//パドルの左側と当たれば、左側へ跳ね返す
				if ((ball[i].position.x <= paddle.position.x) && (ball[i].velocity.x > 0))
					ball[i].velocity.x = -ball[i].speed.x;

				//パドルが緑なら
				if (paddle.itemNumber == TYPE_BLOCK_GREEN) {
					//パドルに対するボールの距離を計算して、その位置でボールがくっつく
					paddle.greenCatch = true;
					paddle.greenDifference = ball[i].position.x - paddle.position.x;
					ball[i].position.x = paddle.position.x + paddle.greenDifference;
					ball[i].position.y = paddle.position.x;
				}
			}

		}

		//アイテム
		if (item.itemFall) {
			//アイテムを落下させる
			item.position.x += item.velocity.x;
			item.position.y += item.velocity.y;

			//アイテムの中心点
			float itemPointX = item.position.x;
			float itemPointY = item.position.y + BLOCK_HEIGHT / 2;

			//パドルに衝突
			if ((itemPointX >= paddle.position.x - paddle.width / 2) &&
				(itemPointX <= paddle.position.x + paddle.width / 2) &&
				(itemPointY >= paddle.position.y) &&
				(itemPointY <= paddle.position.y + paddle.height))
			{
				if ((item.itemNumber == TYPE_BLOCK_ORANGE) ||
					(item.itemNumber == TYPE_BLOCK_PINK) ||
					(item.itemNumber == TYPE_BLOCK_SILVER))
				{
					if (item.itemNumber == TYPE_BLOCK_ORANGE) {
						//ボールの速度を1割減にする
						for (int i = 0;i < BALL_MAX;i++) {
							ball[i].speed.x *= 0.9f;
							ball[i].speed.y *= 0.9f;

							ball[i].velocity.x *= 0.9f;
							ball[i].velocity.y *= 0.9f;
						}
					}
					if (item.itemNumber == TYPE_BLOCK_PINK) {
						/*
							画面の右端に扉が出現する。その付近で左クリックすれば、
							扉が開き、中に入ればクリアになる(プログラム初期化)
						*/
						paddle.pinkEscape = true;
					}
					if (item.itemNumber == TYPE_BLOCK_SILVER) {
						//パドルのライフを増やす
						paddle.life++;
					}
				}
				else {
					paddle.itemNumber = item.itemNumber;

					//1度元の能力に戻す
					playerNormalMode();

					//パドルの色(緑,水色,青,赤)ごとに能力を得る
					if (paddle.itemNumber == TYPE_BLOCK_GREEN) {}
					if (paddle.itemNumber == TYPE_BLOCK_LBLUE) {
						int count = 0;
						for (int j = 0;j < BALL_MAX;j++) {
							//ボールを2つ増やせば終わる
							if (count >= 2) break;

							/*
								ボールの数を1から3に増やす。画面にない他のボールは落下状態に
								しているので、そのフラグを下ろす。
							*/
							if (ball[j].fall) {
								ball[j].position = {
									12 * BLOCK_WIDTH / 2,
									25 * BLOCK_HEIGHT
								};
								ball[j].velocity = {
									rand() % 2 == 0 ? ball[j].speed.x : -ball[j].speed.x,
									-ball[j].speed.y
								};
								ball[j].fall = false;
								count++;
							}
						}
					}
					if (paddle.itemNumber == TYPE_BLOCK_BLUE) {
						//パドルの長さを1.5倍にする
						paddle.width *= 1.5f;
					}
					if (paddle.itemNumber == TYPE_BLOCK_RED) {
						for (int j = 0;j < BALL_MAX;j++) {
							//落下してないボールは銃弾フラグを下ろす
							if (!ball[j].fall) ball[j].bullet = false;
							//それ以外は銃弾(4発)にする。
							else {
								ball[j].fall = false;
								ball[j].bullet = true;
							}
						}
					}

				}

				//アイテムの落下が終わればフラグを下ろす
				item.itemFall = false;
			}

			//アイテムが一番下に落ちればフラグを下ろす
			if (item.position.y >= BLOCK_HEIGHT * WALL_ROW_MAX)
				item.itemFall = false;
		}

		//ゲームオーバー判定
		if (ballFallCount >= BALL_MAX) {
			//ボールが全て落下すればパドルのライフを減らす
			paddle.life--;

			//パドルのライフが残っていたら、ブロックはそのままで再スタート
			if (paddle.life >= 1) {
				for (int i = 0;i < BALL_MAX;i++) {
					if (paddle.itemNumber == TYPE_BLOCK_RED) {
						if ((ball[i].fall) && (!ball[i].bullet)) ball[i].bullet = false;
						else ball[i].bullet = true;
						ball[i].fall = false;
					}
					else {
						ball[i].bullet = false;
						if (i == 0) ball[i].fall = false;
						else ball[i].fall = true;
					}

					ball[i].position = {
						12 * BLOCK_WIDTH / 2,
						25 * BLOCK_HEIGHT
					};
					ball[i].velocity = {
						rand() % 2 == 0 ? ball[i].speed.x : -ball[i].speed.x,
						-ball[i].speed.y
					};
				}
			}
			//パドルのライフがなくなれば、プログラムを初期化
			else Init();
		}

		//画面のブロックが全て無くなればクリア、プログラムを初期化
		if (paddle.blockCount <= 0) Init();

		glutPostRedisplay();
	}
}

//マウスの動きがパドルの動きに連動
void PassiveMotion(int _x, int _y) {
	//マウスポイントのx方向の動きに応じてパドルが動く

	//パドルの右端と左端のx座標
	float paddleRightX = (float)_x + paddle.width / 2;
	float paddleLeftX = (float)_x - paddle.width / 2;

	//パドルと壁の当たり判定
	if (paddleRightX > 23 * BLOCK_WIDTH / 2) {
		/*
			画面の右端に扉が出現する。その付近で左クリックすれば、
			扉が開き、中に入ればクリアになる(プログラム初期化)
		*/
		if (!paddle.pinkOpen)
			paddle.position.x = 23 * BLOCK_WIDTH / 2 - (float)paddle.width / 2;
		else {
			if (paddleRightX > 25 * BLOCK_WIDTH / 2) Init();
			else paddle.position.x = (float)_x;
		}
	}
	else if (paddleLeftX < BLOCK_WIDTH / 2)
		paddle.position.x = BLOCK_WIDTH / 2 + (float)paddle.width / 2;
	//パドルが壁に当たらない限り、マウスポイントのx座標がパドルのx座標
	else
		paddle.position.x = (float)_x;
}

//マウスの左クリックによる処理
void Mouse(int, int _mouse, int, int) {
	//クリックされてないときは処理終了
	if (_mouse != GLUT_DOWN)
		return;

	//パドルが赤ならクリックすることで銃弾のボールが発射
	if (paddle.itemNumber == TYPE_BLOCK_RED) {
		/*
			発射地点はパドルの右端と左端にあるブロック。
			shotPoint = 0なら左、shotPoint = 1なら右が発射地点。
			銃弾は2連装で、2連射で、弾数4発。
		*/
		int shotPoint = 0;
		for (int i = 0;i < BALL_MAX;i++) {
			//2連装
			if (shotPoint >= 2) break;

			if ((ball[i].bullet) && (!ball[i].redShot)) {
				ball[i].redShot = true;
				ball[i].position = {
					shotPoint == 0
					? paddle.position.x - paddle.width / 2 + BLOCK_WIDTH / 4
					: paddle.position.x + paddle.width / 2 - BLOCK_WIDTH / 4,
					paddle.position.y
				};
				shotPoint++;
				ball[i].velocity = {
					0,
					-2.0f
				};
			}
		}
	}

	//パドルが緑なら、パドルに付いたボールをクリックで飛ばすことができる
	if ((paddle.itemNumber == TYPE_BLOCK_GREEN) && (paddle.greenCatch))
		paddle.greenCatch = false;

	//右端の壁にドアがあれば、付近でクリックすると開き、入ればクリアとなる(プログラム初期化)
	float paddleRight = paddle.position.x + paddle.width / 2;
	if ((paddle.pinkEscape) && (paddleRight > 22 * BLOCK_WIDTH / 2))
		paddle.pinkOpen = true;
}

int main(int argc, char* argv[]) {
	srand((unsigned int)time(NULL));
	glutInit(&argc, argv);
	glutInitDisplayMode(GL_DOUBLE);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutCreateWindow("ARKANOID");
	glutDisplayFunc(Display);
	glutIdleFunc(Idle);
	glutPassiveMotionFunc(PassiveMotion);
	glutMouseFunc(Mouse);
	Init();
	glutMainLoop();
}