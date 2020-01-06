/*
 *	Graphical User Interface
 *  Touchscreen
 */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "mp3_player.h"
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

#define CONTROL_BUTTON_SCALE 0.16
#define CONTROL_BUTTON_SIZE CONTROL_BUTTON_SCALE * LCD_X_SIZE
#define CONTROL_BUTTONS_NUMBER 4

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

/* ------------------------------------------------------------------- */

static TS_StateTypeDef TS_State;
static TS_StateTypeDef lastState;
uint16_t newX;
uint16_t newY;
uint16_t buttonsLeftUpper[CONTROL_BUTTONS_NUMBER][2] = {
	{XPix(0.08), YPix(BUTTON_Y_POSITION)},
	{XPix(0.31), YPix(BUTTON_Y_POSITION)},
	{XPix(0.54), YPix(BUTTON_Y_POSITION)},
	{XPix(0.77), YPix(BUTTON_Y_POSITION)} };
Mp3_Player_State buttonState[4] = { PREV, PLAY, STOP, NEXT };
Mp3_Player_State playButtonState = PLAY;
uint32_t lastTicks = 0;

/* ------------------------------------------------------------------- */

void lcd_start(void);
void draw_background(void);
int initialize_touchscreen(void);
void touchscreen_loop_init(void);
Mp3_Player_State check_touchscreen();
void update_progress_bar(double);
void update_actual_time(int);
uint16_t getXPix (double factor);
uint16_t getYPix (double factor);
void refresh_screen(const char *info_text);
void draw_buttons(void);
void update_play_pause_button(void);

/* ------------------------------------------------------------------- */

// Initialize the LCD display (call this only once at the start of the player!)
void lcd_start(void)
{
  /* LCD Initialization */
  BSP_LCD_Init();

  /* LCD Initialization */
  BSP_LCD_LayerDefaultInit(LAYER_BG, (unsigned int)0xC0000000);
  BSP_LCD_LayerDefaultInit(LAYER_FG, (unsigned int)0xC0000000+(LCD_X_SIZE*LCD_Y_SIZE*8));

  /* Enable the LCD */
  BSP_LCD_DisplayOn();

  /* Select the LCD Background Layer  */
  BSP_LCD_SelectLayer(LAYER_BG);

  /* Clear the Background Layer */
  BSP_LCD_Clear(BG_COLOR);
  BSP_LCD_SetBackColor(BG_COLOR);


  /* Select the LCD Foreground Layer  */
  BSP_LCD_SelectLayer(LAYER_FG);

  /* Clear the Foreground Layer */
  BSP_LCD_SetColorKeying(LAYER_FG,BG_COLOR);
  BSP_LCD_Clear(BG_COLOR);
  BSP_LCD_SetBackColor(BG_COLOR);


  /* Configure the transparency for foreground and background :
     Increase the transparency */
  BSP_LCD_SetTransparency(LAYER_BG, 0xFF);
  BSP_LCD_SetTransparency(LAYER_FG, 0xFF);
}

// Draw the screen background
// Should be called only once to draw all the necessary screen elements
void draw_background(void)
{
	/* Select the LCD Background Layer  */
	BSP_LCD_SelectLayer(LAYER_BG);
	draw_buttons();
	
	//select Foreground Layer
	BSP_LCD_SelectLayer(LAYER_FG);
}

// Initialize the touchscreen
// Should be called once, to create all the necessary structures
int initialize_touchscreen(void)
{
	uint8_t status = 0;
	status = BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
	if(status != TS_OK) return -1;
	return 0;
}

// Call this once to init the TS-input-reading-loop
void touchscreen_loop_init(void)
{
    newX = 120;
	newY = 120;
    BSP_LCD_SelectLayer(LAYER_FG);
	BSP_LCD_SetTransparency(LAYER_FG, (uint8_t) 0xFF);
	BSP_LCD_SetColorKeying(LAYER_FG, BG_COLOR);
}

// Single iteration of getting TS input
// RETURNS: new state according to the user choice or EMPTY if no new choice has been made
Mp3_Player_State check_touchscreen()
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

			else
				return buttonState[i];
		}
	}
	if (newX <= 80 && newY <= 80) {
		return VOLUMEDOWN;
	}
	if (newX >= LCD_X_SIZE-80 && newY <= 80) {
		return VOLUMEUP;
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

	// Play/Pause Button (update label)
	update_play_pause_button();
}

// Draw the 4 main state control buttons: PREV/PLAY_PAUSE/STOP/NEXT
void draw_buttons() {

	BSP_LCD_SetTextColor(BUTTON_COLOR);
	uint16_t xButton, yButton;
	
	// Previous button label
	xButton = buttonsLeftUpper[0][0];
	yButton = buttonsLeftUpper[0][1];
	Point Points1[]= {{xButton + 7, yButton + CONTROL_BUTTON_SIZE / 2}, {xButton + CONTROL_BUTTON_SIZE / 2, yButton + 7}, 
		{xButton + CONTROL_BUTTON_SIZE / 2, yButton + 7 + CONTROL_BUTTON_SIZE*0.806}};

	// Control button size in pixels is 72
	BSP_LCD_FillPolygon(Points1, 3);
	Points1[0].X += CONTROL_BUTTON_SIZE*0.4;
	Points1[1].X += CONTROL_BUTTON_SIZE*0.4;
	Points1[2].X += CONTROL_BUTTON_SIZE*0.4;
	BSP_LCD_FillPolygon(Points1, 3);

	// Play/Pause button label
	update_play_pause_button();

	// Volume control button
	BSP_LCD_SetTextColor(BUTTON_COLOR);
	BSP_LCD_DisplayStringAt(15, 29, (unsigned char *)"-", LEFT_MODE);
	BSP_LCD_DisplayStringAt(LCD_X_SIZE - 30, 29, (unsigned char *)"+", LEFT_MODE);

	// Stop button label
	BSP_LCD_SetTextColor(BUTTON_COLOR);
	xButton = buttonsLeftUpper[2][0];
	yButton = buttonsLeftUpper[2][1];
	BSP_LCD_FillRect(xButton, yButton + 7, CONTROL_BUTTON_SIZE - 14, CONTROL_BUTTON_SIZE - 14);

	// Next button label
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

// Refresh only the PLAY/PAUSE button
void update_play_pause_button() {

	BSP_LCD_SelectLayer(LAYER_BG);
	uint16_t xButton = buttonsLeftUpper[1][0];
	uint16_t yButton = buttonsLeftUpper[1][1];
	// Clear field under previous displayed button
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

// Auxillary functions
uint16_t getXPix (double factor)  {
	return factor * LCD_X_SIZE;
}

uint16_t getYPix (double factor)  {
	return factor * LCD_Y_SIZE;
}
