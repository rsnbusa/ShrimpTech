#define GLOBAL
#include "includes.h"
#include "globals.h"

/*
 * Handles reading and returning the current system limits via MQTT.
 * Constructs a JSON response with all current limit values and sends them back.
 */

extern void write_to_flash();
extern void writeLog(char* que);

int cmdReadLimits(void *argument)
{
    if((theConf.debug_flags >> dLIMITS) & 1U) 
        ESP_LOGI(MESH_TAG,"ReadLimits Cmd");

    cJSON *readLimitsCommand = (cJSON *)argument;
    if(readLimitsCommand == NULL)
    {
        ESP_LOGE(MESH_TAG,"ReadLimits command argument is NULL");
        return ESP_FAIL;
    }

    // Build response JSON with all current limits
    cJSON *response = cJSON_CreateObject();
    if(!response)
    {
        ESP_LOGE(MESH_TAG,"Failed to create response JSON");
        return ESP_FAIL;
    }

    cJSON_AddStringToObject(response, "cmd", "readlimits");
    
    // PV Panels
    cJSON *pvPanels = cJSON_CreateObject();
    cJSON_AddNumberToObject(pvPanels, "VoltsMin", theConf.milim.vmin);
    cJSON_AddNumberToObject(pvPanels, "VoltsMax", theConf.milim.vmax);
    cJSON_AddNumberToObject(pvPanels, "AmpsMin", theConf.milim.amin);
    cJSON_AddNumberToObject(pvPanels, "AmpsMax", theConf.milim.amax);
    cJSON_AddItemToObject(response, "PVPanels", pvPanels);

    // Battery
    cJSON *battery = cJSON_CreateObject();
    cJSON_AddNumberToObject(battery, "SOCMin", theConf.milim.bSOCmin);
    cJSON_AddNumberToObject(battery, "SOCMax", theConf.milim.bSOCmax);
    cJSON_AddNumberToObject(battery, "SOHMin", theConf.milim.bSOHmin);
    cJSON_AddNumberToObject(battery, "SOHMax", theConf.milim.bSOHmax);
    cJSON_AddNumberToObject(battery, "CyclesMin", theConf.milim.bcyclemin);
    cJSON_AddNumberToObject(battery, "CyclesMax", theConf.milim.bcyclemax);
    cJSON_AddNumberToObject(battery, "TempMin", theConf.milim.btempmin);
    cJSON_AddNumberToObject(battery, "TempMax", theConf.milim.btempmax);
    cJSON_AddItemToObject(response, "Battery", battery);

    // Amps
    cJSON *amps = cJSON_CreateObject();
    cJSON_AddNumberToObject(amps, "ChargeTodayMin", theConf.milim.bcAhoymin);
    cJSON_AddNumberToObject(amps, "ChargeTodayMax", theConf.milim.bcAhoymax);
    cJSON_AddNumberToObject(amps, "DischargeTodayMin", theConf.milim.bdAhoymin);
    cJSON_AddNumberToObject(amps, "DischargeTodayMax", theConf.milim.bdAhoymax);
    cJSON_AddNumberToObject(amps, "ChargeTotalMin", theConf.milim.bcATotmin);
    cJSON_AddNumberToObject(amps, "ChargeTotalMax", theConf.milim.bcATotmax);
    cJSON_AddNumberToObject(amps, "DischargeTotalMin", theConf.milim.bdATotmin);
    cJSON_AddNumberToObject(amps, "DischargeTotalMax", theConf.milim.bdATotmax);
    cJSON_AddItemToObject(response, "Amps", amps);

    // Energy (kWh)
    cJSON *energy = cJSON_CreateObject();
    cJSON_AddNumberToObject(energy, "GeneratedTodayMin", theConf.milim.kwgtodaymin);
    cJSON_AddNumberToObject(energy, "GeneratedTodayMax", theConf.milim.kwgtodaymax);
    cJSON_AddNumberToObject(energy, "ConsumedTodayMin", theConf.milim.kwctodaymin);
    cJSON_AddNumberToObject(energy, "ConsumedTodayMax", theConf.milim.kwctodaymax);
    cJSON_AddNumberToObject(energy, "LoadConsumedTodayMin", theConf.milim.kwloadhoymin);
    cJSON_AddNumberToObject(energy, "LoadConsumedTodayMax", theConf.milim.kwloadhoymax);
    cJSON_AddNumberToObject(energy, "BatteryChargeTodayMin", theConf.milim.kwbatchoymin);
    cJSON_AddNumberToObject(energy, "BatteryChargeTodayMax", theConf.milim.kwbatchoymax);
    cJSON_AddNumberToObject(energy, "BatteryDischargeTodayMin", theConf.milim.kwbatdhoymin);
    cJSON_AddNumberToObject(energy, "BatteryDischargeTodayMax", theConf.milim.kwbatdhoymax);
    cJSON_AddItemToObject(response, "Energy", energy);

    // Sensors
    cJSON *sensors = cJSON_CreateObject();
    cJSON_AddNumberToObject(sensors, "DissolvedOxygenMin", theConf.milim.domin);
    cJSON_AddNumberToObject(sensors, "DissolvedOxygenMax", theConf.milim.domax);
    cJSON_AddNumberToObject(sensors, "PHMin", theConf.milim.phmin);
    cJSON_AddNumberToObject(sensors, "PHMax", theConf.milim.phmax);
    cJSON_AddNumberToObject(sensors, "WaterTempMin", theConf.milim.wtempmin);
    cJSON_AddNumberToObject(sensors, "WaterTempMax", theConf.milim.wtempmax);
    cJSON_AddNumberToObject(sensors, "AmbientTempMin", theConf.milim.atempmin);
    cJSON_AddNumberToObject(sensors, "AmbientTempMax", theConf.milim.atempmax);
    cJSON_AddNumberToObject(sensors, "HumidityMin", theConf.milim.hummin);
    cJSON_AddNumberToObject(sensors, "HumidityMax", theConf.milim.hummax);
    cJSON_AddItemToObject(response, "Sensors", sensors);

    // Convert to string and send
    char *jsonString = cJSON_PrintUnformatted(response);
    if(!jsonString)
    {
        ESP_LOGE(MESH_TAG,"Failed to print response JSON");
        cJSON_Delete(response);
        return ESP_FAIL;
    }

    if((theConf.debug_flags >> dLIMITS) & 1U) 
        ESP_LOGI(MESH_TAG,"ReadLimits response: %s", jsonString);

    // Create message for MQTT sender queue
    mqttSender_t mqttMsg = {0};
    mqttMsg.msg = (char*)calloc(strlen(jsonString) + 1, 1);
    if(!mqttMsg.msg)
    {
        ESP_LOGE(MESH_TAG,"Failed to allocate memory for limits response");
        free(jsonString);
        cJSON_Delete(response);
        return ESP_FAIL;
    }

    strcpy(mqttMsg.msg, jsonString);
    mqttMsg.lenMsg = strlen(jsonString);
    mqttMsg.code = NULL;
    mqttMsg.param = NULL;
    mqttMsg.queue =limitsQueue;
    // Send via MQTT (this will be handled by the MQTT sender task)
    if(xQueueSend(mqttSender, &mqttMsg, 0) != pdTRUE)
    {
        ESP_LOGE(MESH_TAG,"Failed to queue limits response");
        free(mqttMsg.msg);
        free(jsonString);
        cJSON_Delete(response);
        return ESP_FAIL;
    }

    free(jsonString);
    cJSON_Delete(response);

    return ESP_OK;
}
