#include <player.h>
#include <stdio.h>
#include <string.h>
#include "stm32746g_discovery_lcd.h"
#include "fonts.h"
#include "stm32746g_discovery_ts.h"
#include "term_io.h"
#include "dbgu.h"
#include "ansi.h"
#include "stm32f7xx_hal.h"

#define LCD_X_SIZE			RK043FN48H_WIDTH
#define LCD_Y_SIZE			RK043FN48H_HEIGHT
#define LAYER_FG 1
#define LAYER_BG 0

/* derektywy konfigurowalne */

#define BUTTON_SCOPE 0.16
#define BUTTON_SIZE BUTTON_SCOPE * LCD_X_SIZE
#define BUTTONS_NUMBER 6

#define TICKS_DELTA 100

#define XPix(x) x * LCD_X_SIZE
#define YPix(x) x * LCD_Y_SIZE

#define BG_COLOR LCD_COLOR_VERYDARKGRAY
#define FG_COLOR BG_COLOR
#define BUTTON_COLOR LCD_COLOR_DARKGREEN
#define TEXT_COLOR LCD_COLOR_WHITE
#define BAR_COLOR LCD_COLOR_WHITE
#define VOLUME_BAR_COLOR LCD_COLOR_ORANGE
#define TIME_COLOR LCD_COLOR_WHITE

#define BUTTON_Y_POSITION 0.6
#define BAR_Y_POSITION 0.91
#define TEXT_Y_POSITION 0.5

/* zmienne globalne */

static TS_StateTypeDef TS_State;
static TS_StateTypeDef lastState;
uint16_t newX;
uint16_t newY;
uint16_t buttonsLeftUpCorner[BUTTONS_NUMBER][2] = {
	{XPix(0.08), YPix(BUTTON_Y_POSITION)},
	{XPix(0.31), YPix(BUTTON_Y_POSITION)},
	{XPix(0.54), YPix(BUTTON_Y_POSITION)},
	{XPix(0.77), YPix(BUTTON_Y_POSITION)},
	{0, 0},
	{XPix(0.9), 0}
};

Player_State buttonState[BUTTONS_NUMBER] = { PREV, PLAY, STOP, NEXT, VOLUMEDOWN, VOLUMEUP };
Player_State playButtonState = PLAY;
uint32_t lastTicks = 0;

/* funkcje interfejsu */

void lcd_start();
void draw_background();
int initialize_touchscreen();
void touchscreen_loop_init();
Player_State check_touchscreen();
void update_progress_bar(double);
void update_actual_time(int);
void refresh_screen(const char *info_text);
void draw_buttons();
void update_play_pause_button();


void lcd_start() {

	BSP_LCD_Init();

	BSP_LCD_LayerDefaultInit(LAYER_BG, (unsigned int)0xC0000000);
	BSP_LCD_LayerDefaultInit(LAYER_FG, (unsigned int)0xC0000000+(LCD_X_SIZE*LCD_Y_SIZE*8));

	BSP_LCD_DisplayOn();

	BSP_LCD_SelectLayer(LAYER_BG);

	BSP_LCD_Clear(BG_COLOR);
	BSP_LCD_SetBackColor(BG_COLOR);

	BSP_LCD_SelectLayer(LAYER_FG);

	BSP_LCD_SetColorKeying(LAYER_FG,BG_COLOR);
	BSP_LCD_Clear(BG_COLOR);
	BSP_LCD_SetBackColor(BG_COLOR);

	BSP_LCD_SetTransparency(LAYER_BG, 0xFF);
	BSP_LCD_SetTransparency(LAYER_FG, 0xFF);
}

void draw_background() {

	BSP_LCD_SelectLayer(LAYER_BG);
	draw_buttons();
	BSP_LCD_SelectLayer(LAYER_FG);
}

int initialize_touchscreen() {

	uint8_t status = 0;
	status = BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
	if(status != TS_OK) return -1;
	return 0;
}

void touchscreen_loop_init() {

    newX = 120;
	newY = 120;
    BSP_LCD_SelectLayer(LAYER_FG);
	BSP_LCD_SetTransparency(LAYER_FG, (uint8_t) 0xFF);
	BSP_LCD_SetColorKeying(LAYER_FG, BG_COLOR);
}

Player_State check_touchscreen() {

	uint32_t currentTicks = HAL_GetTick();
	
	if (currentTicks < lastTicks + TICKS_DELTA) {
		return EMPTY;
	}
	
	lastTicks = currentTicks;
	
    BSP_TS_GetState(&TS_State);
	if (TS_State.touchDetected == 0) {
		return EMPTY;
	}
	if ((TS_State.touchX[0] & 0x0FFF) >= 10) {
		newX = TS_State.touchX[0] & 0x0FFF;
	}
	if ((TS_State.touchY[0] & 0x0FFF) >= 10) {
		newY = TS_State.touchY[0] & 0x0FFF;
	}

	lastState.touchX[0] = newX;
	lastState.touchY[0] = newY;

	for (int i = 0; i < BUTTONS_NUMBER; i++) {

		uint16_t buttonCornerX = buttonsLeftUpCorner[i][0];
		uint16_t buttonCornerY = buttonsLeftUpCorner[i][1];

		if (newX <= buttonCornerX + BUTTON_SIZE && newY <= buttonCornerY + BUTTON_SIZE && newX > buttonCornerX && newY > buttonCornerY) {
			if (i == 1 || i == 2) {
				if (playButtonState == PLAY) {
					playButtonState = PAUSE;
				} else {
					playButtonState = PLAY;
				}
			}
			if (i == 1) {
				return playButtonState;
			} else if (i == 2) {
				playButtonState = PAUSE;
				return STOP;
			} else {
				if (buttonState[i] == PREV || buttonState[i] == NEXT) {
					playButtonState = PLAY;
				}
				return buttonState[i];
			}
		}
	}
	return EMPTY;
}

void update_actual_time(int time) {

	BSP_LCD_SelectLayer(LAYER_FG);
	BSP_LCD_SetTextColor(BG_COLOR);
	BSP_LCD_FillRect(0, LCD_Y_SIZE - 75, 100, 75);
	BSP_LCD_SetTextColor(TIME_COLOR);
	char buf[50];
	sprintf(buf, " %d:%d%d", time/60, (time%60)/10, time%10);
	BSP_LCD_DisplayStringAt(0, LCD_Y_SIZE - 34, (unsigned char *) buf, LEFT_MODE);
}

void show_full_time(int time) {

	BSP_LCD_SelectLayer(LAYER_FG);
	BSP_LCD_SetTextColor(BG_COLOR);
	BSP_LCD_FillRect(LCD_X_SIZE-100, LCD_Y_SIZE - 75, 100, 75);
	BSP_LCD_SetTextColor(TIME_COLOR);
	char buf[50];
	sprintf(buf, " %d:%d%d", time/60, (time%60)/10, time%10);
	BSP_LCD_DisplayStringAt(LCD_X_SIZE-100, LCD_Y_SIZE - 34, (unsigned char *) buf, LEFT_MODE);
}

void update_progress_bar(double progress) {

	BSP_LCD_SelectLayer(LAYER_FG);

	double epsilon = 1e-2;
	if(progress <= epsilon) {
		BSP_LCD_SetTextColor(BG_COLOR);
		BSP_LCD_FillRect(100, YPix(BAR_Y_POSITION), LCD_X_SIZE - 200, 7);
		return;
	}

	BSP_LCD_SetTextColor(BAR_COLOR);
	BSP_LCD_FillRect(100, YPix(BAR_Y_POSITION),  (uint16_t)(progress * (LCD_X_SIZE - 200)), 5);
}

void update_volume_bar(int volume) {

	BSP_LCD_SelectLayer(LAYER_FG);
	BSP_LCD_SetTextColor(FG_COLOR);
	BSP_LCD_FillRect(44, 29, LCD_X_SIZE - 88, 17);
	BSP_LCD_SetTextColor(VOLUME_BAR_COLOR);
	BSP_LCD_FillRect(45, 30, (uint16_t)(volume * (LCD_X_SIZE - 90)/100), 15);
}

void refresh_screen(const char *info_text) {

	// Wypisz nazwę utworu lub komunikat
	BSP_LCD_SelectLayer(LAYER_FG);
	BSP_LCD_SetTextColor(BG_COLOR);
	BSP_LCD_FillRect(0, YPix(TEXT_Y_POSITION), LCD_X_SIZE, 40);
	BSP_LCD_SetTextColor(TEXT_COLOR);

	char buf[100];
	sprintf(buf, " %s", info_text);
	BSP_LCD_DisplayStringAt(0, YPix(TEXT_Y_POSITION), (unsigned char *)buf,CENTER_MODE);

	// Odśwież przycisk start/pauza
	update_play_pause_button();
}

void draw_buttons() {

	BSP_LCD_SetTextColor(BUTTON_COLOR);
	uint16_t xButton, yButton;
	
	// Przyciski głośności
	BSP_LCD_SetTextColor(BUTTON_COLOR);
	BSP_LCD_DisplayStringAt(15, 29, (unsigned char *)"-", LEFT_MODE);
	BSP_LCD_DisplayStringAt(LCD_X_SIZE - 30, 29, (unsigned char *)"+", LEFT_MODE);

	// Przycisk poprzedniego utworu
	xButton = buttonsLeftUpCorner[0][0];
	yButton = buttonsLeftUpCorner[0][1];
	Point Points1[]= {{xButton + 7, yButton + BUTTON_SIZE / 2}, {xButton + BUTTON_SIZE / 2, yButton + 7}, 
		{xButton + BUTTON_SIZE / 2, yButton + 7 + BUTTON_SIZE*0.806}};
	BSP_LCD_FillPolygon(Points1, 3);
	Points1[0].X += BUTTON_SIZE*0.4;
	Points1[1].X += BUTTON_SIZE*0.4;
	Points1[2].X += BUTTON_SIZE*0.4;
	BSP_LCD_FillPolygon(Points1, 3);

	// Przyciski start/pauza
	update_play_pause_button();

	// Przycisk stopu
	BSP_LCD_SetTextColor(BUTTON_COLOR);
	xButton = buttonsLeftUpCorner[2][0];
	yButton = buttonsLeftUpCorner[2][1];
	BSP_LCD_FillRect(xButton, yButton + 7, BUTTON_SIZE - 14, BUTTON_SIZE - 14);

	// Przycisk następnego utworu
	xButton = buttonsLeftUpCorner[3][0];
	yButton = buttonsLeftUpCorner[3][1];
	Point Points3[]= {{xButton + 7, yButton + 7}, {xButton + 7, yButton + 7 + BUTTON_SIZE*0.806},
		{xButton + BUTTON_SIZE / 2 , yButton + BUTTON_SIZE / 2}};
	BSP_LCD_FillPolygon(Points3, 3);
	Points3[0].X += BUTTON_SIZE*0.4;
	Points3[1].X += BUTTON_SIZE*0.4;
	Points3[2].X += BUTTON_SIZE*0.4;
	BSP_LCD_FillPolygon(Points3, 3);
}

void update_play_pause_button() {

	BSP_LCD_SelectLayer(LAYER_BG);
	uint16_t xButton = buttonsLeftUpCorner[1][0];
	uint16_t yButton = buttonsLeftUpCorner[1][1];
	BSP_LCD_SetTextColor(BG_COLOR);
	BSP_LCD_FillRect(xButton, yButton, BUTTON_SIZE, BUTTON_SIZE);
	BSP_LCD_SetTextColor(BUTTON_COLOR);
	if(playButtonState == PAUSE) {
		// Przycisk startu
		Point Points2[]= {{xButton + 7, yButton + 7}, {xButton + 7, yButton + 7 + 0.806*BUTTON_SIZE},
			{xButton + 7 + 58, yButton + BUTTON_SIZE / 2}};
		BSP_LCD_FillPolygon(Points2, 3);
	} else if(playButtonState == PLAY) {
		// Przycisk pauzy
		BSP_LCD_FillRect(xButton + 7, yButton + 7, BUTTON_SIZE*0.27, BUTTON_SIZE - 14);
		BSP_LCD_FillRect(xButton + 7 + BUTTON_SIZE*0.431, yButton + 7, BUTTON_SIZE*0.27, BUTTON_SIZE - 14);
	}

}
