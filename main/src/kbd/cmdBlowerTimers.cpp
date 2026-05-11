#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

int cmdBlowerTimers(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    show_blower_timers();
    return 0;
}
