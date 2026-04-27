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

static bool key_matches(const char *key, const char *k1, const char *k2, const char *k3)
{
    if (!key) {
        return false;
    }
    return ((k1 && strcmp(key, k1) == 0) ||
            (k2 && strcmp(key, k2) == 0) ||
            (k3 && strcmp(key, k3) == 0));
}

static cJSON *find_number_field_recursive(cJSON *node, const char *k1, const char *k2, const char *k3)
{
    if (!node) {
        return NULL;
    }

    if (cJSON_IsObject(node)) {
        for (cJSON *child = node->child; child != NULL; child = child->next) {
            if (key_matches(child->string, k1, k2, k3) && cJSON_IsNumber(child)) {
                return child;
            }

            cJSON *found = find_number_field_recursive(child, k1, k2, k3);
            if (found) {
                return found;
            }
        }
        return NULL;
    }

    if (cJSON_IsArray(node)) {
        for (cJSON *child = node->child; child != NULL; child = child->next) {
            cJSON *found = find_number_field_recursive(child, k1, k2, k3);
            if (found) {
                return found;
            }
        }
    }

    return NULL;
}

static cJSON *get_number_field(cJSON *obj, const char *k1, const char *k2, const char *k3)
{
    return find_number_field_recursive(obj, k1, k2, k3);
}

static void apply_feeder_runtime_config(cJSON *feederNode, const char *source)
{
    (void)source;

    if (!cJSON_IsObject(feederNode)) {
        return;
    }

    cJSON *field = NULL;

    field = get_number_field(feederNode, "time_opne_close_ms", "time_open_close_ms", "full_open_close_ms");
    if (field)
    {
        theConf.feederData.full = field->valueint;
    }

    // Treat all clear-line keys as seconds (legacy key names may still include "min").
    field = get_number_field(feederNode, "lineclear", "clear_line_time_min", "line_clear_min");
    if (field && field->valueint >= 0)
    {
        theConf.feederData.lineclear = field->valueint;
    }
    else
    {
        field = get_number_field(feederNode, "clear_line_time_sec", "line_clear_sec", NULL);
        if (field && field->valueint >= 0)
        {
            theConf.feederData.lineclear = field->valueint;
        }
    }

    field = get_number_field(feederNode, "dispenser_g_per_liter", "gramsliter", "grams_per_liter");
    if (field)
    {
        theConf.feederData.gramsliter = field->valueint;
    }

    field = get_number_field(feederNode, "number_of_lines", "numlines", "lines");
    if (field)
    {
        theConf.feederData.numlines = field->valueint;
    }

    field = get_number_field(feederNode, "feeder_flow_lpm", "feederFlow", "feeder_flow");
    if (field && field->valueint >= 0)
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

    cJSON *feederNode = cJSON_GetObjectItem(feedProfileCommand, "feeder");
    if (!feederNode)
    {
        MESP_LOGE(MESH_TAG, "Feed profile schedule missing required fields");
        return ESP_FAIL;
    }

    // Parse runtime feeder config from likely locations. Recursive key lookup
    // also finds values nested deeper than these entry nodes.
    apply_feeder_runtime_config(feedProfileCommand, "root");
    apply_feeder_runtime_config(feederNode, "feeder");
    apply_feeder_runtime_config(cJSON_GetObjectItem(feedProfileCommand, "schedule"), "schedule");
    apply_feeder_runtime_config(cJSON_GetObjectItem(feedProfileCommand, "config"), "config");
    apply_feeder_runtime_config(cJSON_GetObjectItem(feedProfileCommand, "feeder_config"), "feeder_config");

    char *profileStr = cJSON_PrintUnformatted(feederNode);
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

    MESP_LOGI(MESH_TAG, "Feed profile parameters updated: numlines=%d gramsliter=%d feederFlow=%u, saving to flash...",
              theConf.feederData.numlines, theConf.feederData.gramsliter, theConf.feederFlow);
    write_to_flash();
    writeLog("Feed profile updated via mqtt command, rebooting to apply changes");
    vTaskDelay(pdMS_TO_TICKS(1000));

    int *p = NULL;
    *p = 0;   // crash the system to trigger a reboot, as esp_restart is not functioning correctly in this context
    return ESP_OK;
}
