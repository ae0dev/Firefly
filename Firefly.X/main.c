/*
 * Firefly simulator
 * No fireflies were harmed in the making of this device
 */

#pragma warning disable 520         // Suppress "function [...] not called" warning

#include "mcc_generated_files/mcc.h"
#include <stdlib.h>

// Function prototypes
void ISR_Timer0();      // Timer0 ISR
void pulse();           // Pulse LED

// Constants
static const uint16_t PULSE_SUSTAIN = 18;   // Number of timer periods to sustain max PWM
static const uint16_t PWM_RAMP_UP = 11;     // Duty cycle increase per timer period
static const uint16_t PWM_RAMP_DOWN = 5;    // Duty cycle decrease per timer period
static const uint16_t PWM_DC_MAX = 0x7F;    // Maximum duty cycle value
static const uint16_t PWM_DC_MIN = 0x00;    // Minimum duty cycle value

// Variables
static volatile bool t0Rollover = false;    // Timer0 rollover flag

/*
 * Main
 * Makes the  magic happen
 */
void main()
{
    const int RAND_THRESH = RAND_MAX / 2;       // Random value threshold
    
    // Initialize system
    SYSTEM_Initialize();
    
    // Set Timer0 ISR handler
    TMR0_SetInterruptHandler(ISR_Timer0);
    
    // Seed RNG...  sort of...
    srand(0);
    
    // Loop forever!
    while (1)
    {
        // Disable interrupts
        INTERRUPT_PeripheralInterruptDisable();
        INTERRUPT_GlobalInterruptDisable();
        
        // Sleep...
        CLRWDT();
        SLEEP();
        
        // Re-enable interrupts
        INTERRUPT_GlobalInterruptEnable();
        INTERRUPT_PeripheralInterruptEnable();
        
        // 1-in-X chance of doing a thing this cycle
        if(rand() < RAND_THRESH)
        {        
            // Reset Timer0 and pulse the LED
            TMR0_Reload();
            pulse();
        }
    }
}

/*
 * Timer0 ISR
 * Sets rollover flag
 */
void ISR_Timer0()
{
    t0Rollover = true;
}

/*
 * Run through a single flash pattern
 * Timer0 must be running with its interrupt enabled before calling this
 */
void pulse()
{
    uint16_t dutyCycle = PWM_DC_MIN;        // Current duty cycle
    uint16_t adj;                           // Ramp min/max trim
    
    uint16_t sustainCount = PULSE_SUSTAIN  + (rand() & 0x03) - (rand() & 0x03); // Sustain countdown
    uint16_t rampUp = PWM_RAMP_UP + (rand() & 0x03) - (rand() & 0x03);          // Ramp up value
    uint16_t rampDown = PWM_RAMP_DOWN + (rand() & 0x03) - (rand() & 0x03);      // Ramp down value
    
    // Set starting duty cycle
    EPWM_LoadDutyValue(dutyCycle);
    
    // Ramp DC up
    while(dutyCycle < PWM_DC_MAX)
    {
        if(t0Rollover)
        {
            t0Rollover = false;
            CLRWDT();
            
            adj = (PWM_DC_MAX - dutyCycle);
            dutyCycle += (adj < rampUp) ? adj : rampUp;
            EPWM_LoadDutyValue(dutyCycle);
        }
    }
    
    // Sustain DC at max
    while(sustainCount > 0)
    {
        if(t0Rollover)
        {
            t0Rollover = false;
            CLRWDT();
            
            --sustainCount;
        }
    }
    
    // Ramp DC down
    while(dutyCycle > PWM_DC_MIN)
    {
        if(t0Rollover)
        {
            t0Rollover = false;
            CLRWDT();
            
            adj = (dutyCycle - PWM_DC_MIN);
            dutyCycle -= (adj < rampDown) ? adj : rampDown;
            EPWM_LoadDutyValue(dutyCycle);
        }
    }
    
    // Set ending DC
    EPWM_LoadDutyValue(PWM_DC_MIN);
}
