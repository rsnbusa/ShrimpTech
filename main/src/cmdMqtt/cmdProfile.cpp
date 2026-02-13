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

// MQTT command validation constants
#define MQTT_SERVER_MIN_LENGTH 20
#define MQTT_LOG_BUFFER_SIZE 100

typedef struct
{
    const char *schedule;
} ProfileCommandFields;

static bool copy_string_safe(char *dest, size_t destSize, const char *src)
{
    if(!dest || !destSize)
        return false;
    if(!src)
    {
        dest[0] = '\0';
        return false;
    }
    size_t written = strlcpy(dest, src, destSize);
    return written < destSize;
}

/* Validates the JSON payload and surfaces the required string pointers if all checks pass. */
static bool validate_profile_command(cJSON *profileCommand, ProfileCommandFields *outFields)
{
    if(!profileCommand || !outFields)
    {
        MESP_LOGE(MESH_TAG,"Profile command argument invalid");
        return false;
    }

    if(!cJSON_IsObject(profileCommand))
    {
        MESP_LOGE(MESH_TAG,"Profile command payload invalid");
        return false;
    }

    cJSON *scheduleNode = cJSON_GetObjectItem(profileCommand,"schedule");

    if( !scheduleNode)
    {
        MESP_LOGE(MESH_TAG,"Profile command missing required fields");
        return false;
    }

    if( !scheduleNode->valuestring)
    {
        MESP_LOGE(MESH_TAG,"Schedule parameters invalid");
        return false;
    }

    outFields->schedule = scheduleNode->valuestring;
    return true;
}

/* Copies the validated fields into the persistent config and writes them to flash. */
static bool apply_profile_config(const ProfileCommandFields *fields)
{
    if(!fields)
        return false;

    err_t err=make_profile((char*)fields->schedule);
    if(err != ESP_OK)    {
        MESP_LOGE(MESH_TAG,"Failed to apply profile configuration");
        return false;
    }
    write_to_flash();
    return true;
}

/*
 * Emits two audit log lines: one reflecting what is stored and another indicating the
 * requested profile. Keeping both helps when diagnosing mismatches between command payload
 * and persisted configuration.
 */
static void log_profile_update(const char *configuredProfile, const char *requestedProfile)
{
    char *logBuffer = (char*)calloc(MQTT_LOG_BUFFER_SIZE,1);
    if(logBuffer)
    {
        snprintf(logBuffer,MQTT_LOG_BUFFER_SIZE,"Profile Command Profile %s",configuredProfile ? configuredProfile : "");
        writeLog(logBuffer);
        free(logBuffer);
    }

    char *installLog = (char*)calloc(MQTT_LOG_BUFFER_SIZE,1);
    if(installLog)
    {
        snprintf(installLog,MQTT_LOG_BUFFER_SIZE,"Profile installed %s",requestedProfile ? requestedProfile : "");
        writeLog(installLog);
        free(installLog);
    }
}

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

    ProfileCommandFields fields = {0};
    if(!validate_profile_command(profileCommand, &fields))
        return ESP_FAIL;

    if(!apply_profile_config(&fields))
        return ESP_FAIL;

    MESP_LOGI(MESH_TAG,"Profile parameters updated");
    // log_profile_update(theConf.profile, fields.profile);

    /* Allow log flush before forcing a reboot so clients reconnect with new broker data. */
    delay(1000);
    esp_restart();

    return ESP_OK;
}

