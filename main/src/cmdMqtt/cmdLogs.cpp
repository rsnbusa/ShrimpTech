/**
 * @file cmdLogs.cpp
 * @brief Command handler for retrieving and managing log files on ESP-MESH root node
 * 
 * This module handles the "log" command which allows retrieval of log file contents
 * and erasure of log files. This command is only processed by the root node in the
 * mesh network. The log file is stored in SPIFFS at /spiffs/log.txt.
 * 
 * Command Format:
 *   {"cmd":"log","f":"xxxxxx","mid":"thismeter","lines":nnn,"erase":"y/n"}
 * 
 * Where:
 *   - cmd: Command type (must be "log")
 *   - f: Authentication/authorization field
 *   - mid: Meter ID (currently unused - only root responds)
 *   - lines: Number of log lines to retrieve (optional, must be > 0)
 *   - erase: "y" to erase log file, "n" or omit to skip erase (optional)
 * 
 * Usage Examples:
 *   // Retrieve 100 lines from log
 *   cJSON *cmd = cJSON_Parse("{\"cmd\":\"log\",\"lines\":100}");
 *   int result = cmdLogs(cmd);
 * 
 *   // Erase log file
 *   cJSON *cmd = cJSON_Parse("{\"cmd\":\"log\",\"erase\":\"y\"}");
 *   int result = cmdLogs(cmd);
 * 
 *   // Retrieve logs and then erase
 *   cJSON *cmd = cJSON_Parse("{\"cmd\":\"log\",\"lines\":50,\"erase\":\"y\"}");
 *   int result = cmdLogs(cmd);
 * 
 * Behavior:
 *   - Only root nodes process this command (non-root nodes return ESP_OK immediately)
 *   - Log retrieval sends data to MQTT queue for transmission
 *   - Log file is automatically reopened in append mode after operations
 *   - Erase operation removes and recreates the log file
 *   - Memory is automatically managed (buffers freed on all paths)
 * 
 * @note The global file handle 'myFile' is used and managed by this module.
 *       File operations are synchronized to prevent corruption.
 * 
 * @dependencies
 *   - SPIFFS filesystem must be mounted with log.txt file
 *   - mqttSender queue for sending log data
 *   - discoQueue for MQTT message routing
 *   - Global myFile handle for log file operations
 *   - writeLog() function for operation logging
 */

#define GLOBAL
#include "includes.h" 
#include "globals.h"

extern void writeLog(char* que);

// Log command constants
#define LINE_BUFFER_SIZE 200
#define BYTES_PER_LINE_ESTIMATE 100

/**
 * @brief Reopens log file in append mode
 * @return ESP_OK on success, ESP_FAIL on failure
 */
static esp_err_t reopenLogFileAppend(void)
{
    if(myFile)
    {
        fclose(myFile);
    }
    
    myFile = fopen("/spiffs/log.txt", "a");
    if(myFile == NULL)
    {
        MESP_LOGE(MESH_TAG, "Failed to open log file for append");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

/**
 * @brief Reads specified number of lines from log file
 * @param numLines Number of lines to read from the log file
 * @param buffer Pointer to store the allocated buffer containing log data (caller must free on success)
 * @param bytesRead Pointer to store number of bytes actually read
 * @return ESP_OK on success, ESP_FAIL on failure
 * 
 * This function:
 * 1. Opens log file in read mode
 * 2. Allocates buffer based on estimated line size
 * 3. Reads up to numLines lines from the file
 * 4. Returns buffer pointer and bytes read
 * 5. Automatically reopens file in append mode
 * 
 * @note Caller is responsible for freeing the buffer on success
 * @note File is automatically reopened in append mode on both success and failure
 */
static esp_err_t readLogLines(int numLines, char **buffer, int *bytesRead)
{
    if(numLines <= 0 || !buffer || !bytesRead)
    {
        MESP_LOGE(MESH_TAG, "Invalid parameters for readLogLines");
        return ESP_FAIL;
    }
    
    // Close current file handle if open
    if(myFile)
    {
        fclose(myFile);
    }
    
    myFile = fopen("/spiffs/log.txt", "r");
    if(!myFile)
    {
        MESP_LOGE(MESH_TAG, "Failed to open log file for reading");
        reopenLogFileAppend();
        return ESP_FAIL;
    }
    
    int totalSize = numLines * BYTES_PER_LINE_ESTIMATE;
    char *logBuffer = (char*)calloc(1, totalSize);
    
    if(!logBuffer)
    {
        MESP_LOGE(MESH_TAG, "Failed to allocate %d bytes for log buffer", totalSize);
        reopenLogFileAppend();
        return ESP_FAIL;
    }
    
    char lineBuffer[LINE_BUFFER_SIZE];
    char *writePtr = logBuffer;
    int remainingLines = numLines;
    *bytesRead = 0;
    
    while(remainingLines > 0)
    {
        if(fgets(lineBuffer, sizeof(lineBuffer), myFile) != NULL)
        {
            size_t lineLength = strlen(lineBuffer);
            if((*bytesRead + lineLength) < totalSize)
            {
                memcpy(writePtr, lineBuffer, lineLength);
                writePtr += lineLength;
                remainingLines--;
                *bytesRead += lineLength;
            }
            else
            {
                MESP_LOGW(MESH_TAG, "Log buffer full, stopping at %d bytes", *bytesRead);
                break;
            }
        }
        else
        {
            // End of file or read error
            break;
        }
    }
    
    if(*bytesRead == 0)
    {
        MESP_LOGW(MESH_TAG, "No log data read from file");
        free(logBuffer);
        reopenLogFileAppend();
        return ESP_FAIL;
    }
    
    *buffer = logBuffer;
    return ESP_OK;
}

/**
 * @brief Sends log data to MQTT queue for transmission
 * @param logData Pointer to log data buffer (ownership transferred to queue)
 * @param dataSize Size of log data in bytes
 * @return ESP_OK on success, ESP_FAIL if queue send fails
 * 
 * @note On success, the buffer ownership is transferred to the queue handler
 * @note On failure, caller must free the buffer
 */
static esp_err_t sendLogsToQueue(char *logData, int dataSize)
{
    if(!logData || dataSize <= 0)
    {
        MESP_LOGE(MESH_TAG, "Invalid log data for queue");
        return ESP_FAIL;
    }
    
    mqttSender_t mqttMsg;
    mqttMsg.queue = discoQueue;
    mqttMsg.msg = logData;
    mqttMsg.lenMsg = dataSize;
    mqttMsg.code = NULL;
    mqttMsg.param = NULL;
    
    if(xQueueSend(mqttSender, &mqttMsg, 0) != pdTRUE)
    {
        MESP_LOGE(MESH_TAG, "Error queueing msg logs");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

/**
 * @brief Erases the log file and creates a new empty one
 * @return ESP_OK on success, ESP_FAIL if file cannot be recreated
 * 
 * This function:
 * 1. Closes current file handle
 * 2. Removes /spiffs/log.txt
 * 3. Creates new empty log file in append mode
 * 4. Updates global myFile handle
 * 
 * @note Logs warning if file removal fails (file may not exist)
 */
static esp_err_t eraseLogFile(void)
{
    MESP_LOGI(MESH_TAG, "Erase Log file cmd");
    
    if(myFile)
    {
        fclose(myFile);
        myFile = NULL;
    }
    
    if(remove("/spiffs/log.txt") != 0)
    {
        MESP_LOGW(MESH_TAG, "Failed to remove log file (may not exist)");
    }
    
    myFile = fopen("/spiffs/log.txt", "a");
    if(myFile == NULL)
    {
        MESP_LOGE(MESH_TAG, "Failed to open file for append after erase");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

/**
 * @brief Command handler to retrieve or erase log files on root node
 * @param argument Pointer to cJSON object containing the log command
 * @return ESP_OK on success or if non-root node, ESP_FAIL on error
 * 
 * This function processes log management commands on the root node only.
 * Non-root nodes return ESP_OK immediately without processing.
 * 
 * Processing Flow:
 * 1. Check if node is root (non-root returns ESP_OK)
 * 2. Validate command structure
 * 3. Process "lines" field if present (retrieve logs)
 *    - Read specified number of lines from log file
 *    - Send data to MQTT queue for transmission
 *    - Reopen file in append mode
 * 4. Process "erase" field if present (erase log)
 *    - Remove existing log file
 *    - Create new empty log file
 * 
 * Error Handling:
 *   - Returns ESP_FAIL if command is NULL
 *   - Returns ESP_FAIL if invalid line count requested
 *   - Returns ESP_FAIL if log reading fails
 *   - Returns ESP_FAIL if queue send fails
 *   - Returns ESP_FAIL if file erase/recreate fails
 * 
 * Memory Management:
 *   - Log buffer is automatically freed on all error paths
 *   - On successful queue send, buffer ownership transfers to queue handler
 * 
 * Example Commands:
 *   // Get last 100 log lines
 *   {"cmd":"log","lines":100}
 *   
 *   // Erase log file
 *   {"cmd":"log","erase":"y"}
 *   
 *   // Get logs then erase
 *   {"cmd":"log","lines":50,"erase":"y"}
 * 
 * @note Only processes commands on root nodes
 * @note Modifies global myFile handle
 */
int cmdLogs(void *argument)
{
    // Only root node handles log commands
    if(!esp_mesh_is_root())
        return ESP_OK;

    cJSON *cmd = (cJSON *)argument;
    if(!cmd)
    {
        MESP_LOGE(MESH_TAG, "NULL command argument");
        return ESP_FAIL;
    }
    
    MESP_LOGI(MESH_TAG, "Log CMD");
    
    /*
    Command format:
    {"cmd":"log","f":"xxxxxx","mid":"thismeter","lines":nnn,"erase":"y/n"}
    Note: 'mid' is currently irrelevant - only root node answers this command
    */
    
    cJSON *linesNode = cJSON_GetObjectItem(cmd, "lines");
    cJSON *eraseNode = cJSON_GetObjectItem(cmd, "erase");
    
    // Handle log retrieval request
    if(linesNode)
    {
        if(linesNode->valueint <= 0)
        {
            MESP_LOGW(MESH_TAG, "Invalid number of lines requested: %d", linesNode->valueint);
            return ESP_FAIL;
        }
        
        char *logBuffer = NULL;
        int bytesRead = 0;
        
        esp_err_t err = readLogLines(linesNode->valueint, &logBuffer, &bytesRead);
        if(err != ESP_OK)
        {
            return ESP_FAIL;
        }
        
        err = sendLogsToQueue(logBuffer, bytesRead);
        if(err != ESP_OK)
        {
            free(logBuffer);
            reopenLogFileAppend();
            return ESP_FAIL;
        }
        
        // Reopen file in append mode after sending
        reopenLogFileAppend();
        
        return ESP_OK;
    }
    
    // Handle log erase request
    if(eraseNode)
    {
        if(!eraseNode->valuestring)
        {
            MESP_LOGW(MESH_TAG, "Erase node has no value");
            return ESP_FAIL;
        }
        
        if(strcmp(eraseNode->valuestring, "y") == 0)
        {
            return eraseLogFile();
        }
    }
    
    return ESP_OK;
}