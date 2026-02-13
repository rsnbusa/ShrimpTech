#define GLOBAL
#include "includes.h" 
#include "globals.h"
#include "time_utils.h"

/*
 * Handles runtime MQTT reconfiguration pushed over mesh. Parses the incoming JSON payload,
 * validates the parameters, updates persistent credentials, logs the change, and reboots the
 * device so the new broker settings take effect immediately.
 */

extern void write_to_flash();
extern void writeLog(char* que);
extern err_t make_profile(char * prof);

int cmdProfile(void *argument)
{
    MESP_LOGW(MESH_TAG,"Cmd Profile received!!!");
    cJSON *profileCommand = (cJSON *)argument;

    /*
    {"cmd":"profile","schedule":""}
    */

    if(profileCommand == NULL)
    {
        MESP_LOGE(MESH_TAG,"Profile command argument is NULL");
        return ESP_FAIL;
    }

    cJSON *scheduleNode = cJSON_GetObjectItem(profileCommand,"schedule");

    if(!scheduleNode)
    {
        MESP_LOGE(MESH_TAG,"Profile schedule missing required fields");
        return ESP_FAIL;
    }

    char *profileStr = cJSON_PrintUnformatted(scheduleNode);
    if(!profileStr)        {
        MESP_LOGE(MESH_TAG,"Failed to serialize profiles for logging");
        return ESP_FAIL;
    }

    err_t err=make_profile(profileStr);
    if(err != ESP_OK)    {
        MESP_LOGE(MESH_TAG,"Failed to apply profile configuration");
        return false;
    }
    free(profileStr);
    MESP_LOGI(MESH_TAG,"Profile parameters updated successfully, rebooting to apply changes...");
    write_to_flash();
    writeLog("Profile updated via mqtt command, rebooting to apply changes");
    vTaskDelay(pdMS_TO_TICKS(1000));

    int *p=NULL;
    *p=0;   // crash the system to trigger a reboot, as esp_restart is not functioning correctly in this context
    return ESP_OK;
}

