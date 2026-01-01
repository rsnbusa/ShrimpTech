
#define GLOBAL     
/**
 * @file cmdBaseTimer.cpp
 * @brief Console command handler for configuring base timer and repeat timer intervals
 * 
 * This command allows configuration of two timer parameters:
 * - First timer (base timer): Minimum value is 10, controls the base timing interval
 * - Repeat timer: Controls the repeat timing interval
 * 
 * Changes are saved to flash and take effect on the next timer cycle.
 * 
 * Usage: basetimer -f <first_timer_value> -r <repeat_timer_value>
 */

#include "includes.h"
#include "globals.h"
#include "forwards.h"

/**
 * @brief Command handler for setting base timer and repeat timer values
 * 
 * @param argc Argument count
 * @param argv Argument values
 * @return 0 on success or error
 */
int cmdBasetimer(int argc, char **argv)
{
    // Parse command line arguments
    int nerrors = arg_parse(argc, argv, (void **)&basetimer);
    if (nerrors != 0) {
        arg_print_errors(stderr, basetimer.end, argv[0]);
        return 0;
    }

    // Configure first timer (base timer) if provided
    if (basetimer.first->count) {
        int lev = basetimer.first->ival[0];
        
        // Enforce minimum value of 10
        if (lev < 1) {
            lev = 10;
        }
        
        printf("First timer changed from %d to %d\n", theConf.baset, lev);
        theConf.baset = BASETIMER = lev;  // Will take effect in NEXT timer repeat
    }

    // Configure MINUTE value
    if (basetimer.minute->count) {
        int lev = basetimer.minute->ival[0];
        
        // Enforce minimum value of 10
        if (lev < 1) {
            lev = 1;
        }
        
        printf("Minute  changed from %d to %d\n", theConf.minute, lev);
        theConf.minute =lev;  // Will take effect in NEXT timer repeat
    }

    // Configure repeat timer if provided
    if (basetimer.repeat->count) {
        int lev = basetimer.repeat->ival[0];
        
        if (lev < 1) {
            printf("Error: Repeat timer value must be at least 1\n");
            return 0;
        }
        
        printf("Repeat timer changed from %d to %d\n", theConf.repeat, lev);
        theConf.repeat = lev;
    }

    // Save configuration to flash
    write_to_flash();
    printf("Timer configuration saved to flash\n");

    return 0;
}
