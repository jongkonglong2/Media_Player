/*
 * COE718 Media Center - LPC1768 / MCB1700
 * CMSIS-RTOS2 application entry point.
 *
 * Required Keil components:
 *   - CMSIS-RTOS2 (RTX5)
 *   - MCB1700 Board Support: GLCD and Joystick
 *   - LPC1768 device startup/system files
 *
 * The supplied Final_project.zip uses CMSIS-RTOS2 in Lab5_Part2 and
 * CMSIS-RTOS v1 in Lab5_Part1. This application follows the v2 API.
 */
#include "RTE_Components.h"
#include CMSIS_device_header
#include "cmsis_os2.h"
#include "GLCD.h"
#include "Board_Joystick.h"
#include "media_center.h"
#include "audio_engine.h"

__NO_RETURN static void MediaCenterThread(void *argument) {
    (void)argument;
    GLCD_Init();
    Joystick_Initialize();
    AudioEngine_Init();
    MediaCenter_Init();

    for (;;) {
        MediaCenter_Task(Joystick_GetState());
        osDelay(35U);
    }
}

static const osThreadAttr_t appThreadAttr = {
    .name = "MediaCenter",
    .priority = osPriorityNormal,
    .stack_size = 2048U
};

int main(void) {
    SystemCoreClockUpdate();
    osKernelInitialize();
    osThreadNew(MediaCenterThread, NULL, &appThreadAttr);
    osKernelStart();

    for (;;) {
    }
}