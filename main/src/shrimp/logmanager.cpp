/**
 * @file logmanager.cpp
 * @brief SPIFFS-based logging system for persistent log storage
 * 
 * This module provides persistent logging functionality using ESP32's SPIFFS
 * filesystem. Logs are written with timestamps and automatically recycled when
 * the file reaches capacity.
 * 
 * Features:
 * - Timestamped log entries
 * - Automatic file recycling when full
 * - SPIFFS partition management
 * - Append-only logging
 * 
 * Usage:
 *   logFileInit();                    // Initialize once at startup
 *   writeLog("System started");       // Write log entries
 */

#define GLOBAL
#include "globals.h"
#include "forwards.h"

// ===================================================================
// Log Manager Constants
// ===================================================================

#define LOG_BUFFER_SIZE         500
#define LOG_MOUNT_POINT         "/spiffs"
#define LOG_FILE_PATH           "/spiffs/log.txt"
#define LOG_MAX_FILES           10
#define LOG_MAX_FILE_SIZE       65536  // 64KB max before recycling
#define TIMESTAMP_FORMAT        "[%d-%02d-%02d %02d:%02d:%02d] %s\n"

// ===================================================================
// Helper Functions
// ===================================================================

/**
 * @brief Creates a formatted timestamp string with message
 * @param buffer Buffer to store the formatted string
 * @param bufferSize Size of the buffer
 * @param message Message to append to timestamp
 * @return true if successful, false if buffer too small
 */
static bool formatLogMessage(char *buffer, size_t bufferSize, const char *message)
{
    if(!buffer || !message)
    {
        return false;
    }

    time_t currentTime = time(NULL);
    struct tm timeInfo = *localtime(&currentTime);
    
    int written = snprintf(buffer, bufferSize, TIMESTAMP_FORMAT,
                          timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
                          timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec, message);
    
    return (written > 0 && written < (int)bufferSize);
}

/**
 * @brief Recycles the log file by deleting and recreating it
 * @return true if successful, false on failure
 */
static bool recycleLogFile()
{
    MESP_LOGW("LOG", "Log file full, recycling...");
    
    if(myFile)
    {
        fclose(myFile);
        myFile = NULL;
    }
    
    remove(LOG_FILE_PATH);
    
    myFile = fopen(LOG_FILE_PATH, "a");
    if(!myFile)
    {
        MESP_LOGE("LOG", "Failed to open file after recycling");
        return false;
    }
    
    // Write recycling notice
    char logBuffer[LOG_BUFFER_SIZE];
    if(formatLogMessage(logBuffer, LOG_BUFFER_SIZE, "File was recycled"))
    {
        fprintf(myFile, logBuffer);
        fflush(myFile);
    }
    
    return true;
}

// ===================================================================
// Log Writing Functions
// ===================================================================

/**
 * @brief Writes a timestamped log entry to the SPIFFS log file
 * 
 * This function appends a log message with current timestamp to the persistent
 * log file. If the file is full (write fails), it automatically recycles the
 * file by deleting and recreating it.
 * 
 * @param message The log message to write (must not be NULL)
 * 
 * @note This function uses the global myFile handle which must be initialized
 *       by calling logFileInit() first
 * @warning Not thread-safe - caller must ensure synchronization
 */
void writeLog(char *message)
{
    if(!message)
    {
        MESP_LOGW("LOG", "Attempted to write NULL message");
        return;
    }

    if(!myFile)
    {
        MESP_LOGE("LOG", "Log file not initialized - call logFileInit() first");
        return;
    }
    
    char *logBuffer = (char*)calloc(LOG_BUFFER_SIZE, 1);
    if(!logBuffer)
    {
        MESP_LOGE("LOG", "Failed to allocate log buffer");
        return;
    }

    // Format message with timestamp
    if(!formatLogMessage(logBuffer, LOG_BUFFER_SIZE, message))
    {
        MESP_LOGE("LOG", "Failed to format log message");
        free(logBuffer);
        return;
    }
    
    size_t messageLength = strlen(logBuffer);
    int bytesWritten = fprintf(myFile, logBuffer);
    
    // Check if write failed due to full filesystem - recycle log file
    if(bytesWritten < (int)messageLength)
    {
        if(!recycleLogFile())
        {
            free(logBuffer);
            return;
        }
        
        // Retry writing original message after recycling
        fprintf(myFile, logBuffer);
    }
    
    fflush(myFile);
    free(logBuffer);
}

// ===================================================================
// Log Initialization
// ===================================================================

/**
 * @brief Initializes the SPIFFS filesystem and opens the log file
 * 
 * This function must be called once during system startup before any calls
 * to writeLog(). It performs the following:
 * - Registers and mounts SPIFFS filesystem
 * - Formats partition if mount fails
 * - Opens log file in append mode
 * - Reports partition usage statistics
 * 
 * @note Sets global myFile handle for use by writeLog()
 * @note Automatically formats SPIFFS if partition is corrupt or not found
 */
void logFileInit()
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = LOG_MOUNT_POINT,
        .partition_label = "profile",
        .max_files = LOG_MAX_FILES,
        .format_if_mount_failed = true
    };

    esp_err_t result = esp_vfs_spiffs_register(&conf);

    if(result != ESP_OK)
    {
        if(result == ESP_FAIL)
        {
            MESP_LOGE("LOG", "Failed to mount or format filesystem");
        }
        else if(result == ESP_ERR_NOT_FOUND)
        {
            MESP_LOGE("LOG", "Failed to find SPIFFS partition");
        }
        else
        {
            MESP_LOGE("LOG", "Failed to initialize SPIFFS: %s", esp_err_to_name(result));
        }
        return;
    }

    // Get partition information
    size_t totalBytes = 0, usedBytes = 0;
    result = esp_spiffs_info(conf.partition_label, &totalBytes, &usedBytes);
    
    if(result != ESP_OK)
    {
        MESP_LOGW("LOG", "Failed to get SPIFFS partition info: %s. Formatting...", 
                 esp_err_to_name(result));
        esp_spiffs_format(conf.partition_label);
        // Continue to open file after formatting
    }
    else
    {
        // Calculate and display usage percentage only if info was retrieved successfully
        int usagePercent = (totalBytes > 0) ? (usedBytes * 100 / totalBytes) : 0;
        MESP_LOGI("LOG", "SPIFFS - Total: %d bytes, Used: %d bytes (%d%%)", 
                 totalBytes, usedBytes, usagePercent);
    }
    
    // Open log file in append mode
    myFile = fopen(LOG_FILE_PATH, "a");
    if(!myFile)
    {
        MESP_LOGE("LOG", "Failed to open log file: %s", LOG_FILE_PATH);
        return;
    }
    
    MESP_LOGI("LOG", "Log file opened successfully: %s", LOG_FILE_PATH);
}