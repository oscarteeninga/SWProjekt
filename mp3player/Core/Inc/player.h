#ifndef PLAYER_H_
#define PLAYER_H_

void player_main(const char*);

// Represents player states (FSM)
typedef enum Mp3_Player_State_Tag {
    PLAY,
    PAUSE,
    STOP,
    NEXT,
    PREV,
    FINISH,
	VOLUMEUP,
	VOLUMEDOWN,
	EMPTY
} Mp3_Player_State;

#endif
