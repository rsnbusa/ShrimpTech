
#ifndef _FramI2C_H_
#define _FramI2C_H_
#include "includes.h"

/*
 * FramI2C - Driver for Fujitsu MB85RC16 FRAM (Ferroelectric RAM) over I2C
 * 
 * This class provides an interface to the 16Kb FRAM memory chip using ESP-IDF's
 * new I2C master driver API. FRAM offers non-volatile storage with unlimited
 * write endurance and instant write capability.
 */

// I2C Configuration Constants
#define I2C_ACK_CHECK_ENABLE            0x1
#define I2C_ACK_CHECK_DISABLE           0x0
#define I2C_ACK_VALUE                   (i2c_ack_value_t)0x0
#define I2C_NACK_VALUE                  (i2c_ack_value_t)0x1
#define I2C_MASTER_TX_BUF_DISABLE       0
#define I2C_MASTER_RX_BUF_DISABLE       0
#define FRAM_I2C_SLAVE_ADDRESS          0x50
#define I2C_WRITE_BIT                   I2C_MASTER_WRITE
#define I2C_READ_BIT                    I2C_MASTER_READ
#define I2C_BUS_NUMBER                  I2C_NUM_0

// FRAM Memory Constants
#define FRAM_16K_SIZE_WORDS             2048
#define FRAM_I2C_CLOCK_SPEED_HZ         100000
#define FRAM_I2C_GLITCH_FILTER_COUNT    7
#define FRAM_ADDRESS_LOW_MASK           0xFF
#define FRAM_ADDRESS_HIGH_MASK          0x700
#define FRAM_ADDRESS_HIGH_SHIFT         8
#define FRAM_ADDRESS_BYTE_MULTIPLIER    256
#define FRAM_PROTOCOL_OVERHEAD_BYTES    2

class FramI2C {
public:
    FramI2C(void);
    
    // Initialization
    bool begin(int sdaPin, int sclPin, SemaphoreHandle_t *framSemaphore);
    
    // High-level API for application use
    int format(uint8_t *fillBuffer, uint32_t chunkSize, bool verifyAll);
    int write_Blower(uint8_t *data, uint16_t length);
    int read_Blower(uint8_t *buffer, uint16_t length);
    int write_recovery(uint16_t offset,uint8_t *data, uint16_t length);
    int read_recovery(uint16_t offset,uint8_t *buffer, uint16_t length);
    
    // Low-level I2C operations (public for advanced use)
    int i2c_master_read_slave(uint16_t framAddress, uint8_t *readBuffer, size_t readSize);
    esp_err_t i2c_master_write_slave(uint16_t framAddress, uint8_t *writeData, size_t writeSize);

private:
    // Device identification
    void getDeviceID(uint16_t *manufacturerID, uint16_t *productID, uint8_t *densityCode);
    
    // FRAM memory operations
    int writeMany(uint16_t framAddress, uint8_t *data, uint16_t count);
    int readMany(uint16_t framAddress, uint8_t *buffer, uint16_t count);
    int write_bytes(uint32_t address, uint8_t *source, uint32_t count);
    int read_bytes(uint32_t address, uint8_t *destination, uint32_t count);

    // Member variables
    bool _framInitialised;
    uint8_t addressBytes;
    uint32_t intframWords;
    uint32_t maxSpeed;
    uint16_t manufID;
    uint16_t prodId;
    uint8_t density;
    i2c_master_dev_handle_t idev_handle;
};

#endif
