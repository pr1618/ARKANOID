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

//�u���b�N�̎��
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

//���Ԑ���
typedef struct {
	clock_t lastUpdate;	 //�Ō�̌v������
	clock_t nowUpdate;	 //�ŐV�̌v������
	float updateInterval;//���ԊԊu
}CLOCK;

CLOCK clockMode;

//2�����x�N�g��
typedef struct {
	float x, y;
}VEC2;

//�p�h��
typedef struct {
	VEC2 position;			//���W
	int itemNumber;			//�ԍ����Ƃɔ\�͕ω�

	bool pinkEscape;		//��ʊO�ւ̒E�o�t���O
	bool pinkOpen;			//��ʊO�ւ̔��J�t���O
	bool greenCatch;		//�p�h���Ƀ{�[������t���t���O
	float greenDifference;	//�p�h���ɑ΂���{�[���̋���

	int life;				//�p�h���̎c�@
	int blockCount;			//��ʏ�̃u���b�N�̐�
	float width;			//�p�h���̕�
	float height;			//�p�h���̍���
}PADDLE;

PADDLE paddle;

//�{�[��
typedef struct {
	VEC2 position;		//���W
	VEC2 lastPosition;	//�Â����W
	VEC2 velocity;		//���x
	VEC2 speed;			//����
	bool fall;			//�����t���O
	bool bullet;		//�e�e�t���O
	bool redShot;		//���˃t���O
}BALL;

BALL ball[BALL_MAX];

//�u���b�N
typedef struct {
	int type;		//�u���b�N�̐F����
	int durability;	//�u���b�N�̑ϋv�l
	int itemNumber;	//�ԍ������u���b�N���󂷂ƃA�C�e�����o�Ă���
}BLOCK;

BLOCK blocks[BLOCK_ROW_MAX][BLOCK_COLUMN_MAX];

//�A�C�e��
typedef struct {
	VEC2 position;	//���W
	VEC2 velocity;	//���x
	bool itemFall;	//�������t���O
	int itemNumber;	//�A�C�e�����p�h���ɓ�����Δԍ���n��
}ITEM;

ITEM item;

//�L�[�{�[�h256���̃o�C�g�z��
bool keysPressed[256];

//�\�͕ω��O�̏��
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

//�{�[�����u���b�N�ƏՓ˂������̔���
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

//�{�[���̋O�����ǂ̐���(�u���b�N�̕�)�ƌ������Ă邩�̔���
bool ballCrossLine(int _ballNumber, int _columnNumber, int _rowNumber, VEC2 _blockStartPoint, VEC2 _blockEndPoint) {
	//2�̐������������Ă邩�O�ς��g���Ĕ���

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

//�{�[���𒵂˕Ԃ�
void ballRebound(int _ballNumber, int _columnNumber, int _rowNumber) {
	//�u���b�N��4�_�����߂�

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

//���ԊǗ�
bool timePasses(clock_t _lastClock, clock_t _nowClock, float _interval) {
	_nowClock = clock();
	if (_nowClock >= _lastClock + _interval) return true;
	else return false;
}

//�u���b�N�̐F��I��
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

//�v���O�����̏�����
void Init(void) {
	//�u���b�N�̏�����
	paddle.blockCount = 0;
	for (int y = 0;y < BLOCK_ROW_MAX;y++)
		for (int x = 0;x < BLOCK_COLUMN_MAX;x++) {
			//�u���b�N�̐F����
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

	//�A�C�e���̏�����
	for (int i = TYPE_BLOCK_ORANGE;i <= TYPE_BLOCK_SILVER;i++) {
		//�����_���Ńu���b�N�ɃA�C�e���i���o�[��n��
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

	//�p�h���̏�����
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

	//�{�[���̏�����
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

	//����
	clockMode.updateInterval = 1000 / 60;
}

//�u���b�N��p�h���̕`��
void Display(void) {
	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//�w�i�̕`��
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

	//�u���b�N�̕`��
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

	//�p�h���̕`��
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

	//�A�C�e��	
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

	//�{�[���̕`��
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

	//�~�j�p�h��
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

	//�ǂ̕`��
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

//�{�[����u���b�N�A�A�C�e���̋�������������
void Idle(void) {
	if (timePasses(clockMode.lastUpdate, clockMode.nowUpdate, clockMode.updateInterval)) {

		//�{�[��
		int ballFallCount = 0;//�{�[���J�E���g�̏�����
		for (int i = 0;i < BALL_MAX;i++) {
			//�������e�Ȃ�J�E���g
			if ((ball[i].fall) || (ball[i].bullet)) {
				ballFallCount++;
				//�����̃{�[���͏��O
				if (ball[i].fall) continue;
			}

			//���݂̍��W��ۑ�
			ball[i].lastPosition = ball[i].position;

			if ((paddle.itemNumber == TYPE_BLOCK_GREEN) || (paddle.itemNumber == TYPE_BLOCK_RED)) {
				//�p�h�����΂Ȃ�{�[�����p�h���ɒ���t��
				if (paddle.itemNumber == TYPE_BLOCK_GREEN) {
					//�p�h���ɑ΂���{�[���̋������v�Z
					if (paddle.greenCatch) {
						ball[i].position.x = paddle.position.x + paddle.greenDifference;
						ball[i].position.y = paddle.position.y;
						continue;
					}
					//����t���ĂȂ��Ȃ�{�[�����΂�
					else {
						ball[i].position.x += ball[i].velocity.x;
						ball[i].position.y += ball[i].velocity.y;
					}
				}
				//�p�h�����ԂȂ�{�[�����e�e�ɂȂ�
				if (paddle.itemNumber == TYPE_BLOCK_RED) {
					//�e�e�̃{�[��
					if (ball[i].bullet) {
						//�e�e�͍��N���b�N�Ŕ��
						if (ball[i].redShot) {
							ball[i].position.x += ball[i].velocity.x;
							ball[i].position.y += ball[i].velocity.y;
						}
						else continue;
					}
					//���ʂ̃{�[��
					else {
						ball[i].position.x += ball[i].velocity.x;
						ball[i].position.y += ball[i].velocity.y;
					}
				}
			}
			else {
				//�p�h�������̑��̐F�Ȃ�
				ball[i].position.x += ball[i].velocity.x;
				ball[i].position.y += ball[i].velocity.y;
			}

			//�{�[�����u���b�N�ɓ������
			int columnNumber, rowNumber;
			if (ballHitsBlock(i, &columnNumber, &rowNumber)) {
				//�u���b�N�̑ϋv�l��������
				blocks[rowNumber][columnNumber].durability--;

				//�e�e�̃{�[���Ȃ璵�˕Ԃ����e�e������
				if (ball[i].bullet) ball[i].redShot = false;
				//���ʂ̃{�[���Ȃ璵�˕Ԃ�
				else ballRebound(i, columnNumber, rowNumber);

				//�u���b�N�̑ϋv�l��0�Ȃ�
				if (blocks[rowNumber][columnNumber].durability <= 0) {
					//�u���b�N������
					blocks[rowNumber][columnNumber].type = TYPE_BLOCK_NONE;
					//�u���b�N�̐������炷
					paddle.blockCount--;

					//�u���b�N�̒��ɃA�C�e�����B����Ă����
					if ((blocks[rowNumber][columnNumber].itemNumber >= 0) && (!item.itemFall)) {
						//true�̊Ԃ͕ʂ̃A�C�e���͗����Ă��Ȃ�
						item.itemFall = true;
						//�u���b�N�̎��A�C�e���ԍ����󂯎��A�p�h���ɓ�����Δԍ���n��
						item.itemNumber = blocks[rowNumber][columnNumber].itemNumber;
						//���W
						item.position = {
							(float)BLOCK_WIDTH / 2 + columnNumber * BLOCK_WIDTH + BLOCK_WIDTH / 2,
							(float)BLOCK_HEIGHT + rowNumber * BLOCK_HEIGHT
						};
						//���x
						item.velocity = { 0,0.25f };
					}
				}
			}

			//��
			{
				//�ǂɓ�����Β��˕Ԃ�
				if (ball[i].position.x > 23 * BLOCK_WIDTH / 2) {
					ball[i].position.x = 23 * BLOCK_WIDTH / 2;
					ball[i].velocity.x *= -1;
				}
				if (ball[i].position.x < BLOCK_WIDTH / 2) {
					ball[i].position.x = BLOCK_WIDTH / 2;
					ball[i].velocity.x *= -1;
				}
				if (ball[i].position.y < BLOCK_HEIGHT) {
					//�e�e�̃{�[����������Ώ���
					if (ball[i].bullet) ball[i].redShot = false;
					else {
						ball[i].position.y = BLOCK_HEIGHT;
						ball[i].velocity.y *= -1;
					}
				}
				//�������J�E���g����
				if (ball[i].position.y > 28 * BLOCK_HEIGHT) {
					ball[i].fall = true;
					ballFallCount++;
				}
			}

			//�p�h���ƃ{�[�����������
			if ((ball[i].position.y >= paddle.position.y) &&
				(ball[i].position.y <= paddle.position.y + paddle.height) &&
				(ball[i].position.x >= paddle.position.x - paddle.width / 2) &&
				(ball[i].position.x <= paddle.position.x + paddle.width / 2))
			{

				//�{�[���𒵂˕Ԃ�
				ball[i].velocity.y = -ball[i].speed.y;
				//�p�h���̉E���Ɠ�����΁A�E���֒��˕Ԃ�
				if ((ball[i].position.x >= paddle.position.x) && (ball[i].velocity.x < 0))
					ball[i].velocity.x = ball[i].speed.x;
				//�p�h���̍����Ɠ�����΁A�����֒��˕Ԃ�
				if ((ball[i].position.x <= paddle.position.x) && (ball[i].velocity.x > 0))
					ball[i].velocity.x = -ball[i].speed.x;

				//�p�h�����΂Ȃ�
				if (paddle.itemNumber == TYPE_BLOCK_GREEN) {
					//�p�h���ɑ΂���{�[���̋������v�Z���āA���̈ʒu�Ń{�[����������
					paddle.greenCatch = true;
					paddle.greenDifference = ball[i].position.x - paddle.position.x;
					ball[i].position.x = paddle.position.x + paddle.greenDifference;
					ball[i].position.y = paddle.position.x;
				}
			}

		}

		//�A�C�e��
		if (item.itemFall) {
			//�A�C�e���𗎉�������
			item.position.x += item.velocity.x;
			item.position.y += item.velocity.y;

			//�A�C�e���̒��S�_
			float itemPointX = item.position.x;
			float itemPointY = item.position.y + BLOCK_HEIGHT / 2;

			//�p�h���ɏՓ�
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
						//�{�[���̑��x��1�����ɂ���
						for (int i = 0;i < BALL_MAX;i++) {
							ball[i].speed.x *= 0.9f;
							ball[i].speed.y *= 0.9f;

							ball[i].velocity.x *= 0.9f;
							ball[i].velocity.y *= 0.9f;
						}
					}
					if (item.itemNumber == TYPE_BLOCK_PINK) {
						/*
							��ʂ̉E�[�ɔ����o������B���̕t�߂ō��N���b�N����΁A
							�����J���A���ɓ���΃N���A�ɂȂ�(�v���O����������)
						*/
						paddle.pinkEscape = true;
					}
					if (item.itemNumber == TYPE_BLOCK_SILVER) {
						//�p�h���̃��C�t�𑝂₷
						paddle.life++;
					}
				}
				else {
					paddle.itemNumber = item.itemNumber;

					//1�x���̔\�͂ɖ߂�
					playerNormalMode();

					//�p�h���̐F(��,���F,��,��)���Ƃɔ\�͂𓾂�
					if (paddle.itemNumber == TYPE_BLOCK_GREEN) {}
					if (paddle.itemNumber == TYPE_BLOCK_LBLUE) {
						int count = 0;
						for (int j = 0;j < BALL_MAX;j++) {
							//�{�[����2���₹�ΏI���
							if (count >= 2) break;

							/*
								�{�[���̐���1����3�ɑ��₷�B��ʂɂȂ����̃{�[���͗�����Ԃ�
								���Ă���̂ŁA���̃t���O�����낷�B
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
						//�p�h���̒�����1.5�{�ɂ���
						paddle.width *= 1.5f;
					}
					if (paddle.itemNumber == TYPE_BLOCK_RED) {
						for (int j = 0;j < BALL_MAX;j++) {
							//�������ĂȂ��{�[���͏e�e�t���O�����낷
							if (!ball[j].fall) ball[j].bullet = false;
							//����ȊO�͏e�e(4��)�ɂ���B
							else {
								ball[j].fall = false;
								ball[j].bullet = true;
							}
						}
					}

				}

				//�A�C�e���̗������I���΃t���O�����낷
				item.itemFall = false;
			}

			//�A�C�e������ԉ��ɗ�����΃t���O�����낷
			if (item.position.y >= BLOCK_HEIGHT * WALL_ROW_MAX)
				item.itemFall = false;
		}

		//�Q�[���I�[�o�[����
		if (ballFallCount >= BALL_MAX) {
			//�{�[�����S�ė�������΃p�h���̃��C�t�����炷
			paddle.life--;

			//�p�h���̃��C�t���c���Ă�����A�u���b�N�͂��̂܂܂ōăX�^�[�g
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
			//�p�h���̃��C�t���Ȃ��Ȃ�΁A�v���O������������
			else Init();
		}

		//��ʂ̃u���b�N���S�Ė����Ȃ�΃N���A�A�v���O������������
		if (paddle.blockCount <= 0) Init();

		glutPostRedisplay();
	}
}

//�}�E�X�̓������p�h���̓����ɘA��
void PassiveMotion(int _x, int _y) {
	//�}�E�X�|�C���g��x�����̓����ɉ����ăp�h��������

	//�p�h���̉E�[�ƍ��[��x���W
	float paddleRightX = (float)_x + paddle.width / 2;
	float paddleLeftX = (float)_x - paddle.width / 2;

	//�p�h���ƕǂ̓����蔻��
	if (paddleRightX > 23 * BLOCK_WIDTH / 2) {
		/*
			��ʂ̉E�[�ɔ����o������B���̕t�߂ō��N���b�N����΁A
			�����J���A���ɓ���΃N���A�ɂȂ�(�v���O����������)
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
	//�p�h�����ǂɓ�����Ȃ�����A�}�E�X�|�C���g��x���W���p�h����x���W
	else
		paddle.position.x = (float)_x;
}

//�}�E�X�̍��N���b�N�ɂ�鏈��
void Mouse(int, int _mouse, int, int) {
	//�N���b�N����ĂȂ��Ƃ��͏����I��
	if (_mouse != GLUT_DOWN)
		return;

	//�p�h�����ԂȂ�N���b�N���邱�Ƃŏe�e�̃{�[��������
	if (paddle.itemNumber == TYPE_BLOCK_RED) {
		/*
			���˒n�_�̓p�h���̉E�[�ƍ��[�ɂ���u���b�N�B
			shotPoint = 0�Ȃ獶�AshotPoint = 1�Ȃ�E�����˒n�_�B
			�e�e��2�A���ŁA2�A�˂ŁA�e��4���B
		*/
		int shotPoint = 0;
		for (int i = 0;i < BALL_MAX;i++) {
			//2�A��
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

	//�p�h�����΂Ȃ�A�p�h���ɕt�����{�[�����N���b�N�Ŕ�΂����Ƃ��ł���
	if ((paddle.itemNumber == TYPE_BLOCK_GREEN) && (paddle.greenCatch))
		paddle.greenCatch = false;

	//�E�[�̕ǂɃh�A������΁A�t�߂ŃN���b�N����ƊJ���A����΃N���A�ƂȂ�(�v���O����������)
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