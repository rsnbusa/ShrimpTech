#include "includes.h"
#include "typedef.h"
#include "driver/i2c_types.h"
#include "FramI2C.h"

/*
 * FramI2C Implementation - Fujitsu MB85RC16 FRAM Driver
 * 
 * Provides I2C communication with 16Kb FRAM memory chip using ESP-IDF's
 * new master driver API. Supports random read/write operations with
 * 11-bit addressing scheme for the full 2K word address space.
 */

FramI2C::FramI2C(void)
{
    _framInitialised = false;
    intframWords = 0;
    addressBytes = 0;
    prodId = 0;
    manufID = 0;
    density = 0;
    idev_handle = NULL;
}


bool FramI2C::begin(int sdaPin, int sclPin, SemaphoreHandle_t *framSemaphore)
{
    if(!framSemaphore)
    {
        MESP_LOGE("FRAM", "Invalid semaphore pointer");
        return false;
    }

    // Configure I2C master bus
    i2c_master_bus_config_t busConfig = {};
    busConfig.i2c_port = I2C_BUS_NUMBER;
    busConfig.sda_io_num = (gpio_num_t)sdaPin;
    busConfig.scl_io_num = (gpio_num_t)sclPin;
    busConfig.clk_source = I2C_CLK_SRC_DEFAULT;
    busConfig.glitch_ignore_cnt = FRAM_I2C_GLITCH_FILTER_COUNT;
    busConfig.flags.enable_internal_pullup = true;
    busConfig.flags.allow_pd = false;

    i2c_master_bus_handle_t busHandle;
    esp_err_t ret = i2c_new_master_bus(&busConfig, &busHandle);
    if(ret != ESP_OK)
    {
        MESP_LOGE("FRAM", "Failed to create I2C master bus: %d", ret);
        return false;
    }

    // Configure I2C device
    i2c_device_config_t deviceConfig = {};
    deviceConfig.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    deviceConfig.device_address = I2C_DEVICE_ADDRESS_NOT_USED;
    deviceConfig.scl_speed_hz = FRAM_I2C_CLOCK_SPEED_HZ;

    i2c_master_dev_handle_t deviceHandle;
    ret = i2c_master_bus_add_device(busHandle, &deviceConfig, &deviceHandle);
    if(ret != ESP_OK)
    {
        MESP_LOGE("FRAM", "Failed to add I2C device: %d", ret);
        return false;
    }

    idev_handle = deviceHandle;
    intframWords = FRAM_16K_SIZE_WORDS;

    // Create semaphore for FRAM access synchronization
    *framSemaphore = xSemaphoreCreateBinary();
    if(*framSemaphore)
    {
        xSemaphoreGive(*framSemaphore);  // Initialize as available
        MESP_LOGI("FRAM", "Initialized successfully: %d words", intframWords);
    }
    else
    {
        MESP_LOGE("FRAM", "Failed to allocate semaphore");
        return false;
    }

    _framInitialised = true;
    return true;
}


int FramI2C::i2c_master_read_slave(uint16_t framAddress, uint8_t *readBuffer, size_t readSize)
{
    if(!readBuffer)
    {
        MESP_LOGE("FRAM", "Invalid read buffer pointer");
        return ESP_ERR_INVALID_ARG;
    }

    if(readSize == 0)
        return ESP_OK;

    // Split 11-bit address: low 8 bits and high 3 bits
    uint8_t addressLow = framAddress & FRAM_ADDRESS_LOW_MASK;
    uint8_t addressHigh = (framAddress & FRAM_ADDRESS_HIGH_MASK) >> FRAM_ADDRESS_HIGH_SHIFT;

    // Construct I2C addresses with high bits encoded in device address
    uint8_t i2cWriteAddress = (FRAM_I2C_SLAVE_ADDRESS + addressHigh) << 1 | I2C_WRITE_BIT;
    uint8_t i2cReadAddress = (FRAM_I2C_SLAVE_ADDRESS + addressHigh) << 1 | I2C_READ_BIT;
    uint16_t addressPacket = addressLow * FRAM_ADDRESS_BYTE_MULTIPLIER + i2cWriteAddress;

    // MB85RC16 random read sequence: START, write address, RESTART, read data, STOP
    i2c_operation_job_t operations[7];
    memset(operations, 0, sizeof(operations));
    operations[0].command = I2C_MASTER_CMD_START;
    operations[1].command = I2C_MASTER_CMD_WRITE;
    operations[1].write.ack_check = true;
    operations[1].write.data = (uint8_t*)&addressPacket;
    operations[1].write.total_bytes = 2;
    operations[2].command = I2C_MASTER_CMD_START;
    operations[3].command = I2C_MASTER_CMD_WRITE;
    operations[3].write.ack_check = true;
    operations[3].write.data = (uint8_t*)&i2cReadAddress;
    operations[3].write.total_bytes = 1;
    operations[4].command = I2C_MASTER_CMD_READ;
    operations[4].read.ack_value = I2C_ACK_VALUE;
    operations[4].read.data = readBuffer;
    operations[4].read.total_bytes = readSize - 1;
    operations[5].command = I2C_MASTER_CMD_READ;
    operations[5].read.ack_value = I2C_NACK_VALUE;
    operations[5].read.data = readBuffer + readSize - 1;
    operations[5].read.total_bytes = 1;
    operations[6].command = I2C_MASTER_CMD_STOP;

    esp_err_t result = i2c_master_execute_defined_operations(
        idev_handle, 
        operations, 
        sizeof(operations) / sizeof(i2c_operation_job_t), 
        -1
    );

    if(result != ESP_OK)
    {
        MESP_LOGE("FRAM", "Read failed at address 0x%03X: %d", framAddress, result);
    }

    return result;
}

esp_err_t FramI2C::i2c_master_write_slave(uint16_t framAddress, uint8_t *writeData, size_t writeSize)
{
    if(!writeData)
    {
        MESP_LOGE("FRAM", "Invalid write buffer pointer");
        return ESP_ERR_INVALID_ARG;
    }

    if(writeSize == 0)
        return ESP_OK;

    // Split 11-bit address: low 8 bits and high 3 bits
    uint8_t addressLow = framAddress & FRAM_ADDRESS_LOW_MASK;
    uint8_t addressHigh = (framAddress & FRAM_ADDRESS_HIGH_MASK) >> FRAM_ADDRESS_HIGH_SHIFT;

    // Construct packet: I2C address + FRAM address + data
    uint8_t* packetBuffer = (uint8_t*)malloc(writeSize + 2);
    if(!packetBuffer)
    {
        MESP_LOGE("FRAM", "Failed to allocate %d byte write buffer", writeSize + 2);
        return ESP_ERR_NO_MEM;
    }

    uint8_t i2cWriteAddress = (FRAM_I2C_SLAVE_ADDRESS + addressHigh) << 1 | I2C_WRITE_BIT;
    packetBuffer[0] = i2cWriteAddress;
    packetBuffer[1] = addressLow;
    memcpy(&packetBuffer[2], writeData, writeSize);

    // MB85RC16 byte write sequence: START, write address + data, STOP
    i2c_operation_job_t operations[3];
    memset(operations, 0, sizeof(operations));
    operations[0].command = I2C_MASTER_CMD_START;
    operations[1].command = I2C_MASTER_CMD_WRITE;
    operations[1].write.ack_check = true;
    operations[1].write.data = packetBuffer;
    operations[1].write.total_bytes = writeSize + 2;
    operations[2].command = I2C_MASTER_CMD_STOP;

    esp_err_t result = i2c_master_execute_defined_operations(
        idev_handle, 
        operations, 
        sizeof(operations) / sizeof(i2c_operation_job_t), 
        -1
    );

    free(packetBuffer);

    if(result != ESP_OK)
    {
        MESP_LOGE("FRAM", "Write failed at address 0x%03X: %d", framAddress, result);
    }

    return result;
}

int FramI2C::writeMany(uint16_t framAddress, uint8_t *data, uint16_t count)
{
    return i2c_master_write_slave(framAddress, data, count);
}

int FramI2C::readMany(uint16_t framAddress, uint8_t *buffer, uint16_t count)
{
    return i2c_master_read_slave(framAddress, buffer, count);
}

/**
 * @brief Erases FRAM memory by writing zeros to all addresses
 * 
 * @param fillPattern Optional pattern buffer to write instead of zeros
 * @param bufferSize Size of the fill buffer
 * @param eraseAll Currently unused parameter (kept for API compatibility)
 * @return ESP_OK on success, error code otherwise
 */
int FramI2C::format(uint8_t *fillPattern, uint32_t bufferSize, bool eraseAll)
{
    uint16_t remainingWords = intframWords;
    uint16_t currentAddress = 0;
    uint16_t writeLength = (bufferSize > remainingWords) ? remainingWords : bufferSize;

    MESP_LOGI("FRAM", "Starting format operation for %d bytes", intframWords);

    // Allocate write buffer
    uint8_t *writeBuffer = (uint8_t*)malloc(writeLength);
    if(!writeBuffer)
    {
        MESP_LOGE("FRAM", "Failed to allocate %d byte format buffer", writeLength);
        return ESP_ERR_NO_MEM;
    }

    // Initialize buffer with pattern or zeros
    if(fillPattern)
        memcpy(writeBuffer, fillPattern, writeLength);
    else
        memset(writeBuffer, 0, writeLength);

    uint32_t bytesWritten = 0;
    int result = ESP_OK;

    // Write in chunks
    while(remainingWords > 0)
    {
        uint16_t chunkSize = (remainingWords > writeLength) ? writeLength : remainingWords;
        
        result = writeMany(currentAddress, writeBuffer, chunkSize);
        if(result != ESP_OK)
        {
            MESP_LOGE("FRAM", "Format failed at address 0x%03X: %d", currentAddress, result);
            free(writeBuffer);
            return result;
        }

        bytesWritten += chunkSize;
        currentAddress += writeLength;
        remainingWords -= chunkSize;
    }

    free(writeBuffer);
    MESP_LOGI("FRAM", "Format complete: %d bytes written", bytesWritten);
    
    return ESP_OK;
}

// ===================================================================
// Application-Specific Data Access Methods
// ===================================================================

/**
 * @brief Writes raw bytes to FRAM at specified address
 * 
 * @param address FRAM address to write to
 * @param source Pointer to source data buffer
 * @param count Number of bytes to write
 * @return ESP_OK on success, error code otherwise
 */
int FramI2C::write_bytes(uint32_t address, uint8_t* source, uint32_t count)
{
    if(!source)
    {
        MESP_LOGE("FRAM", "Invalid source buffer for write_bytes");
        return ESP_ERR_INVALID_ARG;
    }

    return writeMany(address, source, count);
}

/**
 * @brief Reads raw bytes from FRAM at specified address
 * 
 * @param address FRAM address to read from
 * @param destination Pointer to destination buffer
 * @param count Number of bytes to read
 * @return ESP_OK on success, error code otherwise
 */
int FramI2C::read_bytes(uint32_t address, uint8_t* destination, uint32_t count)
{
    if(!destination)
    {
        MESP_LOGE("FRAM", "Invalid destination buffer for read_bytes");
        return ESP_ERR_INVALID_ARG;
    }

    return readMany(address, destination, count);
}

/**
 * @brief Writes blower configuration data to FRAM at base address 0
 * 
 * @param blowerData Pointer to blower data structure
 * @param dataSize Size of blower data in bytes
 * @return ESP_OK on success, error code otherwise
 */
int FramI2C::write_Blower(uint8_t *blowerData, uint16_t dataSize)
{
    if(!blowerData)
    {
        MESP_LOGE("FRAM", "Invalid blower data pointer");
        return ESP_ERR_INVALID_ARG;
    }

    const uint32_t BLOWER_BASE_ADDRESS = 0;
    int result = write_bytes(BLOWER_BASE_ADDRESS, blowerData, dataSize);
    
    if(result != ESP_OK)
        MESP_LOGE("FRAM", "Failed to write blower data: 0x%x", result);

    return result;
}

/**
 * @brief Reads blower configuration data from FRAM at base address 0
 * 
 * @param blowerData Pointer to destination buffer for blower data
 * @param dataSize Size of data to read in bytes
 * @return ESP_OK on success, error code otherwise
 */
int FramI2C::read_Blower(uint8_t* blowerData, uint16_t dataSize)
{
    if(!blowerData)
    {
        MESP_LOGE("FRAM", "Invalid blower data pointer");
        return ESP_ERR_INVALID_ARG;
    }

    const uint32_t BLOWER_BASE_ADDRESS = 0;
    return read_bytes(BLOWER_BASE_ADDRESS, blowerData, dataSize);
}
/**
 * @brief Writes recovery data
 * 
 * @param recoveryData Pointer to recovery data structure
 * @param dataSize Size of recovery data in bytes
 * @return ESP_OK on success, error code otherwise
 */
int FramI2C::write_recovery(uint16_t offset,uint8_t *recoveryData, uint16_t dataSize)
{
    if(!recoveryData)
    {
        MESP_LOGE("FRAM", "Invalid recovery data pointer");
        return ESP_ERR_INVALID_ARG;
    }

    const uint32_t RECOVERY_BASE_ADDRESS = offset;
    int result = write_bytes(RECOVERY_BASE_ADDRESS, recoveryData, dataSize);
    
    if(result != ESP_OK)
        MESP_LOGE("FRAM", "Failed to write recovery data: 0x%x", result);

    return result;
}

/**
 * @brief Reads recovery data from FRAM at base address 0
 * 
 * @param recoveryData Pointer to destination buffer for recovery data
 * @param dataSize Size of data to read in bytes
 * @return ESP_OK on success, error code otherwise
 */
int FramI2C::read_recovery(uint16_t offset,uint8_t* recoveryData, uint16_t dataSize)
{
    if(!recoveryData)
    {
        MESP_LOGE("FRAM", "Invalid recovery data pointer");
        return ESP_ERR_INVALID_ARG;
    }

    const uint32_t RECOVERY_BASE_ADDRESS = offset;
    return read_bytes(RECOVERY_BASE_ADDRESS, recoveryData, dataSize);
}

