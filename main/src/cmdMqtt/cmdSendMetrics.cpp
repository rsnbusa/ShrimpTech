#define GLOBAL
#include "includes.h" 
#include "globals.h"

/*
 * Handles commands to request metrics data from specific meters in the mesh network.
 * Forwards the request as a mesh group broadcast to reach the target node, which will
 * validate and respond with its metrics data.
 */

extern void writeLog(char* que);

// SendMetrics command constants
#define METRICS_LOG_BUFFER_SIZE 100
#define METRICS_MESSAGE_SIZE 100
#define MESH_CMD_OFFSET 4

/* Validates the metrics command JSON payload and extracts meter ID. */
static bool validate_metrics_command(cJSON *metricsCommand, const char **outMeterId)
{
    if(!metricsCommand || !outMeterId)
    {
        ESP_LOGE(MESH_TAG, "Metrics command validation arguments invalid");
        return false;
    }
    
    if(!cJSON_IsObject(metricsCommand))
    {
        ESP_LOGE(MESH_TAG, "Metrics command payload invalid");
        return false;
    }
    
    cJSON *meterIdNode = cJSON_GetObjectItem(metricsCommand, "mid");
    if(!meterIdNode)
    {
        ESP_LOGE(MESH_TAG, "Metrics command missing meter ID field");
        return false;
    }
    
    if(!cJSON_IsString(meterIdNode) || !meterIdNode->valuestring || strlen(meterIdNode->valuestring) == 0)
    {
        ESP_LOGE(MESH_TAG, "Metrics meter ID parameter invalid or empty");
        return false;
    }
    
    *outMeterId = meterIdNode->valuestring;
    return true;
}

/* Logs the metrics request for audit trail. */
static void log_metrics_request(const char *meterId)
{
    char *logBuffer = (char*)calloc(METRICS_LOG_BUFFER_SIZE, 1);
    if(logBuffer)
    {
        snprintf(logBuffer, METRICS_LOG_BUFFER_SIZE, "SendMetrics Mid %s", meterId ? meterId : "");
        writeLog(logBuffer);
        free(logBuffer);
    }
}

/* Prepares and sends the metrics request via mesh broadcast. */
static int send_metrics_mesh_message(cJSON *metricsCommand)
{
    char *jsonMessage = cJSON_PrintUnformatted(metricsCommand);
    if(!jsonMessage)
    {
        ESP_LOGE(MESH_TAG, "Failed to serialize metrics command to JSON");
        return ESP_FAIL;
    }
    
    size_t messageLength = strlen(jsonMessage);
    size_t totalSize = messageLength + MESH_CMD_OFFSET;
    
    meshunion_t *meshMessage = (meshunion_t*)calloc(1, METRICS_MESSAGE_SIZE);
    if(!meshMessage)
    {
        ESP_LOGE(MESH_TAG, "Failed to allocate mesh message buffer");
        free(jsonMessage);
        return ESP_FAIL;
    }
    
    meshMessage->cmd = MESH_INT_DATA_CJSON;
    void *payloadPtr = (void*)meshMessage + MESH_CMD_OFFSET;
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
        ESP_LOGE(MESH_TAG, "Failed to send metrics request via mesh: %d", err);
        return ESP_FAIL;
    }
    
    ESP_LOGI(MESH_TAG, "Metrics request sent successfully");
    return ESP_OK;
}

int cmdSendMetrics(void *argument)
{
    /*
     * Expected JSON format:
     * {"cmd":"mqttmetrics","f":"xxxxxx","mid":"thismeter"}
     */
    
    ESP_LOGI(MESH_TAG, "Metrics CMD");
    
    cJSON *metricsCommand = (cJSON *)argument;
    
    if(metricsCommand == NULL)
    {
        ESP_LOGE(MESH_TAG, "Metrics command argument is NULL");
        return ESP_FAIL;
    }
    
    const char *meterId = NULL;
    if(!validate_metrics_command(metricsCommand, &meterId))
        return ESP_FAIL;
    
    log_metrics_request(meterId);
    
    return send_metrics_mesh_message(metricsCommand);
}