#define GLOBAL
#include "includes.h"
#include "globals.h"

/*
 * Handles WiFi network reconfiguration commands sent over mesh. Validates and persists
 * new SSID/password credentials to flash, optionally triggering a device reboot so the
 * new network settings take effect immediately.
 */

extern void write_to_flash();
extern void writeLog(char* que);

// Network command constants
#define NETWORK_PASSWORD_MIN_LENGTH 8
#define NETWORK_LOG_BUFFER_SIZE 100
#define NETWORK_RESTART_DELAY_MS 3000

typedef struct
{
    const char *ssid;
    const char *password;
} NetworkCommandFields;

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

/* Validates the JSON payload and extracts the WiFi credential fields if all checks pass. */
static bool validate_network_command(cJSON *networkCommand, NetworkCommandFields *outFields)
{
    if(!networkCommand || !outFields)
    {
        ESP_LOGE(MESH_TAG,"Network command argument invalid");
        return false;
    }

    if(!cJSON_IsObject(networkCommand))
    {
        ESP_LOGE(MESH_TAG,"Network command payload invalid");
        return false;
    }

    cJSON *ssidNode = cJSON_GetObjectItem(networkCommand,"ssid");
    cJSON *passwordNode = cJSON_GetObjectItem(networkCommand,"ssidpassw");

    if(!ssidNode || !passwordNode)
    {
        ESP_LOGE(MESH_TAG,"Network command missing required fields (ssid or password)");
        return false;
    }

    if(!cJSON_IsString(ssidNode) || !ssidNode->valuestring || strlen(ssidNode->valuestring) == 0)
    {
        ESP_LOGE(MESH_TAG,"Network SSID parameter invalid or empty");
        return false;
    }

    if(!cJSON_IsString(passwordNode) || !passwordNode->valuestring)
    {
        ESP_LOGE(MESH_TAG,"Network password parameter invalid");
        return false;
    }

    if(strlen(passwordNode->valuestring) < NETWORK_PASSWORD_MIN_LENGTH)
    {
        ESP_LOGW(MESH_TAG,"Network password too short (%d chars, minimum %d)", 
                 (int)strlen(passwordNode->valuestring), NETWORK_PASSWORD_MIN_LENGTH);
        return false;
    }

    outFields->ssid = ssidNode->valuestring;
    outFields->password = passwordNode->valuestring;
    return true;
}

/* Copies the validated WiFi credentials into persistent config and writes them to flash. */
static bool apply_network_config(const NetworkCommandFields *fields)
{
    if(!fields)
        return false;

    memset(theConf.thessid, 0, sizeof(theConf.thessid));
    memset(theConf.thepass, 0, sizeof(theConf.thepass));

    if(!copy_string_safe(theConf.thessid, sizeof(theConf.thessid), fields->ssid))
    {
        ESP_LOGE(MESH_TAG,"Failed to copy network SSID (too long)");
        return false;
    }

    if(!copy_string_safe(theConf.thepass, sizeof(theConf.thepass), fields->password))
    {
        ESP_LOGE(MESH_TAG,"Failed to copy network password (too long)");
        return false;
    }

    write_to_flash();
    return true;
}

/* Logs the WiFi credential update to the audit trail. */
static void log_network_update(const char *configuredSsid)
{
    char *logBuffer = (char*)calloc(NETWORK_LOG_BUFFER_SIZE, 1);
    if(logBuffer)
    {
        snprintf(logBuffer, NETWORK_LOG_BUFFER_SIZE, "Network change SSID %s", configuredSsid ? configuredSsid : "");
        writeLog(logBuffer);
        free(logBuffer);
    }
}

/* Flushes logs and triggers system restart to apply new WiFi credentials. */
static void reboot_device_if_requested(cJSON *rebootNode)
{
    if(rebootNode && cJSON_IsString(rebootNode) && rebootNode->valuestring)
    {
        if(strcmp(rebootNode->valuestring, "Y") == 0)
        {
            ESP_LOGI(MESH_TAG,"Reboot requested, restarting device");
            fclose(myFile);
            vTaskDelay(NETWORK_RESTART_DELAY_MS / portTICK_PERIOD_MS);
            esp_restart();
        }
    }
}

int cmdNetw(void *argument)
{
    ESP_LOGI(MESH_TAG,"Netw Cmd");
    
    cJSON *networkCommand = (cJSON *)argument;
    
    if(networkCommand == NULL)
    {
        ESP_LOGE(MESH_TAG,"Network command argument is NULL");
        return ESP_FAIL;
    }
    
    /*
     * Expected JSON format:
     * {"cmd":"newt","f":"123456","ssid":"myssid","ssidpassw":"qweret","reboot":"Y/N"}
     */

    NetworkCommandFields fields = {0};
    if(!validate_network_command(networkCommand, &fields))
        return ESP_FAIL;

    if(!apply_network_config(&fields))
        return ESP_FAIL;

    ESP_LOGI(MESH_TAG,"Network credentials updated");
    log_network_update(theConf.thessid);

    cJSON *rebootNode = cJSON_GetObjectItem(networkCommand,"reboot");
    reboot_device_if_requested(rebootNode);

    return ESP_OK;
}