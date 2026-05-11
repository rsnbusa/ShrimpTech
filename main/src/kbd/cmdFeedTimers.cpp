#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"
#include "feederScheduler.h"

int cmdFeedTimers(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    show_feeder_timers();
    return 0;
}
