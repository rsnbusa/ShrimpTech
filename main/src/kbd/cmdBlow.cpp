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
    theBlower.setPVPanel(pvPanelData.chargeCurr, pvPanelData.pv1Volts, pvPanelData.pv2Volts,pvPanelData.pv1Amp, pvPanelData.pv2Amp);
    theBlower.setBattery(batteryData.batSOC, batteryData.batSOH, batteryData.batteryCycleCount, batteryData.batBmsTemp);
    theBlower.setEnergy(energyData.batChgAHToday, energyData.batDischgAHToday, 
                       energyData.batChgAHTotal, energyData.batDischgAHTotal, 
                       energyData.generateEnergyToday, energyData.usedEnergyToday, 
                       energyData.gLoadConsumLineTotal, energyData.batChgkWhToday, 
                       energyData.batDischgkWhToday, energyData.genLoadConsumToday);
    theBlower.setSensors(sensorData.WTemp, randomFloat(4, 8), sensorData.DO, 
                         randomFloat(22, 32), randomFloat(40, 99));
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
        return 0;
    }

    // Set MINUTES for simulation
    if (blowArgs.minute->count) {
        int minutes = blowArgs.minute->ival[0];
        MESP_LOGI(TAG,"Setting MINUTES to %d from %d", minutes, theConf.test_timer_div);
        if(minutes<0) minutes=1;
        theConf.test_timer_div = minutes;
        write_to_flash();
        return 0;
    }
    // Set modbus for simulation
    if (blowArgs.modbus->count) {
        int modbuss = blowArgs.modbus->ival[0];
        MESP_LOGI(TAG,"Setting Modbus to %d from %d", modbuss, theConf.temp_sensor);
        if(modbuss<0) modbuss=1;
        theConf.temp_sensor = modbuss;
        write_to_flash();
        return 0;
    }

    // Initialize blower with zeros
    if (blowArgs.init->count) {
        theBlower.setPVPanel(0, 0, 0, 0, 0);
        theBlower.setBattery(0, 0, 0, 0);
        theBlower.setEnergy(0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        theBlower.setSensors(0, 0, 0, 0, 0);
        MESP_LOGI(TAG,"Blower initialized with zeros");
        return 0;
    }

    // No valid arguments provided
    MESP_LOGI(TAG,"Error: No valid arguments. Use -s to seed or -i to initialize");
    return 0;
}

