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
 * - test: Write deterministic values, reload from FRAM, and verify read-back
 * 
 * @param argc Argument count from argtable3 parser
 * @param argv Argument vector from argtable3 parser
 * @return 0 on success
 * 
 * @note Usage: fram -f | -s read | -d | --test 1
 *       -f, --format: Format FRAM memory
 *       -s, --stats: Read statistics
 *       -d, --seed: Seed with random test data
 *       --test: Run FRAM write/read-back verification
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
            printf("Msgs In: %ld Bytes In: %ld\n", theBlower.getStatsMsgIn(), theBlower.getStatsBytesIn());
            printf("Msgs Out: %ld Bytes Out: %ld\n", theBlower.getStatsMsgOut(), theBlower.getStatsBytesOut());
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

    if (framArg.test->count) {
        // int initResult = theBlower.initBlower();
        // if (initResult != ESP_OK) {
        //     // Recover once for fresh/invalid FRAM contents, then retry init.
        //     theBlower.deinit();
        //     theBlower.format();
        //     time(&now);
        //     theBlower.writeCreationDate(now);
        //     initResult = theBlower.initBlower();
        //     if (initResult != ESP_OK) {
        //         printf("FRAM test failed: initBlower error %d\n", initResult);
        //         return 1;
        //     }
        // }

        const uint8_t expChargeCurr = 1;
        const float expPv1Volts = 333.75f;
        const float expPv2Volts = 329.50f;
        const float expPv1Amp = 12.25f;
        const float expPv2Amp = 11.75f;

        const uint8_t expBatSoc = 77;
        const uint8_t expBatSoh = 93;
        const uint16_t expBatCycles = 1234;
        const float expBatTemp = 31.5f;

        const uint16_t expChgAHToday = 45;
        const uint16_t expDischgAHToday = 38;
        const uint16_t expChgAHTotal = 678;
        const uint16_t expDischgAHTotal = 654;
        const float expGenToday = 25.75f;
        const float expUsedToday = 24.50f;
        const float expLoadTotal = 1200.25f;
        const float expChgkWhToday = 3.15f;
        const float expDischgkWhToday = 2.95f;
        const float expGenLoadToday = 22.30f;

        const float expDO = 5.55f;
        const float expPH = 7.10f;
        const float expWTemp = 28.40f;
        const float expATemp = 30.20f;
        const float expAHum = 66.0f;

        // Write deterministic values into FRAM via standard setters.
        theBlower.setPVPanel(expChargeCurr, expPv1Volts, expPv2Volts, expPv1Amp, expPv2Amp);
        theBlower.setBattery(expBatSoc, expBatSoh, expBatCycles, expBatTemp);
        theBlower.setEnergy(expChgAHToday, expDischgAHToday, expChgAHTotal, expDischgAHTotal,
                            expGenToday, expUsedToday, expLoadTotal, expChgkWhToday,
                            expDischgkWhToday, expGenLoadToday);
        theBlower.setSensors(expDO, expPH, expWTemp, expATemp, expAHum);

        // Drop RAM cache and reload directly from FRAM for read-back validation.
        theBlower.deinit();
        theBlower.loadBlower();

        uint8_t actChargeCurr = 0;
        float actPv1Volts = 0.0f, actPv2Volts = 0.0f, actPv1Amp = 0.0f, actPv2Amp = 0.0f;
        uint8_t actBatSoc = 0, actBatSoh = 0;
        uint16_t actBatCycles = 0;
        float actBatTemp = 0.0f;
        uint16_t actChgAHToday = 0, actDischgAHToday = 0, actChgAHTotal = 0, actDischgAHTotal = 0;
        float actGenToday = 0.0f, actUsedToday = 0.0f, actLoadTotal = 0.0f;
        float actChgkWhToday = 0.0f, actDischgkWhToday = 0.0f, actGenLoadToday = 0.0f;
        float actDO = 0.0f, actPH = 0.0f, actWTemp = 0.0f, actATemp = 0.0f, actAHum = 0.0f;

        theBlower.getPVPanel(&actChargeCurr, &actPv1Volts, &actPv2Volts, &actPv1Amp, &actPv2Amp);
        theBlower.getBattery(&actBatSoc, &actBatSoh, &actBatCycles, &actBatTemp);
        theBlower.getEnergy(&actChgAHToday, &actDischgAHToday, &actChgAHTotal, &actDischgAHTotal,
                            &actGenToday, &actUsedToday, &actLoadTotal, &actChgkWhToday,
                            &actDischgkWhToday, &actGenLoadToday);
        theBlower.getSensors(&actDO, &actPH, &actWTemp, &actATemp, &actAHum);

        bool pass = true;
        pass &= (actChargeCurr == expChargeCurr);
        pass &= (actPv1Volts == expPv1Volts && actPv2Volts == expPv2Volts && actPv1Amp == expPv1Amp && actPv2Amp == expPv2Amp);
        pass &= (actBatSoc == expBatSoc && actBatSoh == expBatSoh && actBatCycles == expBatCycles && actBatTemp == expBatTemp);
        pass &= (actChgAHToday == expChgAHToday && actDischgAHToday == expDischgAHToday && actChgAHTotal == expChgAHTotal && actDischgAHTotal == expDischgAHTotal);
        pass &= (actGenToday == expGenToday && actUsedToday == expUsedToday && actLoadTotal == expLoadTotal && actChgkWhToday == expChgkWhToday);
        pass &= (actDischgkWhToday == expDischgkWhToday && actGenLoadToday == expGenLoadToday);
        pass &= (actDO == expDO && actPH == expPH && actWTemp == expWTemp && actATemp == expATemp && actAHum == expAHum);

        printf("FRAM test %s\n", pass ? "PASS" : "FAIL");
        if (!pass) {
            printf("PV expected[%u %.2f %.2f %.2f %.2f] actual[%u %.2f %.2f %.2f %.2f]\n",
                expChargeCurr, expPv1Volts, expPv2Volts, expPv1Amp, expPv2Amp,
                actChargeCurr, actPv1Volts, actPv2Volts, actPv1Amp, actPv2Amp);
            printf("BAT expected[%u %u %u %.2f] actual[%u %u %u %.2f]\n",
                expBatSoc, expBatSoh, expBatCycles, expBatTemp,
                actBatSoc, actBatSoh, actBatCycles, actBatTemp);
            printf("SENS expected[%.2f %.2f %.2f %.2f %.2f] actual[%.2f %.2f %.2f %.2f %.2f]\n",
                expDO, expPH, expWTemp, expATemp, expAHum,
                actDO, actPH, actWTemp, actATemp, actAHum);
        }

        return pass ? 0 : 1;
    }

    return 0;
}