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
extern void writeLog(const char* que);

// MQTT command validation constants
#define MQTT_SERVER_MIN_LENGTH 20
#define MQTT_LOG_BUFFER_SIZE 100

typedef struct
{
    const char *server;
    const char *user;
    const char *password;
    const char *certificate;
} MqttCommandFields;

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
static bool validate_mqtt_command(cJSON *mqttCommand, MqttCommandFields *outFields)
{
    if(!mqttCommand || !outFields)
    {
        MESP_LOGE(MESH_TAG,"MQTT command argument invalid");
        return false;
    }

    if(!cJSON_IsObject(mqttCommand))
    {
        MESP_LOGE(MESH_TAG,"MQTT command payload invalid");
        return false;
    }

    cJSON *serverNode = cJSON_GetObjectItem(mqttCommand,"server");
    cJSON *userNode = cJSON_GetObjectItem(mqttCommand,"userm");
    cJSON *passwordNode = cJSON_GetObjectItem(mqttCommand,"passm");
    cJSON *certNode = cJSON_GetObjectItem(mqttCommand,"cert");

    if(!serverNode || !userNode || !passwordNode || !certNode)
    {
        MESP_LOGE(MESH_TAG,"MQTT command missing required fields");
        return false;
    }

    if(!cJSON_IsString(serverNode) || !serverNode->valuestring)
    {
        MESP_LOGE(MESH_TAG,"MQTT server parameter invalid");
        return false;
    }

    if(strlen(serverNode->valuestring) <= MQTT_SERVER_MIN_LENGTH)
    {
        MESP_LOGW(MESH_TAG,"MQTT server parameter too short (%d chars)", (int)strlen(serverNode->valuestring));
        return false;
    }

    if(!cJSON_IsString(userNode) || !userNode->valuestring || strlen(userNode->valuestring) == 0)
    {
        MESP_LOGW(MESH_TAG,"MQTT user parameter invalid");
        return false;
    }

    if(!cJSON_IsString(passwordNode) || !passwordNode->valuestring || strlen(passwordNode->valuestring) == 0)
    {
        MESP_LOGW(MESH_TAG,"MQTT password parameter invalid");
        return false;
    }

    if(!cJSON_IsString(certNode) || !certNode->valuestring || strlen(certNode->valuestring) == 0)
    {
        MESP_LOGW(MESH_TAG,"MQTT certificate parameter invalid");
        return false;
    }

    outFields->server = serverNode->valuestring;
    outFields->user = userNode->valuestring;
    outFields->password = passwordNode->valuestring;
    outFields->certificate = certNode->valuestring;
    return true;
}

/* Copies the validated fields into the persistent config and writes them to flash. */
static bool apply_mqtt_config(const MqttCommandFields *fields)
{
    if(!fields)
        return false;

    memset(theConf.mqttcert, 0, sizeof(theConf.mqttcert));
    memset(theConf.mqttServer, 0, sizeof(theConf.mqttServer));
    memset(theConf.mqttUser, 0, sizeof(theConf.mqttUser));
    memset(theConf.mqttPass, 0, sizeof(theConf.mqttPass));

    if(!copy_string_safe(theConf.mqttServer, sizeof(theConf.mqttServer), fields->server))
    {
        MESP_LOGE(MESH_TAG,"Failed to copy MQTT server parameter");
        return false;
    }

    if(!copy_string_safe(theConf.mqttUser, sizeof(theConf.mqttUser), fields->user))
    {
        MESP_LOGE(MESH_TAG,"Failed to copy MQTT user parameter");
        return false;
    }

    if(!copy_string_safe(theConf.mqttPass, sizeof(theConf.mqttPass), fields->password))
    {
        MESP_LOGE(MESH_TAG,"Failed to copy MQTT password parameter");
        return false;
    }

    size_t certLength = strlen(fields->certificate);
    if(certLength + 2 > sizeof(theConf.mqttcert))
    {
        MESP_LOGE(MESH_TAG,"MQTT certificate too large (%d bytes)", (int)certLength);
        return false;
    }

    memcpy(theConf.mqttcert, fields->certificate, certLength);
    size_t storedLength = certLength;
    if(storedLength == 0 || theConf.mqttcert[storedLength - 1] != '\n')
    {
        theConf.mqttcert[storedLength++] = '\n';
    }
    theConf.mqttcert[storedLength] = '\0';
    theConf.mqttcertlen = storedLength + 1; // include null terminator

    write_to_flash();
    return true;
}

/*
 * Emits two audit log lines: one reflecting what is stored and another indicating the
 * requested broker. Keeping both helps when diagnosing mismatches between command payload
 * and persisted configuration.
 */
static void log_mqtt_update(const char *configuredServer, const char *requestedServer)
{
    char *logBuffer = (char*)calloc(MQTT_LOG_BUFFER_SIZE,1);
    if(logBuffer)
    {
        snprintf(logBuffer,MQTT_LOG_BUFFER_SIZE,"MQTT Command Server %s",configuredServer ? configuredServer : "");
        writeLog(logBuffer);
        free(logBuffer);
    }

    char *installLog = (char*)calloc(MQTT_LOG_BUFFER_SIZE,1);
    if(installLog)
    {
        snprintf(installLog,MQTT_LOG_BUFFER_SIZE,"MQTT installed %s",requestedServer ? requestedServer : "");
        writeLog(installLog);
        free(installLog);
    }
}

int cmdMQTT(void *argument)
{
    MESP_LOGW(MESH_TAG,"Cmd MQTT in force!!!");
    cJSON *mqttCommand = (cJSON *)argument;

    /*
    {"cmd":"mqtt","f":"000000","passw":"xxxx","server":"xxxxxxx","userm":"cccccc","passm":"xxxxxx","cert":"at least 1800 bytes"}
    */

    if(mqttCommand == NULL)
    {
        MESP_LOGE(MESH_TAG,"MQTT command argument is NULL");
        return ESP_FAIL;
    }

    MqttCommandFields fields = {};
    if(!validate_mqtt_command(mqttCommand, &fields))
        return ESP_FAIL;

    if(!apply_mqtt_config(&fields))
        return ESP_FAIL;

    MESP_LOGI(MESH_TAG,"MQTT server parameters updated");
    log_mqtt_update(theConf.mqttServer, fields.server);

    /* Allow log flush before forcing a reboot so clients reconnect with new broker data. */
    delay(1000);
    esp_restart();

    return ESP_OK;
}

