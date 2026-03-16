/*
 * main.c — Entry point for ZX Point
 *
 * Detects hardware, then hands off to the state machine.
 */

#include "../include/hw.h"
#include "../include/state.h"

int main(void)
{
    hw_detect();
    state_run();
    return 0;
}
