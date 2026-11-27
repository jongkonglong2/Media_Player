#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <stdint.h>

void AudioPlayer_Enter(void);
int  AudioPlayer_Task(uint32_t pressed);
void AudioPlayer_Exit(void);

#endif