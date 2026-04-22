/**
 * @file cmdEraseMetrics.cpp
 * @brief Command handler for erasing metrics on remote ESP-MESH nodes
 * 
 * This module handles the "erase" command which instructs remote nodes in the mesh
 * network to erase their stored metrics data. The command is validated locally and
 * then broadcast via ESP-MESH to the appropriate node identified by meter ID.
 * 
 * Command Format:
 *   {"cmd":"erase","f":"xxxxxx","mid":"thismeter"}
 * 
 * Where:
 *   - cmd: Command type (must be "erase")
 *   - f: Authentication/authorization field
 *   - mid: Meter ID of the target node
 * 
 * Usage Example:
 *   cJSON *cmd = cJSON_Parse("{\"cmd\":\"erase\",\"f\":\"auth123\",\"mid\":\"meter001\"}");
 *   int result = cmdEraseMetrics(cmd);
 *   if(result == ESP_OK) {
 *       // Command sent successfully
 *   }
 * 
 * @note The actual erase operation is performed by the target node after receiving
 *       the command. This function only validates and transmits the command.
 * 
 * @dependencies
 *   - ESP-MESH network must be initialized and connected
 *   - cJSON library for JSON parsing
 *   - writeLog() function for operation logging
 */


#define GLOBAL
#include "includes.h" 
#include "globals.h"

extern void writeLog(const char* que);

// Erase metrics command constants
#define MESH_MESSAGE_BUFFER_SIZE sizeof(meshunion_t)
#define MESH_UNION_CMD_SIZE 4
#define LOG_BUFFER_SIZE 100

/**
 * @brief Validates that the erase metrics command contains required fields
 * @param cmd The cJSON command object to validate
 * @return Pointer to the meter ID node if valid, NULL otherwise
 */
static cJSON* validateEraseCommand(cJSON *cmd)
{
    if(!cmd)
    {
        MESP_LOGE(MESH_TAG, "Erase metrics command is NULL");
        return NULL;
    }
    
    cJSON *meterIdNode = cJSON_GetObjectItem(cmd, "mid");
    if(!meterIdNode)
    {
        MESP_LOGE(MESH_TAG, "Erase metrics command missing required 'mid' field");
        return NULL;
    }
    
    return meterIdNode;
}

/**
 * @brief Logs the erase metrics operation with meter ID
 * @param meterIdNode The cJSON node containing the meter ID
 */
static void logEraseOperation(cJSON *meterIdNode)
{
    if(!meterIdNode || !meterIdNode->valuestring)
    {
        MESP_LOGW(MESH_TAG, "Cannot log erase operation: invalid meter ID");
        return;
    }
    
    char *logBuffer = (char*)malloc(LOG_BUFFER_SIZE);
    if(logBuffer)
    {
        snprintf(logBuffer, LOG_BUFFER_SIZE, "EraseMetrics Mid %s", meterIdNode->valuestring);
        writeLog(logBuffer);
        free(logBuffer);
    }
    else
    {
        MESP_LOGW(MESH_TAG, "Failed to allocate log buffer for erase metrics operation");
    }
}

/**
 * @brief Builds a mesh union message from JSON command
 * @param jsonString The JSON command string to embed in the mesh message
 * @param meshMessage Pointer to store the allocated mesh message (caller must free)
 * @return ESP_OK on success, ESP_FAIL on allocation failure
 * 
 * @note Allocates MESH_MESSAGE_BUFFER_SIZE bytes. Caller is responsible for freeing.
 */
static esp_err_t buildMeshMessageErase(const char *jsonString, meshunion_t **meshMessage)
{
    if(!jsonString || !meshMessage)
    {
        MESP_LOGE(MESH_TAG, "Invalid parameters for buildMeshMessage");
        return ESP_FAIL;
    }
    
    *meshMessage = (meshunion_t*)calloc(1, MESH_MESSAGE_BUFFER_SIZE);
    if(!*meshMessage)
    {
        MESP_LOGE(MESH_TAG, "Failed to allocate mesh message buffer");
        return ESP_FAIL;
    }
    
    (*meshMessage)->cmd = MESH_INT_DATA_CJSON;
    
    // Copy JSON payload after the command field
    void *payloadPtr = (uint8_t*)(*meshMessage) + MESH_UNION_CMD_SIZE;
    memcpy(payloadPtr, jsonString, strlen(jsonString));
    
    return ESP_OK;
}

/**
 * @brief Sends erase metrics command to mesh network
 * @param meshMessage The mesh union message to send
 * @param jsonStringLen Length of the JSON string in the message
 * @param meterIdNode The meter ID node for logging
 * @return ESP_OK on success, error code otherwise
 */
static esp_err_t sendEraseCommandToMesh(meshunion_t *meshMessage, size_t jsonStringLen, cJSON *meterIdNode)
{
    if(!meshMessage)
    {
        MESP_LOGE(MESH_TAG, "Mesh message is NULL");
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
        MESP_LOGE(MESH_TAG, "Failed to send erase metrics command via mesh: %s", esp_err_to_name(err));
    }
    else if(meterIdNode && meterIdNode->valuestring)
    {
        MESP_LOGI(MESH_TAG, "Erase metrics command sent successfully to node: %s", meterIdNode->valuestring);
    }
    
    return err;
}

/*
 * @brief Command handler to erase metrics on remote mesh nodes
 * @param argument Pointer to cJSON object containing the erase command
 * @return ESP_OK if command sent successfully, ESP_FAIL otherwise
 * 
 * This function processes erase metrics commands and transmits them via ESP-MESH
 * to the target node. It performs the following operations:
 * 1. Validates the command structure and required fields
 * 2. Serializes the command to JSON format
 * 3. Logs the operation for audit purposes
 * 4. Builds a mesh union message containing the command
 * 5. Broadcasts the message to the mesh network
 * 
 * @note The target node is identified by the 'mid' field in the command.
 *       The actual erase operation is performed by the receiving node.
 * 
 * Error Conditions:
 *   - Returns ESP_FAIL if command is NULL or missing 'mid' field
 *   - Returns ESP_FAIL if JSON serialization fails
 *   - Returns ESP_FAIL if mesh message allocation fails
 *   - Returns mesh send error code if transmission fails
 * 
 * Example:
 *   cJSON *cmd = cJSON_Parse("{\"cmd\":\"erase\",\"mid\":\"meter001\"}");
 *   esp_err_t result = cmdEraseMetrics(cmd);
 */
int cmdEraseMetrics(void *argument)
{
    cJSON *cmd = (cJSON *)argument;
    MESP_LOGI(MESH_TAG, "Erase CMD");
    
    /*
    Command format:
    {"cmd":"erase","f":"xxxxxx","mid":"thismeter"}
    */
    
    // Validate command has required fields
    cJSON *meterIdNode = validateEraseCommand(cmd);
    if(!meterIdNode)
    {
        return ESP_FAIL;
    }
    
    // Serialize command to JSON string
    char *jsonString = cJSON_PrintUnformatted(cmd);
    if(!jsonString)
    {
        MESP_LOGE(MESH_TAG, "Failed to serialize erase metrics command to JSON");
        return ESP_FAIL;
    }
    
    // Log the erase metrics operation
    logEraseOperation(meterIdNode);
    
    // Build mesh message
    meshunion_t *meshMessage = NULL;
    esp_err_t err = buildMeshMessageErase(jsonString, &meshMessage);
    if(err != ESP_OK)
    {
        free(jsonString);
        return err;
    }
    
    // Send command to mesh network
    if(!theConf.wifi_mode)    
        err = sendEraseCommandToMesh(meshMessage, strlen(jsonString), meterIdNode);
    
    // Cleanup
    free(jsonString);
    free(meshMessage);
    
    return err;
}
