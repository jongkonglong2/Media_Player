#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <stdint.h>

typedef enum {
    SFX_NONE = 0,
    SFX_NAV,        /* Menu / card cursor move */
    SFX_SELECT,     /* Menu open / card flip */
    SFX_EAT,        /* Snake food pickup */
    SFX_MATCH,      /* Memory pair matched */
    SFX_WIN,        /* Game victory fanfare */
    SFX_GAMEOVER    /* Game over tone */
} SfxType;

/* Initializes LPC1768 DAC (P0.26), ADC (P0.25 Potentiometer), and Timer1 for SFX */
void AudioEngine_Init(void);

/* Non-blocking sound effect trigger for Menu & Games */
void Audio_PlaySfx(SfxType sfx);

/* USB Audio streaming control (Keil USBAudio integration) */
void AudioUSB_Connect(void);
void AudioUSB_Disconnect(void);
int  AudioUSB_IsConnected(void);

/* Reads MCB1700 potentiometer (CCW = higher volume, CW = lower volume) */
uint8_t AudioUSB_UpdatePotVolume(void);

#endif