#include "audio_engine.h"
#include "LPC17xx.h"

/* External symbols from Keil USBAudio (usbcore.c / usbhw.c / usbdmain.c) */
extern void     USB_Init(void);
extern void     USB_Connect(uint32_t con);
extern uint16_t PotVal;
extern uint8_t  DataRun;
extern void     get_potval(void);

static volatile int usb_connected = 0;

/* Non-blocking SFX synthesizer state (Timer 1) */
static volatile uint32_t sfx_samples_left = 0;
static volatile uint32_t sfx_half_period  = 10;
static volatile uint32_t sfx_toggle_cnt   = 0;
static volatile uint8_t  sfx_level        = 0;
static volatile int32_t  sfx_period_step  = 0;
static volatile uint32_t sfx_step_timer   = 0;

void AudioEngine_Init(void) {
    /* 1. Configure P0.26 as AOUT (DAC) and P0.25 as AD0.2 (Potentiometer) */
    LPC_PINCON->PINSEL1 &= ~((3U << 20) | (3U << 18));
    LPC_PINCON->PINSEL1 |=  ((2U << 20) | (1U << 18));
    LPC_DAC->DACR = 0x8000U;

    /* 2. Power and configure ADC (matches usbdmain.c's get_potval) */
    LPC_SC->PCONP |= (1U << 12);
    LPC_ADC->ADCR  = 0x00200E04U;

    /* 3. Configure Timer 1 at 16 kHz for menu/game SFX */
    LPC_SC->PCONP |= (1U << 2);
    LPC_TIM1->TCR  = 0x02U;
    LPC_TIM1->PR   = 0U;
    LPC_TIM1->MR0  = (SystemCoreClock / 4U) / 16000U;
    LPC_TIM1->MCR  = 0x03U;
    NVIC_SetPriority(TIMER1_IRQn, 6U);
    NVIC_EnableIRQ(TIMER1_IRQn);
    LPC_TIM1->TCR  = 0x01U;
}

void TIMER1_IRQHandler(void) {
    LPC_TIM1->IR = 1U;

    if (usb_connected) {
        return;
    }

    if (sfx_samples_left > 0U) {
        sfx_samples_left--;
        sfx_toggle_cnt++;
        if (sfx_toggle_cnt >= sfx_half_period) {
            sfx_toggle_cnt = 0U;
            sfx_level = !sfx_level;
        }

        sfx_step_timer++;
        if (sfx_step_timer >= 400U) {
            sfx_step_timer = 0U;
            if ((int32_t)sfx_half_period + sfx_period_step > 4) {
                sfx_half_period = (uint32_t)((int32_t)sfx_half_period + sfx_period_step);
            }
        }

        LPC_DAC->DACR = (sfx_level ? 0xA000U : 0x6000U) & 0xFFC0U;
    } else {
        LPC_DAC->DACR = 0x8000U;
    }
}

void Audio_PlaySfx(SfxType sfx) {
    if (usb_connected) {
        return;
    }
    sfx_toggle_cnt = 0U;
    sfx_step_timer = 0U;
    sfx_level = 1U;

    switch (sfx) {
        case SFX_NAV:      sfx_half_period = 11U; sfx_period_step =  0; sfx_samples_left =  560U; break;
        case SFX_SELECT:   sfx_half_period = 14U; sfx_period_step = -2; sfx_samples_left = 1120U; break;
        case SFX_EAT:      sfx_half_period = 12U; sfx_period_step = -2; sfx_samples_left = 1600U; break;
        case SFX_MATCH:    sfx_half_period = 15U; sfx_period_step = -2; sfx_samples_left = 2880U; break;
        case SFX_WIN:      sfx_half_period = 16U; sfx_period_step = -1; sfx_samples_left = 5600U; break;
        case SFX_GAMEOVER: sfx_half_period = 14U; sfx_period_step =  3; sfx_samples_left = 5120U; break;
        default:           sfx_samples_left = 0U; break;
    }
}
extern void USB_ResetCore(void);

static void usb_force_pll1_48mhz(void) {
    /* Ensure 12 MHz main oscillator is enabled and ready */
    if ((LPC_SC->SCS & (1U << 6)) == 0U) {
        LPC_SC->SCS |= (1U << 5);
        while ((LPC_SC->SCS & (1U << 6)) == 0U) {}
    }

    /* Disconnect and disable PLL1 before reconfiguring */
    LPC_SC->PLL1CON  = 0x00U;
    LPC_SC->PLL1FEED = 0xAAU;
    LPC_SC->PLL1FEED = 0x55U;

    /* 12 MHz * 4 = 48 MHz: MSEL1 = 3 (M=4), PSEL1 = 1 (P=2) */
    LPC_SC->PLL1CFG  = (3U << 0) | (1U << 5);
    LPC_SC->PLL1FEED = 0xAAU;
    LPC_SC->PLL1FEED = 0x55U;

    /* Enable PLL1 */
    LPC_SC->PLL1CON  = 0x01U;
    LPC_SC->PLL1FEED = 0xAAU;
    LPC_SC->PLL1FEED = 0x55U;

    /* Wait for PLL1 lock (bit 10 = PLOCK1) */
    while ((LPC_SC->PLL1STAT & (1U << 10)) == 0U) {}

    /* Enable and Connect PLL1 */
    LPC_SC->PLL1CON  = 0x03U;
    LPC_SC->PLL1FEED = 0xAAU;
    LPC_SC->PLL1FEED = 0x55U;
}


void AudioUSB_Connect(void) {
    volatile uint32_t d;

    if (!usb_connected) {
        sfx_samples_left = 0U;
        usb_connected = 1;

        /* Pause Timer 1 while USB Audio owns the DAC */
        LPC_TIM1->TCR = 0x00U;

        /* Force PCLK_TIMER0 = CCLK / 4 (25 MHz at 100 MHz CCLK) */
        LPC_SC->PCLKSEL0 &= ~(3U << 2);
        LPC_SC->PCONP    |=  (1U << 1);
        LPC_TIM0->TCR     =  0x02U;
        LPC_TIM0->PR      =  0U;
        {
            uint32_t mr0 = (SystemCoreClock / 4U) / 32000U;
            if (mr0 < 200U) {
                mr0 = 781U; /* Fallback for 100 MHz CCLK -> 32 kHz */
            }
            LPC_TIM0->MR0 = mr0;
        }
        LPC_TIM0->MCR = 0x03U;
        LPC_TIM0->IR  = 0xFFU;

        /* Keep USB and Timer0 below RTOS SysTick priority so UI never freezes */
        NVIC_SetPriority(TIMER0_IRQn, 5U);
        NVIC_SetPriority(USB_IRQn, 4U);

				get_potval();
        usb_force_pll1_48mhz();
        USB_Init();
        USB_ResetCore();

        /* Brief disconnect pulse so Windows re-enumerates cleanly on re-entry */
        USB_Connect(0U);
        for (d = 0U; d < 100000U; d++) {
            __NOP();
        }
        USB_Connect(1U);

        NVIC_EnableIRQ(TIMER0_IRQn);
        LPC_TIM0->TCR = 0x01U;
    }
}

void AudioUSB_Disconnect(void) {
    if (usb_connected) {
        LPC_TIM0->TCR = 0x00U;
        NVIC_DisableIRQ(TIMER0_IRQn);

        USB_Connect(0U);
        NVIC_DisableIRQ(USB_IRQn);

        DataRun = 0U;
        LPC_DAC->DACR = 0x8000U;
        usb_connected = 0;

        /* Resume Timer 1 for menu and game sound effects */
        LPC_TIM1->TCR = 0x01U;
    }
}

int AudioUSB_IsConnected(void) {
    return usb_connected;
}

uint8_t AudioUSB_UpdatePotVolume(void) {
    uint32_t pct = ((uint32_t)PotVal * 100U) / 255U;
    if (pct > 100U) pct = 100U;
    return (uint8_t)pct;
}