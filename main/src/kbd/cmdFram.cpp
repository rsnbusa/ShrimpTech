/**
 * @file cmdFram.cpp
 * @brief Console command for FRAM memory management and data seeding
 * 
 * This file implements commands for managing FRAM (Ferroelectric RAM) storage,
 * including formatting, reading statistics, and seeding test data for the
 * blower/solar system simulation.
 * 
 * @note Part of the ShrimpIoT console command system
 * @note Uses theBlower object for FRAM operations
 */

#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"
#include <random>

/**
 * @brief Generate random integer value within range
 * 
 * @param min Minimum value (inclusive)
 * @param max Maximum value (inclusive)
 * @return Random integer between min and max
 */
int randomval(int min, int max)
{
    return (esp_random() % (max - min + 1)) + min;
}

/**
 * @brief Generate random float value within range
 * 
 * @param min Minimum value (inclusive)
 * @param max Maximum value (inclusive)
 * @return Random float between min and max (1 decimal precision)
 */
float frandomval(float min, float max)
{
    int mmin = min * 10.0;
    int mmax = max * 10.0;
    int val = esp_random() % (mmax - mmin + 1) + mmin;
    return (float)val / 10.0;
}

/**
 * @brief Console command for FRAM operations
 * 
 * Provides subcommands for:
 * - format: Format FRAM and write creation timestamp
 * - stats read: Display node statistics (messages, bytes, node counts)
 * - seed: Seed FRAM with random test data for PV panels, battery, energy, sensors
 * 
 * @param argc Argument count from argtable3 parser
 * @param argv Argument vector from argtable3 parser
 * @return 0 on success
 * 
 * @note Usage: fram -f | -s read | -d
 *       -f, --format: Format FRAM memory
 *       -s, --stats: Read statistics
 *       -d, --seed: Seed with random test data
 */
int cmdFram(int argc, char **argv)
{
    time_t now;

    int nerrors = arg_parse(argc, argv, (void **)&framArg);
    if (nerrors != 0) {
        arg_print_errors(stderr, framArg.end, argv[0]);
        return 0;
    }

    if (framArg.format->count) {
        theBlower.deinit();
        theBlower.format();
        time(&now);
        theBlower.writeCreationDate(now);
        printf("Fram Formatted\n");
        return 0;
    }

    if (framArg.stats->count) {
        const char *que = framArg.stats->sval[0];

        if (strcmp(que, "read") == 0)
        {
            time_t ahora = theBlower.getStatsLastCountTS();
            printf("====== Node stats ======\n");
            printf("Msgs In: %d Bytes In: %d\n", theBlower.getStatsMsgIn(), theBlower.getStatsBytesIn());
            printf("Msgs Out: %d Bytes Out: %d\n", theBlower.getStatsMsgOut(), theBlower.getStatsBytesOut());
            printf("Nodes %d Blowers %d lastDate %s", theBlower.getStatsLastNodeCount(), theBlower.getStatsLastBlowerCount(), ctime(&ahora));
        }
    }

    if (framArg.seed->count) {
        theBlower.setPVPanel(randomval(0, 1), randomval(345, 360), randomval(345, 360), frandomval(13.5, 14.7), frandomval(13.5, 14.7));
        theBlower.setBattery(randomval(20, 85), randomval(0, 10), 1, frandomval(22.3, 32.3));
        theBlower.setEnergy(randomval(50, 58), randomval(50, 58), randomval(350, 658), randomval(350, 658), frandomval(39000.0, 41200.0),
            frandomval(39000.0, 41200.0), frandomval(39000.0, 41200.0), frandomval(39000.0, 41200.0), frandomval(39.0, 42.3), frandomval(39.0, 42.3));
        theBlower.setSensors(frandomval(3.5, 7.0), frandomval(4.3, 6.6), frandomval(18.4, 29.2), frandomval(24.4, 32.2), frandomval(20.40, 88.40));
        print_blower("Seed", theBlower.getPtrSolarsystem(), true);
        return 0;
    }

    return 0;
}