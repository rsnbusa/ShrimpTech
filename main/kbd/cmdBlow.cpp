#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

/**
 * @brief Generate a random integer within a specified range
 * 
 * @param min Minimum value (inclusive)
 * @param max Maximum value (inclusive)
 * @return Random integer between min and max
 */
uint16_t randomInt(int min, int max) 
{
    return min + rand() % (max - min + 1);
}

/**
 * @brief Generate a random float within a specified range
 * 
 * @param min Minimum value (inclusive)
 * @param max Maximum value (inclusive)
 * @return Random float between min and max
 */
float randomFloat(float min, float max) 
{
    float scale = rand() / (float)RAND_MAX;  // [0, 1.0]
    return min + scale * (max - min);
}

/**
 * @brief Seed the blower object with random test data
 * 
 * Populates the blower with realistic random values for:
 * - PV Panel data (voltage, current)
 * - Battery data (SOC, SOH, power, temperature)
 * - Energy data (generation, consumption, power metrics)
 * - Sensor data (DO, pH, temperatures, humidity)
 */
void seed_blower()
{
    theBlower.setPVPanel(randomInt(0, 1), randomFloat(340, 390), randomFloat(340, 390), 
                         randomFloat(14, 15), randomFloat(14, 15));
    theBlower.setBattery(batteryData.batSOC, batteryData.batSOH,  randomInt(0, 5000), 
    // theBlower.setBattery(randomInt(20, 80), randomInt(20, 100), randomInt(0, 5000), 
                         randomFloat(10, 40));
    theBlower.setEnergy(randomInt(780, 820), randomInt(720, 820), randomInt(720, 850), 
                        randomInt(720, 820), randomFloat(40, 42), randomFloat(40, 42), 
                        randomFloat(40, 42), randomFloat(40, 42), randomFloat(40, 42), 
                        randomFloat(40, 42));
    theBlower.setSensors(sensorData.WTemp, randomFloat(4, 8), sensorData.DO, 
                         randomFloat(22, 32), randomFloat(40, 99));
    // theBlower.setSensors(randomFloat(4, 8), randomFloat(4, 8), randomFloat(18, 30), 
    //                      randomFloat(22, 32), randomFloat(40, 99));
}

/**
 * @brief Command handler for blower data manipulation
 * 
 * @param argc Argument count
 * @param argv Argument values
 * @return 0 on success
 */
int cmdBlow(int argc, char **argv)
{
    // Parse command line arguments
    int nerrors = arg_parse(argc, argv, (void **)&blowArgs);
    if (nerrors != 0) {
        arg_print_errors(stderr, blowArgs.end, argv[0]);
        return 0;
    }

    // Seed blower with random data
    if (blowArgs.seed->count) {
        srand(time(NULL));
        seed_blower();
        printf("Blower seeded with random test data\n");
        return 0;
    }

    // Initialize blower with zeros
    if (blowArgs.init->count) {
        theBlower.setPVPanel(0, 0, 0, 0, 0);
        theBlower.setBattery(0, 0, 0, 0);
        theBlower.setEnergy(0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        theBlower.setSensors(0, 0, 0, 0, 0);
        printf("Blower initialized with zeros\n");
        return 0;
    }

    // No valid arguments provided
    printf("Error: No valid arguments. Use -s to seed or -i to initialize\n");
    return 0;
}

