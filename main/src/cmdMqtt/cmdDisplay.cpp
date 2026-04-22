/**
 * @file cmdDisplay.cpp
 * @brief Display command handler for mesh network communication
 * 
 * This module handles display control commands that are transmitted through
 * the ESP-MESH network to control display devices on remote nodes.
 * 
 * Command format:
 *   {"cmd":"display","f":"<flag>","mid":"<meter_id>","time":<timestamp>}
 * 
 * Where:
 *   - cmd: Command type (always "display")
 *   - f: Display flag ("o" = turn off, other values for specific display modes)
 *   - mid: Target meter/node ID
 *   - time: Timestamp for the command
 * 
 * Flow:
 *   1. Validate command structure (mid field required)
 *   2. Serialize JSON to string
 *   3. Build mesh union message with CJSON command type
 *   4. Send via ESP-MESH broadcast to group
 *   5. Target node performs additional validation and executes
 */

#define GLOBAL
#include "includes.h" 
#include "globals.h"

extern void writeLog(const char* que);

// Display command constants
#define MESH_MESSAGE_BUFFER_SIZE sizeof(meshunion_t)
#define MESH_UNION_CMD_SIZE 4

/**
 * @brief Validate display command has required fields
 * @param cmd JSON command object
 * @return ESP_OK if valid, ESP_FAIL otherwise
 */
int validateDisplayCommand(cJSON *cmd)
{
    if(!cmd) {
        MESP_LOGE(MESH_TAG, "Display command: NULL argument");
        return ESP_FAIL;
    }
    
    cJSON *meterIdNode = cJSON_GetObjectItem(cmd, "mid");
    if(!meterIdNode) {
        MESP_LOGE(MESH_TAG, "Display command: Missing 'mid' field");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

/**
 * @brief Build mesh message from JSON command
 * @param jsonString Serialized JSON string
 * @param outMessage Output pointer to receive allocated mesh message
 * @param outSize Output pointer to receive message size
 * @return ESP_OK on success, ESP_FAIL on allocation failure
 */
int buildMeshMessage(const char *jsonString, meshunion_t **outMessage, size_t *outSize)
{
    // Allocate buffer for mesh union message
    meshunion_t *meshMessage = (meshunion_t*)calloc(1, MESH_MESSAGE_BUFFER_SIZE);
    if(!meshMessage) {
        MESP_LOGE(MESH_TAG, "Failed to allocate mesh message buffer");
        return ESP_FAIL;
    }
    
    meshMessage->cmd = MESH_INT_DATA_CJSON;
    
    // Copy JSON payload after the command field
    void *payloadPtr = (uint8_t*)meshMessage + MESH_UNION_CMD_SIZE;
    memcpy(payloadPtr, jsonString, strlen(jsonString));
    
    *outMessage = meshMessage;
    *outSize = strlen(jsonString) + MESH_UNION_CMD_SIZE;
    
    return ESP_OK;
}

/**
 * @brief Send display command to mesh network
 * @param meshMessage Prepared mesh message
 * @param messageSize Size of the message
 * @return ESP_OK on success, ESP_FAIL on send failure
 */
int sendDisplayCommandToMesh(meshunion_t *meshMessage, size_t messageSize)
{
    mesh_data_t data;
    data.proto = MESH_PROTO_BIN;
    data.tos = MESH_TOS_P2P;
    data.data = (uint8_t*)meshMessage;
    data.size = messageSize;
    
    int err = esp_mesh_send(&GroupID, &data, MESH_DATA_P2P, NULL, MESH_OPT_SEND_GROUP);
    
    if(err != ESP_OK) {
        MESP_LOGE(MESH_TAG, "Failed to send mesh message: %d", err);
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

/**
 * @brief Display command handler for mesh network
 * 
 * Processes display control commands and transmits them to the appropriate
 * node via the ESP-MESH network. The command is validated, serialized,
 * packaged into a mesh union message, and broadcast to the mesh group.
 * 
 * @param argument Pointer to cJSON object containing the display command
 * 
 * @return ESP_OK on successful transmission
 * @return ESP_FAIL if validation fails, serialization fails, allocation fails,
 *         or mesh transmission fails
 * 
 * Command structure:
 *   Required fields:
 *     - "mid": Meter/node ID (target device identifier)
 *   Optional fields:
 *     - "f": Display flag/mode
 *     - "time": Command timestamp
 * 
 * Example usage:
 *   cJSON *cmd = cJSON_CreateObject();
 *   cJSON_AddStringToObject(cmd, "cmd", "display");
 *   cJSON_AddStringToObject(cmd, "f", "o");
 *   cJSON_AddStringToObject(cmd, "mid", "meter01");
 *   cJSON_AddNumberToObject(cmd, "time", 1234567890);
 *   int result = cmdDisplay(cmd);
 * 
 * Error conditions:
 *   - NULL argument
 *   - Missing 'mid' field
 *   - JSON serialization failure
 *   - Memory allocation failure
 *   - Mesh send failure
 * 
 * Notes:
 *   - The target node is responsible for additional command validation
 *   - Message is sent as P2P broadcast to the mesh group
 *   - All allocated resources are freed before return
 */
int cmdDisplay(void *argument)
{
    cJSON *cmd = (cJSON *)argument;
    MESP_LOGI(MESH_TAG, "Display CMD");
    
    /*
    Command format:
    {"cmd":"display","f":"xxxxxx","mid":"thismeter","time":99999}
    f: "o" -> turn off display
    */
    
    // Validate command structure
    if(validateDisplayCommand(cmd) != ESP_OK) {
        return ESP_FAIL;
    }
    
    // Serialize JSON command
    char *jsonString = cJSON_PrintUnformatted(cmd);
    if(!jsonString) {
        MESP_LOGE(MESH_TAG, "Failed to serialize JSON command");
        return ESP_FAIL;
    }
    
    // Build mesh message
    meshunion_t *meshMessage = NULL;
    size_t messageSize = 0;
    int result = buildMeshMessage(jsonString, &meshMessage, &messageSize);
    
    if(result != ESP_OK) {
        free(jsonString);
        return ESP_FAIL;
    }
    
    // Send to mesh network
    result = sendDisplayCommandToMesh(meshMessage, messageSize);
    
    // Cleanup allocated resources
    free(jsonString);
    free(meshMessage);
    
    return result;
}
