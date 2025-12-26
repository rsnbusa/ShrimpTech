#ifndef TYPESformat_H_
#define TYPESformat_H_
#define GLOBAL
#include "includes.h" 
#include "globals.h"

extern void writeLog(char* que);

// Format command constants
#define MESH_MESSAGE_BUFFER_SIZE 100
#define MESH_UNION_CMD_SIZE 4
#define LOG_BUFFER_SIZE 100

/**
 * @brief Validates that the format command contains required fields
 * @param cmd The cJSON command object to validate
 * @return Pointer to the meter ID node if valid, NULL otherwise
 */
static cJSON* validateFormatCommand(cJSON *cmd)
{
    if(!cmd)
    {
        ESP_LOGE(MESH_TAG, "Format command is NULL");
        return NULL;
    }
    
    cJSON *meterIdNode = cJSON_GetObjectItem(cmd, "mid");
    if(!meterIdNode)
    {
        ESP_LOGE(MESH_TAG, "Format command missing required 'mid' field");
        return NULL;
    }
    
    return meterIdNode;
}

/**
 * @brief Logs the format operation with meter ID
 * @param meterIdNode The cJSON node containing the meter ID
 */
static void logFormatOperation(cJSON *meterIdNode)
{
    if(!meterIdNode || !meterIdNode->valuestring)
    {
        ESP_LOGW(MESH_TAG, "Cannot log format operation: invalid meter ID");
        return;
    }
    
    char *logBuffer = (char*)malloc(LOG_BUFFER_SIZE);
    if(logBuffer)
    {
        snprintf(logBuffer, LOG_BUFFER_SIZE, "Format Mid %s", meterIdNode->valuestring);
        writeLog(logBuffer);
        free(logBuffer);
    }
    else
    {
        ESP_LOGW(MESH_TAG, "Failed to allocate log buffer for format operation");
    }
}

/**
 * @brief Builds a mesh union message from JSON command
 * @param jsonString The JSON command string
 * @param meshMessage Pointer to store the allocated mesh message (caller must free)
 * @return ESP_OK on success, ESP_FAIL on allocation failure
 */
static esp_err_t buildMeshMessageFormat(const char *jsonString, meshunion_t **meshMessage)
{
    if(!jsonString || !meshMessage)
    {
        ESP_LOGE(MESH_TAG, "Invalid parameters for buildMeshMessage");
        return ESP_FAIL;
    }
    
    *meshMessage = (meshunion_t*)calloc(1, MESH_MESSAGE_BUFFER_SIZE);
    if(!*meshMessage)
    {
        ESP_LOGE(MESH_TAG, "Failed to allocate mesh message buffer");
        return ESP_FAIL;
    }
    
    (*meshMessage)->cmd = MESH_INT_DATA_CJSON;
    
    // Copy JSON payload after the command field
    void *payloadPtr = (void*)(*meshMessage) + MESH_UNION_CMD_SIZE;
    memcpy(payloadPtr, jsonString, strlen(jsonString));
    
    return ESP_OK;
}

/**
 * @brief Sends format command to mesh network
 * @param meshMessage The mesh union message to send
 * @param jsonStringLen Length of the JSON string in the message
 * @param meterIdNode The meter ID node for logging
 * @return ESP_OK on success, error code otherwise
 */
static esp_err_t sendFormatCommandToMesh(meshunion_t *meshMessage, size_t jsonStringLen, cJSON *meterIdNode)
{
    if(!meshMessage)
    {
        ESP_LOGE(MESH_TAG, "Mesh message is NULL");
        return ESP_FAIL;
    }
    
    mesh_data_t data;
    data.proto = MESH_PROTO_BIN;
    data.tos = MESH_TOS_P2P;
    data.data = (uint8_t*)meshMessage;
    data.size = jsonStringLen + MESH_UNION_CMD_SIZE;
    
    int err = esp_mesh_send(&GroupID, &data, MESH_DATA_P2P, NULL, MESH_OPT_SEND_GROUP);
    
    if(err != ESP_OK)
    {
        ESP_LOGE(MESH_TAG, "Failed to send format command via mesh: %s", esp_err_to_name(err));
    }
    else if(meterIdNode && meterIdNode->valuestring)
    {
        ESP_LOGI(MESH_TAG, "Format command sent successfully to node: %s", meterIdNode->valuestring);
    }
    
    return err;
}

int cmdFormat(void *argument)
{
    cJSON *cmd = (cJSON *)argument;
    ESP_LOGI(MESH_TAG, "Format CMD");
    
    /*
    Command format:
    {"cmd":"format","f":"xxxxxx","mid":"thismeter","erase":"Y"}
    */
    
    // Validate command has required fields
    cJSON *meterIdNode = validateFormatCommand(cmd);
    if(!meterIdNode)
    {
        return ESP_FAIL;
    }
    
    // Serialize command to JSON string
    char *jsonString = cJSON_PrintUnformatted(cmd);
    if(!jsonString)
    {
        ESP_LOGE(MESH_TAG, "Failed to serialize format command to JSON");
        return ESP_FAIL;
    }
    
    // Log the format operation
    logFormatOperation(meterIdNode);
    
    // Build mesh message
    meshunion_t *meshMessage = NULL;
    esp_err_t err = buildMeshMessageFormat(jsonString, &meshMessage);
    if(err != ESP_OK)
    {
        free(jsonString);
        return err;
    }
    
    // Send command to mesh network
    err = sendFormatCommandToMesh(meshMessage, strlen(jsonString), meterIdNode);
    
    // Cleanup
    free(jsonString);
    free(meshMessage);
    
    return err;
}
#endif