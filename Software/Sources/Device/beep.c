#include "beep.h"
#include "pwm.h"
#include "FreeRTOS.h"
#include "timers.h"

#define QUEUE_SIZE  10

static struct {
    BeepNote_t  notes[QUEUE_SIZE];
    uint8_t     head, tail, count;
    uint8_t     busy;
} g_beep;

static TimerHandle_t g_timer;

static void hw_beep(uint16_t freq) {
    uint16_t reload;
    
    if (freq == 0) { 
        PWM_UpdateDuty(PWMA_CH3, 0); 
        return; 
    }
    
    reload = (100000UL / freq) - 1;
    PWMA_SetReload16(reload);
    PWM_UpdateDuty(PWMA_CH3, reload / 2);
}

static void beep_callback(TimerHandle_t xTimer) {
    BeepNote_t *n;
    
    (void)xTimer;
    
    if (g_beep.count == 0) { 
        hw_beep(0); 
        g_beep.busy = 0; 
        return; 
    }

    n = &g_beep.notes[g_beep.head];
    g_beep.head = (g_beep.head + 1) % QUEUE_SIZE;
    g_beep.count--;

    hw_beep(n->freq);
    xTimerChangePeriod(g_timer, pdMS_TO_TICKS(n->duration), 0);
}

void Beep_Init(void) {
    g_beep.head = 0;
    g_beep.tail = 0;
    g_beep.count = 0;
    g_beep.busy = 0;
    
    g_timer = xTimerCreate("Beep", 1, pdFALSE, NULL, beep_callback);
}

uint8_t Beep_Play(uint16_t freq, uint16_t duration_ms) {
    if (g_beep.count >= QUEUE_SIZE) {
        return 1;
    }

    g_beep.notes[g_beep.tail].freq = freq;
    g_beep.notes[g_beep.tail].duration = duration_ms;
    g_beep.tail = (g_beep.tail + 1) % QUEUE_SIZE;
    g_beep.count++;

    if (!g_beep.busy) { 
        g_beep.busy = 1; 
        beep_callback(g_timer); 
    }
    
    return 0;
}

void Beep_Stop(void) {
    xTimerStop(g_timer, 0);
    hw_beep(0);
    
    g_beep.head = 0;
    g_beep.tail = 0;
    g_beep.count = 0;
    g_beep.busy = 0;
}

uint8_t Beep_IsBusy(void) { 
    return g_beep.busy; 
}