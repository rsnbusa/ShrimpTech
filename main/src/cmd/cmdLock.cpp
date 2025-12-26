/**
 * @file cmdLock.cpp
 * @brief Command handler for locking/unlocking remote ESP-MESH nodes
 * 
 * This module handles the "lock" command which instructs remote nodes in the mesh
 * network to lock or unlock their meters. The command supports batch operations,
 * allowing multiple nodes to be locked/unlocked in a single command. It also
 * supports optional OTA version updates.
 * 
 * Command Format:
 *   {"cmd":"lock","disCon":[{"mid":"meterid","state":0/1},{"mid":"othermid","state":0/1}...],"ver":"1.0.0"}
 * 
 * Where:
 *   - cmd: Command type (must be "lock")
 *   - disCon: Array of disconnect/lock objects (required)
 *   - mid: Meter ID of the target node (required per item)
 *   - state: Lock state - 0 (unlock) or 1 (lock) (required per item)
 *   - ver: Optional OTA firmware version string
 * 
 * Usage Example:
 *   cJSON *cmd = cJSON_Parse("{\"cmd\":\"lock\",\"disCon\":[{\"mid\":\"meter001\",\"state\":1}]}");
 *   int result = cmdLock(cmd);
 *   if(result == ESP_OK) {
 *       // Commands sent successfully
 *   }
 * 
 * Behavior:
 *   - Processes all items in the array even if individual items fail
 *   - Logs warnings for failed items but continues processing
 *   - Returns ESP_OK if command structure is valid, regardless of transmission results
 *   - Returns ESP_FAIL only if command structure is invalid
 * 
 * @note The actual lock/unlock operation is performed by the target node after
 *       receiving the command. This function validates and transmits commands.
 * 
 * @dependencies
 *   - ESP-MESH network must be initialized and connected
 *   - cJSON library for JSON parsing
 *   - writeLog() function for operation logging
 *   - save_ota_version() function for OTA version updates
 *   - internal_cmds[4] array containing the lock command string
 */

#define GLOBAL
#include "globals.h"

extern void writeLog(char *que);
extern void save_ota_version(char *version);

// Lock command constants
#define MESH_MESSAGE_BUFFER_SIZE 100
#define MESH_UNION_CMD_SIZE 4
#define LOG_BUFFER_SIZE 100

/**
 * @brief Validates lock command and returns the disconnect array
 * @param lockCommand The cJSON command object to validate
 * @return Pointer to the disconnect array if valid, NULL otherwise
 */
static cJSON* validateLockCommand(cJSON *lockCommand)
{
    if(!lockCommand)
    {
        ESP_LOGE(MESH_TAG, "NO CMD argument passed. Internal error");
        return NULL;
    }
    
    cJSON *disconnectArray = cJSON_GetObjectItem(lockCommand, "disCon");
    if(!disconnectArray)
    {
        ESP_LOGE(MESH_TAG, "Lock Cmd no disCon array");
        return NULL;
    }
    
    return disconnectArray;
}

/**
 * @brief Processes optional OTA version update from command
 * @param lockCommand The cJSON command object
 */
static void processOtaVersion(cJSON *lockCommand)
{
    if(!lockCommand)
        return;
    
    cJSON *versionNode = cJSON_GetObjectItem(lockCommand, "ver");
    if(versionNode && versionNode->valuestring && strlen(versionNode->valuestring) > 0)
    {
        save_ota_version(versionNode->valuestring);
    }
}

/**
 * @brief Creates a lock command JSON object
 * @param meterIdNode The meter ID node
 * @param lockStateNode The lock state node
 * @return Pointer to created cJSON object (caller must delete), or NULL on failure
 */
static cJSON* createLockMessage(cJSON *meterIdNode, cJSON *lockStateNode)
{
    if(!meterIdNode || !meterIdNode->valuestring || !lockStateNode)
    {
        ESP_LOGE(MESH_TAG, "Invalid meter ID or lock state");
        return NULL;
    }
    
    cJSON *root = cJSON_CreateObject();
    if(!root)
    {
        ESP_LOGE(MESH_TAG, "Error creating Root Lock");
        return NULL;
    }
    
    cJSON_AddStringToObject(root, "cmd", internal_cmds[4]);
    cJSON_AddStringToObject(root, "mid", meterIdNode->valuestring);
    cJSON_AddNumberToObject(root, "state", lockStateNode->valueint);
    
    return root;
}

/**
 * @brief Logs the lock operation with meter ID
 * @param meterIdNode The cJSON node containing the meter ID
 */
static void logLockOperation(cJSON *meterIdNode)
{
    if(!meterIdNode || !meterIdNode->valuestring)
    {
        ESP_LOGW(MESH_TAG, "Cannot log lock operation: invalid meter ID");
        return;
    }
    
    char *logBuffer = (char*)malloc(LOG_BUFFER_SIZE);
    if(logBuffer)
    {
        snprintf(logBuffer, LOG_BUFFER_SIZE, "Lock Mid %s", meterIdNode->valuestring);
        writeLog(logBuffer);
        free(logBuffer);
    }
    else
    {
        ESP_LOGW(MESH_TAG, "Failed to allocate log buffer for lock operation");
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
static esp_err_t buildMeshMessageLock(const char *jsonString, meshunion_t **meshMessage)
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
    
    void *payloadPtr = (void*)(*meshMessage) + MESH_UNION_CMD_SIZE;
    memcpy(payloadPtr, jsonString, strlen(jsonString));
    
    return ESP_OK;
}

/**
 * @brief Sends lock command to mesh network
 * @param meshMessage The mesh union message to send
 * @param jsonStringLen Length of the JSON string in the message
 * @param meterIdNode The meter ID node for logging
 * @return ESP_OK on success, error code otherwise
 */
static esp_err_t sendLockCommandToMesh(meshunion_t *meshMessage, size_t jsonStringLen, cJSON *meterIdNode)
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
        ESP_LOGE(MESH_TAG, "Failed to send lock command via mesh: %s", esp_err_to_name(err));
    }
    else if(meterIdNode && meterIdNode->valuestring)
    {
        ESP_LOGI(MESH_TAG, "Lock command sent successfully to node: %s", meterIdNode->valuestring);
    }
    
    return err;
}

/**
 * @brief Processes a single lock array item
 * @param arrayItem The cJSON array item to process
 * @return ESP_OK on success, ESP_FAIL on error
 * 
 * This function orchestrates the complete process for a single lock command:
 * 1. Validates the array item has required fields (mid, state)
 * 2. Creates the lock message JSON structure
 * 3. Serializes to JSON string
 * 4. Logs the operation
 * 5. Builds the mesh message
 * 6. Sends to mesh network
 * 7. Cleans up all allocated resources
 * 
 * @note All resources are cleaned up regardless of success or failure
 */
static esp_err_t processLockArrayItem(cJSON *arrayItem)
{
    if(!arrayItem)
    {
        ESP_LOGE(MESH_TAG, "Lock Cmd no Array error");
        return ESP_FAIL;
    }
    
    cJSON *meterIdNode = cJSON_GetObjectItem(arrayItem, "mid");
    cJSON *lockStateNode = cJSON_GetObjectItem(arrayItem, "state");
    
    if(!meterIdNode || !meterIdNode->valuestring || !lockStateNode)
    {
        ESP_LOGE(MESH_TAG, "Lock Cmd no MID or STATE");
        return ESP_FAIL;
    }
    
    // Create lock message JSON
    cJSON *root = createLockMessage(meterIdNode, lockStateNode);
    if(!root)
    {
        return ESP_FAIL;
    }
    
    // Serialize to JSON string
    char *jsonString = cJSON_PrintUnformatted(root);
    if(!jsonString)
    {
        ESP_LOGE(MESH_TAG, "Failed to serialize lock command to JSON");
        cJSON_Delete(root);
        return ESP_FAIL;
    }
    
    // Log the operation
    logLockOperation(meterIdNode);
    
    // Build mesh message
    meshunion_t *meshMessage = NULL;
    esp_err_t err = buildMeshMessageLock(jsonString, &meshMessage);
    if(err != ESP_OK)
    {
        free(jsonString);
        cJSON_Delete(root);
        return err;
    }
    
    // Send to mesh
    err = sendLockCommandToMesh(meshMessage, strlen(jsonString), meterIdNode);
    
    // Cleanup
    free(jsonString);
    free(meshMessage);
    cJSON_Delete(root);
    
    return err;
}

/**
 * @brief Command handler to lock/unlock multiple remote mesh nodes
 * @param argument Pointer to cJSON object containing the lock command
 * @return ESP_OK if command structure is valid, ESP_FAIL if invalid
 * 
 * This function processes lock commands and transmits them via ESP-MESH to
 * target nodes. It supports batch operations for multiple nodes and optional
 * OTA version updates.
 * 
 * Processing Flow:
 * 1. Validates the command structure and gets disconnect array
 * 2. Processes optional OTA version update if present
 * 3. Iterates through each item in the disconnect array
 * 4. Processes each lock command independently
 * 5. Continues processing remaining items even if individual items fail
 * 
 * Error Handling:
 *   - Returns ESP_FAIL immediately if command structure is invalid
 *   - Logs warnings for individual item failures but continues processing
 *   - Returns ESP_OK if at least the command structure was valid
 * 
 * Example Command:
 *   {
 *     "cmd": "lock",
 *     "disCon": [
 *       {"mid": "meter001", "state": 1},
 *       {"mid": "meter002", "state": 0}
 *     ],
 *     "ver": "1.2.3"
 *   }
 * 
 * @note Target nodes identified by 'mid' field perform the actual lock/unlock.
 *       This function only validates and broadcasts the commands.
 */
int cmdLock(void *argument)
{
    cJSON *lockCommand = (cJSON *)argument;
    
    /*
    Command format:
    {"cmd":"lock","disCon":[{"mid":"meterid","state":0/1},{"mid":"othermid","state":0/1}...]}
    */
    
    // Validate command and get disconnect array
    cJSON *disconnectArray = validateLockCommand(lockCommand);
    if(!disconnectArray)
    {
        return ESP_FAIL;
    }
    
    // Process optional OTA version update
    processOtaVersion(lockCommand);
    
    // Process each lock command in the array
    int arraySize = cJSON_GetArraySize(disconnectArray);
    for(int i = 0; i < arraySize; i++)
    {
        cJSON *arrayItem = cJSON_GetArrayItem(disconnectArray, i);
        esp_err_t err = processLockArrayItem(arrayItem);
        
        // Continue processing other items even if one fails
        if(err != ESP_OK)
        {
            ESP_LOGW(MESH_TAG, "Failed to process lock item %d, continuing with next", i);
        }
    }
    
    return ESP_OK;
}