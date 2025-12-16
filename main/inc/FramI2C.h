
#ifndef _FramI2C_H_
#define _FramI2C_H_
#include "includes.h"

#define ACK_CHECK_EN                    0x1                        /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS                   0x0                       /*!< I2C master will not check ack from slave */
#define ACK_VAL                         0x0                             /*!< I2C ack value */
#define NACK_VAL                        0x1    
#define I2C_MASTER_TX_BUF_DISABLE       0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE       0                           /*!< I2C master doesn't need buffer */
#define ESP_SLAVE_ADDRF                 (0x50)
#define WRITE_BIT                       I2C_MASTER_WRITE              /*!< I2C master write */
#define READ_BIT                        I2C_MASTER_READ                /*!< I2C master read */
#define I2C_NUMBER                      I2C_NUM_0

class FramI2C {
public:
    FramI2C(void);
    
    bool 		begin(int sda, int scl, SemaphoreHandle_t *framSem);
// Internal call
// App calls
    int 		format( uint8_t *buffer,uint32_t len,bool all);
    int         write_Blower(uint8_t *mid,uint16_t len);
    int         read_Blower( uint8_t*  value,uint16_t len);
    int         i2c_master_read_slave(uint16_t address, uint8_t *data_rd, size_t size);
    esp_err_t   i2c_master_write_slave( uint16_t address, uint8_t *data_wr, size_t size);



private:
    void        getDeviceID(uint16_t *manufacturerID, uint16_t *productID, uint8_t * density);
    int 		writeMany (uint16_t framAddr, uint8_t *valores,uint16_t son);
    int			readMany (uint16_t framAddr, uint8_t *valores,uint16_t son);
    int         write_bytes(uint32_t add,uint8_t*  desde,uint32_t cuantos);
    int         read_bytes(uint32_t add,uint8_t*  donde,uint32_t cuantos);

public:
    bool _framInitialised;
    uint8_t addressBytes;
    uint32_t intframWords,maxSpeed;
    uint16_t manufID,prodId;
    uint8_t density;
    i2c_master_dev_handle_t idev_handle;
};

#endif
