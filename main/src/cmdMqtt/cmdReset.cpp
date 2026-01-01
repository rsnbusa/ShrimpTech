#define GLOBAL
#include "includes.h" 
#include "globals.h"

/*
 * Handles system reset commands with various scope options. Supports full factory reset,
 * selective component resets (meter, mesh, etc.), and mode changes (configuration, reconfigure).
 * All reset operations persist changes to flash and trigger an ESP restart.
 */

extern void writeLog(char* que);
extern void write_to_flash();
extern int findFlag(char * cual);
extern void erase_config();

// Reset command constants
#define RESET_OPTION_BUFFER_SIZE 20

// Reset level types (matches findFlag() return values)
typedef enum {
    RESET_ALL = 0,
    RESET_METER = 1,
    RESET_MESH = 2,
    RESET_COLLECT = 3,
    RESET_SEND = 4,
    RESET_MQTT = 5,
    RESET_CONF = 6,
    RESET_STATUS = 7,
    RESET_INVALID = -1
} ResetLevel;

/* Converts option string to uppercase for case-insensitive matching. */
static void convert_to_uppercase(char *str)
{
    if(!str)
        return;
    for(size_t i = 0; i < strlen(str); i++)
        str[i] = toupper((unsigned char)str[i]);
}

/* Validates and parses the reset command JSON payload. */
static bool validate_reset_command(cJSON *resetCommand, char *optionBuffer, size_t bufferSize, ResetLevel *outLevel)
{
    if(!resetCommand || !optionBuffer || !outLevel)
    {
        ESP_LOGE(MESH_TAG, "Reset command validation arguments invalid");
        return false;
    }
    
    if(!cJSON_IsObject(resetCommand))
    {
        ESP_LOGE(MESH_TAG, "Reset command payload invalid");
        return false;
    }
    
    cJSON *optionNode = cJSON_GetObjectItem(resetCommand, "option");
    if(!optionNode)
    {
        ESP_LOGE(MESH_TAG, "Reset command missing option field");
        return false;
    }
    
    if(!cJSON_IsString(optionNode) || !optionNode->valuestring || strlen(optionNode->valuestring) == 0)
    {
        ESP_LOGE(MESH_TAG, "Reset option parameter invalid or empty");
        return false;
    }
    
    if(strlen(optionNode->valuestring) >= bufferSize)
    {
        ESP_LOGE(MESH_TAG, "Reset option string too long");
        return false;
    }
    
    strlcpy(optionBuffer, optionNode->valuestring, bufferSize);
    convert_to_uppercase(optionBuffer);
    
    int level = findFlag(optionBuffer);
    if(level < 0)
    {
        ESP_LOGE(MESH_TAG, "Invalid reset option: %s", optionBuffer);
        *outLevel = RESET_INVALID;
        return false;
    }
    
    *outLevel = (ResetLevel)level;
    return true;
}

/* Executes the requested reset operation based on reset level. */
static int execute_reset(ResetLevel resetLevel)
{
    switch(resetLevel) 
    {
        case RESET_ALL:
            ESP_LOGI(MESH_TAG, "Executing full factory reset");
            writeLog("All flags resetted\n");
            erase_config();
            esp_restart();
            break;
            
        case RESET_METER:
            ESP_LOGI(MESH_TAG, "Resetting meter configuration");
            writeLog("Meter resetted\n");
            theConf.meterconf = 0;
            write_to_flash();
            esp_restart();
            break;
            
        case RESET_MESH:
            ESP_LOGI(MESH_TAG, "Resetting mesh configuration");
            writeLog("Mesh resetted\n");
            // theConf.meshconf = 0;  // Commented in original
            write_to_flash();
            esp_restart();
            break;
            
        case RESET_COLLECT:
            ESP_LOGW(MESH_TAG, "RESET_COLLECT not implemented");
            break;
            
        case RESET_SEND:
            ESP_LOGW(MESH_TAG, "RESET_SEND not implemented");
            break;
            
        case RESET_MQTT:
            ESP_LOGW(MESH_TAG, "RESET_MQTT not implemented");
            break;
            
        case RESET_CONF:
            ESP_LOGI(MESH_TAG, "Setting reconfigure mode");
            writeLog("Reconfigure mode set\n");
            theConf.meterconf = 3;
            write_to_flash();
            esp_restart();
            break;
            
        case RESET_STATUS:
            ESP_LOGI(MESH_TAG, "Setting configuration mode");
            writeLog("Configuration mode set\n");
            theConf.meterconf = 4;
            write_to_flash();
            esp_restart();
            break;
            
        default:
            ESP_LOGE(MESH_TAG, "Unhandled reset level: %d", resetLevel);
            writeLog("Reset Cmd Wrong choice of reset\n");
            return ESP_FAIL;
    }
    
    return ESP_OK;
}

int cmdReset(void *argument)
{
    /*
     * Expected JSON format:
     * {"cmd":"reset","f":"xxxxxx","option":"ALL","cha":"xxxx"}
     * Valid options: "ALL", "METER", "MESH", "COLLECT", "SEND", "MQTT", "CONF", "STATUS"
     */
    
    ESP_LOGI(MESH_TAG, "Reset CMD");
    
    cJSON *resetCommand = (cJSON *)argument;
    
    if(resetCommand == NULL)
    {
        ESP_LOGE(MESH_TAG, "Reset command argument is NULL");
        return ESP_FAIL;
    }
    
    char optionBuffer[RESET_OPTION_BUFFER_SIZE] = {0};
    ResetLevel resetLevel = RESET_INVALID;
    
    if(!validate_reset_command(resetCommand, optionBuffer, RESET_OPTION_BUFFER_SIZE, &resetLevel))
        return ESP_FAIL;
    
    return execute_reset(resetLevel);
}