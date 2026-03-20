
/**
 * @file printSensors.cpp
 * @brief Debug printing functions for Modbus sensor data
 * 
 * Provides formatted console output for various sensor types including
 * environmental sensors, energy monitoring, battery status, and solar panels.
 * Output is controlled by MODBUS debug flag and error status.
 * 
 * @note Part of the ShrimpIoT Modbus device monitoring system
 */

#define GLOBAL
#include "includes.h"
#include "typedef.h"
#include "globals.h"

// ============================================================================
// Environmental Sensors
// ============================================================================

void cb_vfd_cmd(void *vfdd, int *errors,char *color,int numerrs,int devAddr)
{
    bool hasErrors=false;
    (void)vfdd;
    (void)errors;
    (void)color;
    (void)numerrs;
    (void)devAddr;
 
    printf("VFD Cmd callback cmd %d\n",vfdCmdData.cmd);

    if(vfdHandle!=NULL)
    {
        if(vfdCmdData.cmd>0 )
        {
            vTaskResume(vfdHandle);
            vTaskResume(vfdHandle2);
        }
        else
        {
            vTaskSuspend(vfdHandle);
            vTaskSuspend(vfdHandle2);
        }
    }

    // ! execution order here is important. First do the above and then suspend the task, WHICH includes this code. 
    // ! SO if suspended before sending resume/suspend order, nothing done

    if(vfdcmdHandle)
    {
        vTaskSuspend(vfdcmdHandle); // suspend the command task, it will be resumed by the blower task when the blower is started, and killed when the blower is stopped
        vTaskSuspend(vfdcmdHandle2); // suspend the command task, it will be resumed by the blower task when the blower is started, and killed when the blower is stopped
    }
    }

/**
 * @brief Print environmental VFD data for debugging
 * 
 * Logs environmental monitoring data including water temperature, dissolved
 * oxygen concentration and percentage.
 * 
 * @param sensors Pointer to sensor_t data structure
 * @param errors Pointer to error code array
 * @param color String representing the color for logging
 * @param numerrs Number of error codes
 * @note Checks errors[1] instead of errors[0] for multi-sensor setups
 * @note Only prints if MODBUS debug flag is enabled and no errors occurred
 *  Limits WTEMP,LIMITDO,LIMITPH,ATEMP,AHUM
 */
void cb_vfd_data(void *vfdd, int *errors,char *color,int numerrs,int devAddr)
{
    bool hasErrors=false;
 
    // check errors before printing
    for (int a=0;a<numerrs;a++)
    {
        if(errors[a]!=0)
        {
            if (((theConf.debug_flags >> dMODBUS) & 1U))
                MESP_LOGE(TAG,"%s[%3d] VFD Error CID %d=0x%x %s ",color,devAddr,a,errors[a],esp_err_to_name(errors[a]));
            hasErrors=true;
        }
    }

    if (hasErrors)
    {
        globalErrors|= (1U << VFD_ERROR_BIT); // set sensor error bit
        if (((theConf.debug_flags >> dMODBUS) & 1U))
            printf("\n");
        return;
    }   

    globalErrors &= ~(1U << VFD_ERROR_BIT); // clear the error bit
     
    vfd_t *data = (vfd_t*)vfdd;

    globalErrors &= ~(1U << VFD_LIMIT_ERROR_BIT); // clear the limit error bit
        
    if (!((theConf.debug_flags >> dMODBUS) & 1U))
        return;


    MESP_LOGI(TAG, "%s[%3d] Motor Current:%.02f amps Volts %d Power:%.02f RPM: %d%s", color,devAddr,
             data->mcurrent,
             data->mvolts,
             data->mpower,data->mrpm,RESETC);
    // save data to frame blower sensors
            //  theBlower.setSensors( data->DO,  0,data->WTemp,
            //             temperature, 0);
}

/**
 * @brief Print environmental sensor data for debugging
 * 
 * Logs environmental monitoring data including water temperature, dissolved
 * oxygen concentration and percentage.
 * 
 * @param sensors Pointer to sensor_t data structure
 * @param errors Pointer to error code array
 * @param color String representing the color for logging
 * @param numerrs Number of error codes
 * @note Checks errors[1] instead of errors[0] for multi-sensor setups
 * @note Only prints if MODBUS debug flag is enabled and no errors occurred
 *  Limits WTEMP,LIMITDO,LIMITPH,ATEMP,AHUM
 */
void cb_sensor_data(void *sensors, int *errors,char *color,int numerrs,int devAddr)
{
    bool hasErrors=false;
 
    // check errors before printing
    for (int a=0;a<numerrs;a++)
    {
        if(errors[a]!=0)
        {
            if (((theConf.debug_flags >> dMODBUS) & 1U))
                MESP_LOGE(TAG,"%s[%3d] Sensor Error CID %d=0x%x %s ",color,devAddr,a,errors[a],esp_err_to_name(errors[a]));
            hasErrors=true;
            // bzero(sensors,sizeof(sensor_t));
            theBlower.setSensors( 0.0,  0,0.0,0.0, 0.0);
        }
    }

    if (hasErrors)
    {
        globalErrors|= (1U << SENSOR_ERROR_BIT); // set sensor error bit
        if (((theConf.debug_flags >> dMODBUS) & 1U))
            printf("\n");
        return;
    }   

    globalErrors &= ~(1U << SENSOR_ERROR_BIT); // clear the error bit
     
    sensor_t *data = (sensor_t*)sensors;

    globalErrors &= ~(1U << SENSOR_LIMIT_ERROR_BIT); // clear the limit error bit
        
    if (!((theConf.debug_flags >> dMODBUS) & 1U))
        return;


    MESP_LOGI(TAG, "%s[%3d] WaterTemp:%.02f°C DO%%.%.02f%% DO:%.02fppm", color,devAddr,
             data->WTemp,
             data->percentDO * 100.0,
             data->DO);
    // save data to frame blower sensors
             theBlower.setSensors( data->DO,  0,data->WTemp,
                        temperature, 0);
}

// ============================================================================
// Energy Monitoring
// ============================================================================
 /**
 * @brief Print energy sensor data for debugging
 * 
 * Logs energy monitoring data including battery charge/discharge statistics,
 * energy generation, and consumption. Output varies based on charging state.
 * 
 * @param energy Pointer to energy_t data structure
 * @param errors Pointer to error code array
 * @param color String representing the color for logging
 * @param numerrs Number of error codes
 * @note Only prints if MODBUS debug flag is enabled and no errors occurred
 * * limits BMCHAH,BMDDAH,BMCHKT,BMDDKT,GENEER,USEDEN,LCONLI,BMCHKW,BMDDKW,GENLCT
 */
void cb_energy_data(void *energy, int *errors,char * color,int numerrs,int devAddr)
{
    bool hasErrors=false;
    // check errors before printing
    for (int a=0;a<numerrs;a++)
    {
        if(errors[a]!=0)
        {
            if (((theConf.debug_flags >> dMODBUS) & 1U))
                MESP_LOGE(TAG,"%s[%3d] EnergyError CID %d=0x%x %s ",color,devAddr,a,errors[a],esp_err_to_name(errors[a]));
            hasErrors=true;
                theBlower.setEnergy(0, 0, 0, 0, 0, 0, 0, 0, 0, 0);  // has to save 0 because sender will read from FRAM which has old values probably valid
        }
    }

    if (hasErrors)
    {
        globalErrors|= (1U << ENERGY_ERROR_BIT); // set energy error bit

        if (((theConf.debug_flags >> dMODBUS) & 1U))
            printf("\n");
        return;
    }   

    globalErrors &= ~(1U << ENERGY_ERROR_BIT); // clear the error bit
        
    
    energy_t *data = (energy_t*)energy;

    globalErrors &= ~(1U << ENERGY_LIMIT_ERROR_BIT); // clear the limit error bit

    if (!((theConf.debug_flags >> dMODBUS) & 1U))
        return;

    // Print charging or discharging data based on current state
    if (pvPanelData.chargeCurr)
    {
        MESP_LOGI(TAG, "%s[%3d] [CHARGING] - BatChgAH(Today:%u Total:%u) GenEnergy:%.02fkWh BatChg:%.02fkWh", color, devAddr,
                 data->batChgAHToday,
                 data->batChgAHTotal,
                 data->generateEnergyToday,
                 data->batChgkWhToday);   
    }
    else
    {
        MESP_LOGI(TAG, "%s[%3d] [DISCHARGING] - BatDischgAH(Today:%u Total:%u) UsedEnergy:%.02fkWh LoadConsumTotal:%.02fkWh BatDischg:%.02fkWh GenLoadConsum:%.02fkWh", color, devAddr,
                 data->batDischgAHToday,
                 data->batDischgAHTotal,
                 data->usedEnergyToday,
                 data->gLoadConsumLineTotal,
                 data->batDischgkWhToday,
                 data->genLoadConsumToday);   
    }

    // save data to frame blower Energy
    theBlower.setEnergy(data->batChgAHToday, data->batDischgAHToday, 
                       data->batChgAHTotal, data->batDischgAHTotal, 
                       data->generateEnergyToday, data->usedEnergyToday, 
                       data->gLoadConsumLineTotal, data->batChgkWhToday, 
                       data->batDischgkWhToday, data->genLoadConsumToday);
}

// ============================================================================
// Battery Status
// ============================================================================

/**
 * @brief Print battery sensor data for debugging
 * 
 * Logs battery monitoring data including State of Charge (SOC), State of Health (SOH),
 * battery cycle count, and BMS temperature.
 * 
 * @param batteryData Pointer to battery_t data structure
 * @param errors Pointer to error code array
 * @param color String representing the color for logging
 * @param numerrs Number of error codes
 * @note Only prints if MODBUS debug flag is enabled and no errors occurred
 * * limits BMSOC,BMSOH,BMCC,BMTEMP
 */
void cb_battery_data(void *batteryData, int *errors,char *color,int numerrs,int devAddr)
{
    bool hasErrors=false;
    // check errors before printing
    for (int a=0;a<numerrs;a++)
    {
        if(errors[a]!=0)
        {
            if (((theConf.debug_flags >> dMODBUS) & 1U))
                MESP_LOGE(TAG,"%s[%3d] Battery Error CID %d=0x%x %s ",color,devAddr,a,errors[a],esp_err_to_name(errors[a]));
            hasErrors=true;
                theBlower.setBattery(0, 0,0, 0);

        }
    }

    if (hasErrors)
    {
        globalErrors|= (1U << BATTERY_ERROR_BIT); // set battery error bit
        if (((theConf.debug_flags >> dMODBUS) & 1U))
            printf("\n");
        return;
    }   
     
    globalErrors &= ~(1U << BATTERY_ERROR_BIT); // clear the error bit

    battery_t *data = (battery_t*)batteryData;


    globalErrors &= ~(1U << BATTERY_LIMIT_ERROR_BIT); // clear the limit error bit                

    if (!((theConf.debug_flags >> dMODBUS) & 1U))
        return;

    MESP_LOGI(TAG, "%s[%3d] SOC:%d%% SOH:%d%% CycleCount:%d BmsTemp:%.02f°C", color, devAddr,
             data->batSOC,
             data->batSOH,
             data->batteryCycleCount,
             data->batBmsTemp);

    // save data to frame blower battery
    theBlower.setBattery(data->batSOC, data->batSOH, data->batteryCycleCount, data->batBmsTemp);
}

// ============================================================================
// Solar Panels
// ============================================================================

/**
 * @brief Print panel sensor data for debugging
 * 
 * Logs solar panel monitoring data including charge state and both PV strings'
 * voltage and current readings.
 * 
 * @param pvPanel Pointer to pvPanel_t data structure
 * @param errors Pointer to error code array
 * @param color String representing the color for logging
 * @param numerrs Number of error codes
 * @note Only prints if MODBUS debug flag is enabled and no errors occurred
 * * limits PV1V,PV1A,PV2V,PV2A
 */
void cb_panel_data(void *pvPanel, int *errors,char * color,int numerrs,int devAddr)
{
    bool hasErrors=false;
    // check errors before printing
    for (int a=0;a<numerrs;a++)
    {
        if(errors[a]!=0)
        {
            if (((theConf.debug_flags >> dMODBUS) & 1U))
                MESP_LOGE(TAG,"%s[%3d] Panels Error CID %d=0x%x %s ",color,devAddr,a,errors[a],esp_err_to_name(errors[a]));
            hasErrors=true;
            // here the logic to ERROR MANAGEMNET reporting
            theBlower.setPVPanel(0, 0, 0, 0, 0);
        }
    }

    if (hasErrors)
    {
        globalErrors|= (1U << PANELS_ERROR_BIT); // set panels error bit

        if (((theConf.debug_flags >> dMODBUS) & 1U))
            printf("\n");
        return;
    }   
    globalErrors &= ~(1U << PANELS_ERROR_BIT); // clear the error bit

    pvPanel_t *data = (pvPanel_t*)pvPanel;

    globalErrors &= ~(1U << PANELS_LIMIT_ERROR_BIT); // clear the limit error bit 

    if (!((theConf.debug_flags >> dMODBUS) & 1U))
        return;

        
    MESP_LOGI(TAG, "%s[%3d] [%s] - String1:[%.02fV / %.02fA] String2:[%.02fV / %.02fA]", color, devAddr,
             data->chargeCurr ? "CHARGING" : "DISCHARGING",
             data->pv1Volts,                    
             data->pv1Amp,
             data->pv2Volts,
             data->pv2Amp);

    // save data to frame blower Panels
    theBlower.setPVPanel(data->chargeCurr, data->pv1Volts, data->pv2Volts,data->pv1Amp, data->pv2Amp);  
}
