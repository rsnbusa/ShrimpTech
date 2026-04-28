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

/* Recursively searches for the first array-type field whose key matches k1/k2/k3. */
static cJSON *find_array_field_recursive(cJSON *node, const char *k1, const char *k2, const char *k3)
{
    if (!node) {
        return NULL;
    }

    if (cJSON_IsObject(node)) {
        for (cJSON *child = node->child; child != NULL; child = child->next) {
            if (key_matches(child->string, k1, k2, k3) && cJSON_IsArray(child)) {
                return child;
            }
            cJSON *found = find_array_field_recursive(child, k1, k2, k3);
            if (found) {
                return found;
            }
        }
        return NULL;
    }

    if (cJSON_IsArray(node)) {
        for (cJSON *child = node->child; child != NULL; child = child->next) {
            cJSON *found = find_array_field_recursive(child, k1, k2, k3);
            if (found) {
                return found;
            }
        }
    }

    return NULL;
}

static void apply_feeder_lines_array(cJSON *feederNode)
{
    if (!feederNode) {
        return;
    }

    /* Search recursively — lines may be nested inside profiles[] */
    cJSON *lines = find_array_field_recursive(feederNode, "lines", NULL, NULL);
    if (!cJSON_IsArray(lines)) {
        return;
    }

    const int max_lines = (int)(sizeof(theConf.feederData.lines) / sizeof(theConf.feederData.lines[0]));
    const int line_count = cJSON_GetArraySize(lines);
    const int limit = (line_count < max_lines) ? line_count : max_lines;

    for (int i = 0; i < max_lines; i++) {
        theConf.feederData.lines[i].linenum = 0;
        theConf.feederData.lines[i].length_m = 0;
        theConf.feederData.lines[i].l_c_t = 0;
    }

    for (int i = 0; i < limit; i++) {
        cJSON *lineObj = cJSON_GetArrayItem(lines, i);
        if (!cJSON_IsObject(lineObj)) {
            continue;
        }

        cJSON *field = NULL;

        field = get_number_field(lineObj, "linenum", "line", "line_number");
        if (field) {
            theConf.feederData.lines[i].linenum = field->valueint;
        }

        field = get_number_field(lineObj, "length_m", "length", "line_length_m");
        if (field) {
            theConf.feederData.lines[i].length_m = field->valueint;
        }

        field = get_number_field(lineObj, "l_c_t", "line_clear_time", "line_clear_time_min");
        if (field) {
            theConf.feederData.lines[i].l_c_t = field->valueint;
        }
    }

    theConf.feederData.numlines = limit;
}

static void apply_feeder_runtime_config(cJSON *feederNode, const char *source)
{
    (void)source;

    if (!cJSON_IsObject(feederNode)) {
        return;
    }

    cJSON *field = NULL;

    field = get_number_field(feederNode, "time_open_close_ms", "full_open_close_ms", "line_open_close_ms");
    if (field)
    {
        theConf.feederData.full = field->valueint;
    }

    field = get_number_field(feederNode, "dispenser_g_per_liter", "gramsliter", "grams_per_liter");
    if (field)
    {
        theConf.feederData.gramsliter = field->valueint;
    }

    field = get_number_field(feederNode, "number_of_lines", "numlines", NULL);
    if (field)
    {
        theConf.feederData.numlines = field->valueint;
    }

    apply_feeder_lines_array(feederNode);

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
    for (int i = 0; i < theConf.feederData.numlines; i++)
    {
        MESP_LOGI(MESH_TAG, "Feed line[%d]: linenum=%d length_m=%d l_c_t=%d",
                  i,
                  theConf.feederData.lines[i].linenum,
                  theConf.feederData.lines[i].length_m,
                  theConf.feederData.lines[i].l_c_t);
    }
    write_to_flash();
    writeLog("Feed profile updated via mqtt command, rebooting to apply changes");
    vTaskDelay(pdMS_TO_TICKS(1000));

    int *p = NULL;
    *p = 0;   // crash the system to trigger a reboot, as esp_restart is not functioning correctly in this context
    return ESP_OK;
}
