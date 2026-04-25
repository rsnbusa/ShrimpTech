#define GLOBAL
#include "includes.h"
#include "globals.h"

/*
 * Handles runtime MQTT feeder profile updates. Reuses the existing profile parser,
 * copies parsed content into feedprofiles, restores production profiles, persists,
 * and reboots so the feeder schedule is applied consistently.
 */

extern void write_to_flash();
extern void writeLog(const char* que);
extern err_t make_profile(char * prof);

static void apply_feeder_runtime_config(cJSON *feederNode)
{
    cJSON *field = NULL;

    field = cJSON_GetObjectItem(feederNode, "time_opne_close_ms");
    if(!field)
    {
        field = cJSON_GetObjectItem(feederNode, "time_open_close_ms");
    }
    if(cJSON_IsNumber(field))
    {
        theConf.feederData.full = field->valueint;
    }

    field = cJSON_GetObjectItem(feederNode, "clear_line_time_sec");
    if(cJSON_IsNumber(field))
    {
        theConf.feederData.lineclear = field->valueint;
    }

    field = cJSON_GetObjectItem(feederNode, "dispenser_g_per_liter");
    if(cJSON_IsNumber(field))
    {
        theConf.feederData.gramsliter = field->valueint;
    }

    field = cJSON_GetObjectItem(feederNode, "number_of_lines");
    if(cJSON_IsNumber(field))
    {
        theConf.feederData.numlines = field->valueint;
    }

    field = cJSON_GetObjectItem(feederNode, "feeder_flow_lpm");
    if(cJSON_IsNumber(field) && field->valueint >= 0)
    {
        theConf.feederFlow = (uint16_t)field->valueint;
    }
}

int cmdFeedProfile(void *argument)
{
    MESP_LOGW(MESH_TAG, "Cmd FeedProfile received!!!");
    cJSON *feedProfileCommand = (cJSON *)argument;

    /*
    {"cmd":"feedprofile","schedule":{...}}
    */

    if (feedProfileCommand == NULL)
    {
        MESP_LOGE(MESH_TAG, "Feed profile command argument is NULL");
        return ESP_FAIL;
    }

    cJSON *scheduleNode = cJSON_GetObjectItem(feedProfileCommand, "feeder");
    if (!scheduleNode)
    {
        MESP_LOGE(MESH_TAG, "Feed profile schedule missing required fields");
        return ESP_FAIL;
    }

    apply_feeder_runtime_config(scheduleNode);

    char *profileStr = cJSON_PrintUnformatted(scheduleNode);
    if (!profileStr)
    {
        MESP_LOGE(MESH_TAG, "Failed to serialize feed profiles");
        return ESP_FAIL;
    }

    profile_t profileBackup[MAXPROFILES];
    memcpy(profileBackup, theConf.profiles, sizeof(profileBackup));

    memset(theConf.profiles, 0, sizeof(theConf.profiles));
    err_t err = make_profile(profileStr);
    free(profileStr);

    if (err != ESP_OK)
    {
        memcpy(theConf.profiles, profileBackup, sizeof(profileBackup));
        MESP_LOGE(MESH_TAG, "Failed to apply feed profile configuration");
        return ESP_FAIL;
    }

    memcpy(theConf.feedprofiles, theConf.profiles, sizeof(theConf.feedprofiles));
    memcpy(theConf.profiles, profileBackup, sizeof(profileBackup));

    MESP_LOGI(MESH_TAG, "Feed profile parameters updated successfully, rebooting to apply changes...");
    write_to_flash();
    writeLog("Feed profile updated via mqtt command, rebooting to apply changes");
    vTaskDelay(pdMS_TO_TICKS(1000));

    int *p = NULL;
    *p = 0;   // crash the system to trigger a reboot, as esp_restart is not functioning correctly in this context
    return ESP_OK;
}
