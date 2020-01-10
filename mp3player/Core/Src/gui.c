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

/* stale konfigurowalne */
#define CONTROL_BUTTON_SCALE 0.16
#define CONTROL_BUTTON_SIZE CONTROL_BUTTON_SCALE * LCD_X_SIZE
#define CONTROL_BUTTONS_NUMBER 6

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
uint16_t buttonsLeftUpper[CONTROL_BUTTONS_NUMBER][2] = {
	{XPix(0.08), YPix(BUTTON_Y_POSITION)},
	{XPix(0.31), YPix(BUTTON_Y_POSITION)},
	{XPix(0.54), YPix(BUTTON_Y_POSITION)},
	{XPix(0.77), YPix(BUTTON_Y_POSITION)},
	{0, 0},
	{XPix(0.9), 0}
};
Player_State buttonState[CONTROL_BUTTONS_NUMBER] = { PREV, PLAY, STOP, NEXT, VOLUMEDOWN, VOLUMEUP };
Player_State playButtonState = PLAY;
uint32_t lastTicks = 0;

/* funkcje interfejsu */

void lcd_start(void);
void draw_background(void);
int initialize_touchscreen(void);
void touchscreen_loop_init(void);
Player_State check_touchscreen();
void update_progress_bar(double);
void update_actual_time(int);
void refresh_screen(const char *info_text);
void draw_buttons(void);
void update_play_pause_button(void);


void lcd_start(void)
{
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

void draw_background(void)
{
	BSP_LCD_SelectLayer(LAYER_BG);
	draw_buttons();
	
	BSP_LCD_SelectLayer(LAYER_FG);
}

int initialize_touchscreen(void)
{
	uint8_t status = 0;
	status = BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
	if(status != TS_OK) return -1;
	return 0;
}

void touchscreen_loop_init(void)
{
    newX = 120;
	newY = 120;
    BSP_LCD_SelectLayer(LAYER_FG);
	BSP_LCD_SetTransparency(LAYER_FG, (uint8_t) 0xFF);
	BSP_LCD_SetColorKeying(LAYER_FG, BG_COLOR);
}

Player_State check_touchscreen()
{	
	uint32_t currentTicks = HAL_GetTick();
	
	if (currentTicks < lastTicks + TICKS_DELTA)
		return EMPTY;
	
	lastTicks = currentTicks;
	
    BSP_TS_GetState(&TS_State);
	if (TS_State.touchDetected == 0)
		return EMPTY;
	if ((TS_State.touchX[0] & 0x0FFF) >= 10)
    {
		newX = TS_State.touchX[0] & 0x0FFF;
	}
	if ((TS_State.touchY[0] & 0x0FFF) >= 10)
    {
		newY = TS_State.touchY[0] & 0x0FFF;
	}

	lastState.touchX[0] = newX;
	lastState.touchY[0] = newY;

	for (int i = 0; i < CONTROL_BUTTONS_NUMBER; i++) {

		uint16_t buttonCornerX = buttonsLeftUpper[i][0];
		uint16_t buttonCornerY = buttonsLeftUpper[i][1];

		if (newX <= buttonCornerX + CONTROL_BUTTON_SIZE &&
			newY <= buttonCornerY + CONTROL_BUTTON_SIZE &&
			newX > buttonCornerX &&
			newY > buttonCornerY) {

			if (i == 1 || i == 2) {
				if (playButtonState == PLAY) {
					playButtonState = PAUSE;
				}

				else {
					playButtonState = PLAY;
				}
			}
			if (i == 1)
				return playButtonState;
			else if (i == 2) {
				playButtonState = PAUSE;
				return STOP;
			}

			else {
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

	// Text
	BSP_LCD_SelectLayer(LAYER_FG);
	BSP_LCD_SetTextColor(BG_COLOR);
	BSP_LCD_FillRect(0, YPix(TEXT_Y_POSITION), LCD_X_SIZE, 40);
	BSP_LCD_SetTextColor(TEXT_COLOR);

	char buf[100];
	sprintf(buf, " %s", info_text);
	BSP_LCD_DisplayStringAt(0, YPix(TEXT_Y_POSITION), (unsigned char *)buf,CENTER_MODE);

	update_play_pause_button();
}

void draw_buttons() {

	BSP_LCD_SetTextColor(BUTTON_COLOR);
	uint16_t xButton, yButton;
	
	xButton = buttonsLeftUpper[0][0];
	yButton = buttonsLeftUpper[0][1];
	Point Points1[]= {{xButton + 7, yButton + CONTROL_BUTTON_SIZE / 2}, {xButton + CONTROL_BUTTON_SIZE / 2, yButton + 7}, 
		{xButton + CONTROL_BUTTON_SIZE / 2, yButton + 7 + CONTROL_BUTTON_SIZE*0.806}};

	BSP_LCD_FillPolygon(Points1, 3);
	Points1[0].X += CONTROL_BUTTON_SIZE*0.4;
	Points1[1].X += CONTROL_BUTTON_SIZE*0.4;
	Points1[2].X += CONTROL_BUTTON_SIZE*0.4;
	BSP_LCD_FillPolygon(Points1, 3);

	update_play_pause_button();

	BSP_LCD_SetTextColor(BUTTON_COLOR);
	BSP_LCD_DisplayStringAt(15, 29, (unsigned char *)"-", LEFT_MODE);
	BSP_LCD_DisplayStringAt(LCD_X_SIZE - 30, 29, (unsigned char *)"+", LEFT_MODE);

	BSP_LCD_SetTextColor(BUTTON_COLOR);
	xButton = buttonsLeftUpper[2][0];
	yButton = buttonsLeftUpper[2][1];
	BSP_LCD_FillRect(xButton, yButton + 7, CONTROL_BUTTON_SIZE - 14, CONTROL_BUTTON_SIZE - 14);

	xButton = buttonsLeftUpper[3][0];
	yButton = buttonsLeftUpper[3][1];
	Point Points3[]= {{xButton + 7, yButton + 7}, {xButton + 7, yButton + 7 + CONTROL_BUTTON_SIZE*0.806},
		{xButton + CONTROL_BUTTON_SIZE / 2 , yButton + CONTROL_BUTTON_SIZE / 2}};
	BSP_LCD_FillPolygon(Points3, 3);
	Points3[0].X += CONTROL_BUTTON_SIZE*0.4;
	Points3[1].X += CONTROL_BUTTON_SIZE*0.4;
	Points3[2].X += CONTROL_BUTTON_SIZE*0.4;
	BSP_LCD_FillPolygon(Points3, 3);
}

void update_play_pause_button() {

	BSP_LCD_SelectLayer(LAYER_BG);
	uint16_t xButton = buttonsLeftUpper[1][0];
	uint16_t yButton = buttonsLeftUpper[1][1];
	BSP_LCD_SetTextColor(BG_COLOR);
	BSP_LCD_FillRect(xButton, yButton, CONTROL_BUTTON_SIZE, CONTROL_BUTTON_SIZE);
	BSP_LCD_SetTextColor(BUTTON_COLOR);
	if(playButtonState == PAUSE) {
		Point Points2[]= {{xButton + 7, yButton + 7}, {xButton + 7, yButton + 7 + 0.806*CONTROL_BUTTON_SIZE},
			{xButton + 7 + 58, yButton + CONTROL_BUTTON_SIZE / 2}};
		BSP_LCD_FillPolygon(Points2, 3);
	} else if(playButtonState == PLAY) {
		BSP_LCD_FillRect(xButton + 7, yButton + 7, CONTROL_BUTTON_SIZE*0.27, CONTROL_BUTTON_SIZE - 14);
		BSP_LCD_FillRect(xButton + 7 + CONTROL_BUTTON_SIZE*0.431, yButton + 7, CONTROL_BUTTON_SIZE*0.27, CONTROL_BUTTON_SIZE - 14);
	}

}
