#define GLOBAL
#include "includes.h"
#include "globals.h"

/*
 * Handles meter configuration update commands sent over mesh network. Forwards update
 * requests to specific meters identified by meter ID. Target node validates and applies
 * the configuration changes (billing packages, schedules, skip flags, etc.).
 */

extern void writeLog(const char* que);

// Update command constants
#define UPDATE_LOG_BUFFER_SIZE 100
#define UPDATE_MESSAGE_SIZE 400
#define MESH_CMD_OFFSET 4

/* Validates the update command JSON payload and extracts meter ID. */
static bool validate_update_command(cJSON *updateCommand, const char **outMeterId)
{
    if(!updateCommand || !outMeterId)
    {
        MESP_LOGE(MESH_TAG, "Update command validation arguments invalid");
        return false;
    }
    
    if(!cJSON_IsObject(updateCommand))
    {
        MESP_LOGE(MESH_TAG, "Update command payload invalid");
        return false;
    }
    
    cJSON *meterIdNode = cJSON_GetObjectItem(updateCommand, "mid");
    if(!meterIdNode)
    {
        MESP_LOGE(MESH_TAG, "Update command missing meter ID field");
        return false;
    }
    
    if(!cJSON_IsString(meterIdNode) || !meterIdNode->valuestring || strlen(meterIdNode->valuestring) == 0)
    {
        MESP_LOGE(MESH_TAG, "Update meter ID parameter invalid or empty");
        return false;
    }
    
    *outMeterId = meterIdNode->valuestring;
    return true;
}

/* Logs the update request for audit trail. */
static void log_update_request(const char *meterId)
{
    char *logBuffer = (char*)calloc(UPDATE_LOG_BUFFER_SIZE, 1);
    if(logBuffer)
    {
        snprintf(logBuffer, UPDATE_LOG_BUFFER_SIZE, "Update Mid %s", meterId ? meterId : "");
        writeLog(logBuffer);
        free(logBuffer);
    }
}

/* Logs mesh send failures for debugging. */
static void log_update_error(int errorCode)
{
    char *logBuffer = (char*)calloc(UPDATE_LOG_BUFFER_SIZE, 1);
    if(logBuffer)
    {
        snprintf(logBuffer, UPDATE_LOG_BUFFER_SIZE, "UpdateCmd not executed error %d", errorCode);
        writeLog(logBuffer);
        free(logBuffer);
    }
}

/* Prepares and sends the update command via mesh broadcast. */
static int send_update_mesh_message(cJSON *updateCommand)
{
    char *jsonMessage = cJSON_PrintUnformatted(updateCommand);
    if(!jsonMessage)
    {
        MESP_LOGE(MESH_TAG, "Failed to serialize update command to JSON");
        return ESP_FAIL;
    }
    
    size_t messageLength = strlen(jsonMessage);
    size_t totalSize = messageLength + MESH_CMD_OFFSET;
    
    meshunion_t *meshMessage = (meshunion_t*)calloc(1, UPDATE_MESSAGE_SIZE);
    if(!meshMessage)
    {
        MESP_LOGE(MESH_TAG, "Failed to allocate mesh message buffer");
        free(jsonMessage);
        return ESP_FAIL;
    }
    
    meshMessage->cmd = MESH_INT_DATA_CJSON;
    void *payloadPtr = (uint8_t*)meshMessage + MESH_CMD_OFFSET;
    memcpy(payloadPtr, jsonMessage, messageLength);
    
    mesh_data_t data;
    data.proto = MESH_PROTO_BIN;
    data.tos = MESH_TOS_P2P;
    data.data = (uint8_t*)meshMessage;
    data.size = totalSize;
    
    int err = esp_mesh_send(&GroupID, &data, MESH_DATA_P2P, NULL, MESH_OPT_SEND_GROUP);
    
    free(jsonMessage);
    free(meshMessage);
    
    if(err != ESP_OK)
    {
        MESP_LOGE(MESH_TAG, "Failed to send update command via mesh: %d", err);
        log_update_error(err);
        return ESP_FAIL;
    }
    
    MESP_LOGI(MESH_TAG, "Update command sent successfully");
    return ESP_OK;
}

int cmdUpdate(void *argument)
{
    /*
     * Expected JSON format:
     * {"cmd":"update","f":"xxxxxx","mid":"thismeter","bpk":12234,"ks":1234,"k":12313,"sk":"y/n","skn":9,"cha":"xxxxxxxx"}
     */
    
    MESP_LOGI(MESH_TAG, "Update CMD");
    
    cJSON *updateCommand = (cJSON *)argument;
    
    if(updateCommand == NULL)
    {
        MESP_LOGE(MESH_TAG, "Update command argument is NULL");
        return ESP_FAIL;
    }
    
    const char *meterId = NULL;
    if(!validate_update_command(updateCommand, &meterId))
        return ESP_FAIL;
    
    log_update_request(meterId);
    
    return send_update_mesh_message(updateCommand);
}