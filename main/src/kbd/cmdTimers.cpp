#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"
#include "feederScheduler.h"

int cmdTimers(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    printf("\n%s==================== ALL TIMERS ====================%s\n", CYAN, RESETC);
    
    printf("\n%s--- FEEDER TIMERS ---%s\n", YELLOW, RESETC);
    show_feeder_timers();
    
    printf("\n%s--- BLOWER TIMERS ---%s\n", YELLOW, RESETC);
    show_blower_timers();
    
    printf("%s=====================================================%s\n\n", CYAN, RESETC);
    return 0;
}
