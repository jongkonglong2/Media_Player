#ifndef MEDIA_CENTER_H
#define MEDIA_CENTER_H
#include <stdint.h>
void MediaCenter_Init(void);
void MediaCenter_Task(uint32_t joystick_state);
#endif
