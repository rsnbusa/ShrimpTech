
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
#include "defines.h"
#include "typedef.h"
#include "globals.h"

void check_limits(char * who,  int *limitlocation, int count,char *color,char *names[], ...){
    va_list args;
    va_start(args, names);
    // printf("Checking limits for %s son %d\n",who,count);

    for (int i=0;i<count;i++)
    {
        int value=va_arg(args, int);
        int loc=limitlocation[i];
        // printf("Limit %s Min %d Max %d Loc %d\n",names[i],theConf.limits[loc][1],theConf.limits[loc][0],loc);
        int min=theConf.limits[loc][1];
        int max=theConf.limits[loc][0];
        if (min!=max) // limits defined
        {
            if (value<min)
            {
                if (((theConf.debug_flags >> dLIMITS) & 1U))
                    ESP_LOGW(TAG,"%s%s below minimum limit %d < %d",color,names[i],value,min);
            }
            else if (value>max)
            {
                if (((theConf.debug_flags >> dLIMITS) & 1U))
                    ESP_LOGW(TAG,"%s%s above maximum limit %d > %d",color,names[i],value,max);
            }
        }
    }
    va_end(args);
}

// ============================================================================
// Environmental Sensors
// ============================================================================

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
void print_sensor_data(void *sensors, int *errors,char *color,int numerrs)
{
    bool hasErrors=false;
    int limits_location[]={WTEMP,LIMITDO,LIMITPH,ATEMP,AHUM}; // map to correct location in errors array
    int limit_count=sizeof(limits_location)/sizeof(int);
    char *limits_names[]={"WTEMP","LIMITDO","LIMITPH","ATEMP","AHUM"};
 
    // check errors before printing
    for (int a=0;a<numerrs;a++)
    {
        if(errors[a]!=0)
        {
            if (((theConf.debug_flags >> dMODBUS) & 1U))
                ESP_LOGE(TAG,"%sSensor Error CID %d=0x%x %s ",color,a,errors[a],esp_err_to_name(errors[a]));
            hasErrors=true;
        }
    }

    if (hasErrors)
    {
        if (((theConf.debug_flags >> dMODBUS) & 1U))
            printf("\n");
        return;
    }   
     
        // should check Limits here too
    sensor_t *data = (sensor_t*)sensors;

    check_limits("Sensors",limits_location,limit_count,color,limits_names,
                (int)data->WTemp,(int)data->DO*10,
                (int)data->PH,(int)data->ATemp,(int)data->AHum); 

    if (!((theConf.debug_flags >> dMODBUS) & 1U))
        return;


    ESP_LOGI(TAG, "%sSensors - WaterTemp:%.02f°C DO%%.%.02f%% DO:%.02fppm", color,
             data->WTemp,
             data->percentDO * 100.0,
             data->DO);
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
void print_energy_data(void *energy, int *errors,char * color,int numerrs)
{
    bool hasErrors=false;
    int limits_location[]={BMCHAH,BMDDAH,BMCHKT,BMDDKT,GENEER,USEDEN,LCONLI,BMCHKW,BMDDKW,GENLCT}; // map to correct location in errors array
    int limit_count=sizeof(limits_location)/sizeof(int);
    char *limits_names[]={"BMCHAH","BMDDAH","BMCHKT","BMDDKT","GENEER","USEDEN","LCONLI","BMCHKW","BMDDKW","GENLCT"};
    // check errors before printing
    for (int a=0;a<numerrs;a++)
    {
        if(errors[a]!=0)
        {
            if (((theConf.debug_flags >> dMODBUS) & 1U))
                ESP_LOGE(TAG,"%sEnergyError CID %d=0x%x %s ",color,a,errors[a],esp_err_to_name(errors[a]));
            hasErrors=true;
        }
    }

    if (hasErrors)
    {
        if (((theConf.debug_flags >> dMODBUS) & 1U))
            printf("\n");
        return;
    }   
     
        // should check Limits here too




    energy_t *data = (energy_t*)energy;
    check_limits("Energy",limits_location,limit_count,color,limits_names, (int)data->batChgAHToday,
                (int)data->batChgAHTotal,(int)data->batChgAHTotal,(int)data->batDischgAHTotal,
                (int)data->generateEnergyToday,(int)data->usedEnergyToday,(int)data->gLoadConsumLineTotal,
                (int)data->batChgkWhToday,(int)data->batDischgkWhToday,(int)data->genLoadConsumToday); 

    if (!((theConf.debug_flags >> dMODBUS) & 1U))
        return;

    // Print charging or discharging data based on current state
    if (pvPanelData.chargeCurr)
    {
        ESP_LOGI(TAG, "%sEnergy [CHARGING] - BatChgAH(Today:%u Total:%u) GenEnergy:%.02fkWh BatChg:%.02fkWh", color,
                 data->batChgAHToday,
                 data->batChgAHTotal,
                 data->generateEnergyToday,
                 data->batChgkWhToday);   
    }
    else
    {
        ESP_LOGI(TAG, "%sEnergy [DISCHARGING] - BatDischgAH(Today:%u Total:%u) UsedEnergy:%.02fkWh LoadConsumTotal:%.02fkWh BatDischg:%.02fkWh GenLoadConsum:%.02fkWh", color,
                 data->batDischgAHToday,
                 data->batDischgAHTotal,
                 data->usedEnergyToday,
                 data->gLoadConsumLineTotal,
                 data->batDischgkWhToday,
                 data->genLoadConsumToday);   
    }
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
void print_battery_data(void *batteryData, int *errors,char *color,int numerrs)
{
    bool hasErrors=false;
    int limits_location[]={BMSOC,BMSOH,BMCC,BMTEMP}; // map to correct location in errors array  
    int limit_count=sizeof(limits_location)/sizeof(int);
    char *limits_names[]={"BMSOC","BMSOH","BMCC","BMTEMP"};
    // check errors before printing
    for (int a=0;a<numerrs;a++)
    {
        if(errors[a]!=0)
        {
            if (((theConf.debug_flags >> dMODBUS) & 1U))
                ESP_LOGE(TAG,"%sBattery Error CID %d=0x%x %s ",color,a,errors[a],esp_err_to_name(errors[a]));
            hasErrors=true;
        }
    }

    if (hasErrors)
    {
        if (((theConf.debug_flags >> dMODBUS) & 1U))
            printf("\n");
        return;
    }   
     
        // should check Limits here too


    battery_t *data = (battery_t*)batteryData;


    check_limits("Battery",limits_location,limit_count,color,limits_names, (int)data->batSOC, (int)data->batSOH,
                (int)data->batteryCycleCount, (int)data->batBmsTemp);


    if (!((theConf.debug_flags >> dMODBUS) & 1U))
        return;

    ESP_LOGI(TAG, "%sBattery - SOC:%d%% SOH:%d%% CycleCount:%d BmsTemp:%.02f°C", color,
             data->batSOC,
             data->batSOH,
             data->batteryCycleCount,
             data->batBmsTemp);
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
void print_panel_data(void *pvPanel, int *errors,char * color,int numerrs)
{
    bool hasErrors=false;
    int limits_location[]={PV1V,PV1A,PV1V,PV1A}; // map to correct location in errors array
    int limit_count=sizeof(limits_location)/sizeof(int);
    char *limits_names[]={"PV1V","PV1A","PV2V","PV2A"};
    // check errors before printing
    for (int a=0;a<numerrs;a++)
    {
        if(errors[a]!=0)
        {
            if (((theConf.debug_flags >> dMODBUS) & 1U))
                ESP_LOGE(TAG,"%sPanels Error CID %d=0x%x %s ",color,a,errors[a],esp_err_to_name(errors[a]));
            hasErrors=true;
        }
    }

    if (hasErrors)
    {
        if (((theConf.debug_flags >> dMODBUS) & 1U))
            printf("\n");
        return;
    }   

    pvPanel_t *data = (pvPanel_t*)pvPanel;

    // should check Limits here too

    check_limits("Panels",limits_location,limit_count,color,limits_names,(int)data->pv1Volts, (int)data->pv1Amp,(int) data->pv2Volts,
                (int)data->pv2Amp);

                        
    if (!((theConf.debug_flags >> dMODBUS) & 1U))
        return;

        
    ESP_LOGI(TAG, "%sPanels [%s] - String1:[%.02fV / %.02fA] String2:[%.02fV / %.02fA]", color,
             data->chargeCurr ? "CHARGING" : "DISCHARGING",
             data->pv1Volts,                    
             data->pv1Amp,
             data->pv2Volts,
             data->pv2Amp);
}
