/*
 *	Main MP3 Player functionality
 */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mp3_player.h"
#include "gui.h"
#include "mp3dec.h"
#include "fatfs.h"
#include "stm32746g_discovery_audio.h"
#include "term_io.h"
#include "dbgu.h"
#include "ansi.h"
#define READ_BUFFER_SIZE 2 * MAINBUF_SIZE

#define DECODED_MP3_FRAME_SIZE MAX_NGRAN * MAX_NCHAN * MAX_NSAMP

#define AUDIO_OUT_BUFFER_SIZE 2 * DECODED_MP3_FRAME_SIZE

typedef enum BUFFER_STATE_ENUM {
	NONE = 0, HALF, FULL,
} BUFFER_STATE;

/* zmienne globalne */

static HMP3Decoder hMP3Decoder;
Mp3_Player_State state;
uint8_t input_buffer[READ_BUFFER_SIZE];
static uint8_t *current_ptr;
int currentFilePosition = -1;
short output_buffer[AUDIO_OUT_BUFFER_SIZE];
FIL input_file;
static BUFFER_STATE out_buf_offs = NONE;
char** paths;
int mp3FilesCounter = 0;
int volume = 50;
int currentFileBytes = 0;
int currentFileBytesRead = 0;
int bitrate = 0;
static int buffer_leftover = 0;
static int in_buf_offs;
static int decode_result;
int has_been_paused = 0;
char gui_info_text[100];

/* ------------------------------------------------------------------- */

void BSP_init();
void player_init();
void mp3_player_main(const char *);
void player_play();
int frame_process();
int read_mp3_file();
void clear_state();
void update_volume();

void mp3_player_main(const char* path) {
	BSP_init();
	state = NEXT;

	touchscreen_loop_init();

	sprintf(gui_info_text, "Initializing...");
	refresh_screen(gui_info_text);

	DIR directory;
	FILINFO info;

	if (f_opendir(&directory, path) != FR_OK) {
		return;
	}

	while (1) {
		if (f_readdir(&directory, &info) != FR_OK) {
			return;
		}
		if (info.fname[0] == 0)
			break;
		if (strstr(info.fname, ".mp3"))
			mp3FilesCounter++;
	}

	f_closedir(&directory);

	int i = 0;
	paths = malloc(sizeof(char*) * mp3FilesCounter);

	if (paths == NULL) {
		return;
	}

	if (f_opendir(&directory, path) != FR_OK) {
		return;
	}

	while (1) {
		if (f_readdir(&directory, &info) != FR_OK) {
			return;
		}
		if (info.fname[0] == 0)
			break;
		if (strstr(info.fname, ".mp3")) {
			paths[i] = malloc((strlen(info.fname) + 1) * sizeof(char));
			strcpy(paths[i], info.fname);
			i++;
		}
	}

	f_closedir(&directory);

	update_volume();
	while (1) {
		switch (state) {
		case PLAY:
			if (f_findfirst(&directory, &info, path, paths[currentFilePosition])
					!= FR_OK) {
				return;
			}
			currentFileBytes = info.fsize;
			f_closedir(&directory);
			sprintf(gui_info_text, "%s", paths[currentFilePosition]);
			refresh_screen(gui_info_text);
			player_play();
			f_close(&input_file);
			currentFileBytesRead = 0;
			break;
		case NEXT:
			clear_state();
			if (currentFilePosition == mp3FilesCounter - 1)
				currentFilePosition = 0;
			else
				currentFilePosition++;
			if (f_open(&input_file, paths[currentFilePosition], FA_READ)
					!= FR_OK) {
				return;
			}
			state = PLAY;
			break;
		case PREV:
			clear_state();
			if (currentFilePosition == 0)
				currentFilePosition = mp3FilesCounter - 1;
			else
				currentFilePosition--;
			if (f_open(&input_file, paths[currentFilePosition], FA_READ)
					!= FR_OK) {
				return;
			}
			state = PLAY;
			break;
		case PAUSE:
			// shouldn't reach
			break;
		case STOP:
			update_progress_bar(0);
			clear_state();
			currentFilePosition = 0;
			sprintf(gui_info_text, "STOP");
			refresh_screen(gui_info_text);
			while (state == STOP) {
				Mp3_Player_State newState = check_touchscreen();
				if (newState != EMPTY)
					state = newState;
			}
			if (state == PLAY)
				if (f_open(&input_file, paths[currentFilePosition], FA_READ)
						!= FR_OK) {
					return;
				}
			break;
		case FINISH:
			return;
		default:
			return;
		}

	}

}

void BSP_init() {
	if (BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_BOTH, 100, AUDIO_FREQUENCY_44K) == 0) {
		BSP_AUDIO_OUT_SetAudioFrameSlot(CODEC_AUDIOFRAME_SLOT_02);
	}
}

void player_play() {
	hMP3Decoder = MP3InitDecoder();
	if (frame_process() == 0) {
		state = PLAY;
		BSP_AUDIO_OUT_Play((uint16_t*) &output_buffer[0],
				AUDIO_OUT_BUFFER_SIZE * 2);
		int time = -1;
		show_full_time(currentFileBytes * 8.0 / bitrate);
		while (1) {
			if (currentFileBytesRead * 8.0 / bitrate > time) {
				update_actual_time(time++);
			}
			update_progress_bar(
					((double) currentFileBytesRead) / currentFileBytes);
			Mp3_Player_State newState = check_touchscreen();
			if (newState == VOLUMEUP) {
				if (volume < 100)
					volume += 10;
				update_volume();
			} else if (newState == VOLUMEDOWN) {
				if (volume > 0)
					volume -= 10;
				update_volume();
			} else if (newState != EMPTY)
				state = newState;
			if (!has_been_paused && state == PAUSE) {
				sprintf(gui_info_text, "%s", paths[currentFilePosition]);
				refresh_screen(gui_info_text);
				if (BSP_AUDIO_OUT_Pause() != AUDIO_OK) {
					return;
				}
				has_been_paused = 1;
			} else if (has_been_paused && state == PLAY) {
				sprintf(gui_info_text, "%s", paths[currentFilePosition]);
				refresh_screen(gui_info_text);
				if (BSP_AUDIO_OUT_Resume() != AUDIO_OK) {
					return;
				}
				has_been_paused = 0;
			} else if (has_been_paused && state == PAUSE) {
				continue;
			} else if (state != PLAY) {
				break;
			}
		}
		BSP_AUDIO_OUT_Stop(CODEC_PDWN_SW);
		out_buf_offs = NONE;
	} else {
		state = NEXT;
	}

	buffer_leftover = 0;
	current_ptr = NULL;
	MP3FreeDecoder(hMP3Decoder);

}

int frame_process() {
	MP3FrameInfo mp3FrameInfo;

	if (current_ptr == NULL && read_mp3_file() != 0)
		return EOF;

	in_buf_offs = MP3FindSyncWord(current_ptr, buffer_leftover);

	while (in_buf_offs < 0) {
		if (read_mp3_file() != 0)
			return EOF;
		if (buffer_leftover > 0) {
			buffer_leftover--;
			current_ptr++;
		}
		in_buf_offs = MP3FindSyncWord(current_ptr, buffer_leftover);
	}

	current_ptr += in_buf_offs;
	buffer_leftover -= in_buf_offs;
	if (!(MP3GetNextFrameInfo(hMP3Decoder, &mp3FrameInfo, current_ptr) == 0
			&& mp3FrameInfo.nChans == 2 && mp3FrameInfo.version == 0)) {
		if (buffer_leftover > 0) {
			buffer_leftover--;
			current_ptr++;
		}
		return 0;
	}

	bitrate = mp3FrameInfo.bitrate;

	if (buffer_leftover < MAINBUF_SIZE && read_mp3_file() != 0)
		return EOF;

	if (out_buf_offs == HALF) {
		decode_result = MP3Decode(hMP3Decoder, &current_ptr, &buffer_leftover,
				output_buffer, 0);
		out_buf_offs = NONE;
	}

	if (out_buf_offs == FULL) {
		decode_result = MP3Decode(hMP3Decoder, &current_ptr, &buffer_leftover,
				&output_buffer[DECODED_MP3_FRAME_SIZE], 0);
		out_buf_offs = NONE;
	}

	if (decode_result != ERR_MP3_NONE) {
		if (decode_result == ERR_MP3_INDATA_UNDERFLOW) {
			buffer_leftover = 0;
			if (read_mp3_file() != 0)
				return EOF;
		}
	}

	return 0;
}

void BSP_AUDIO_OUT_TransferComplete_CallBack() {
	out_buf_offs = FULL;
	if (frame_process() != 0) {
		BSP_AUDIO_OUT_Stop(CODEC_PDWN_SW);
		state = NEXT;
	}
}

void BSP_AUDIO_OUT_HalfTransfer_CallBack() {
	out_buf_offs = HALF;
	if (frame_process() != 0) {
		BSP_AUDIO_OUT_Stop(CODEC_PDWN_SW);
		state = NEXT;
	}
}

int read_mp3_file() {
	unsigned int actually_read, how_much_to_read;

	if (buffer_leftover > 0) {
		memcpy(input_buffer, current_ptr, buffer_leftover);
	}

	how_much_to_read = READ_BUFFER_SIZE - buffer_leftover;

	f_read(&input_file, (BYTE *) input_buffer + buffer_leftover,
			how_much_to_read, &actually_read);

	currentFileBytesRead += actually_read;

	if (actually_read == how_much_to_read) {
		current_ptr = input_buffer;
		in_buf_offs = 0;
		buffer_leftover = READ_BUFFER_SIZE;
		return 0;
	} else
		return EOF;
}

void update_volume() {
	BSP_AUDIO_OUT_SetVolume((uint8_t) volume);
	update_volume_bar(volume);
}
void clear_state() {
	buffer_leftover = 0;
	current_ptr = NULL;
	out_buf_offs = NONE;
}
