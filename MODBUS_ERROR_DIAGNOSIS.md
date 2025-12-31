# Modbus Error Diagnosis: ESP_ERR_INVALID_RESPONSE (0x108)

## Error Context
- **Error Code**: 0x108 = `ESP_ERR_INVALID_RESPONSE`
- **Source**: `mbc_master_get_parameter()` in Modbus master controller
- **Location**: [rs485.cpp](main/src/rs485.cpp#L138)
- **Meaning**: Modbus master received an invalid/unexpected response from a slave device

## Root Cause Analysis

The error occurs when the RS485 Modbus master tries to read parameters from a slave device and the slave's response doesn't match the expected format. This indicates:

1. **Physical/Communication Issues**:
   - RS485 transceiver not properly connected
   - Wiring problems (A/B twisted pair reversed, missing termination)
   - Bus voltage outside valid range
   - Glitches or noise on the serial line

2. **Slave Device Issues**:
   - Slave device offline or not responding
   - Slave with wrong address configured
   - Slave firmware mismatch or corruption
   - Slave buffer overflow or message queue full

3. **Protocol/Timing Issues**:
   - Slave response timeout (master expects response too quickly)
   - Slave baudrate mismatch (9600 bps in your config)
   - CRC errors in slave response
   - Message framing errors

4. **Configuration Mismatches**:
   - Modbus function code not supported by slave
   - Requested register address doesn't exist on slave
   - Data type mismatch between master parameter descriptor and slave response

## Troubleshooting Checklist

### 1. **Verify RS485 Physical Layer**
- [ ] Check RS485 A/B cable connections
- [ ] Verify termination resistors (120Ω) at both bus ends
- [ ] Test differential voltage with oscilloscope
- [ ] Confirm all slave devices powered on
- [ ] Check GPIO pins (RS485RX, RS485TX) defined correctly

**Your Configuration**:
```cpp
// From rs485.cpp
MODBUS_PORT = 1
MODBUS_BAUD = 9600
MODBUS_MODE = RTU (0)
UART_MODE = UART_MODE_RS485_HALF_DUPLEX
```

### 2. **Check Slave Device Status**
- [ ] Verify each slave device is powered and connected
- [ ] Check slave device LED indicators (if available)
- [ ] Confirm slave address matches configuration in Modbus descriptor
- [ ] Monitor slave error logs/status messages
- [ ] Test slave response with independent RS485 monitor

### 3. **Analyze Modbus Descriptor Configuration**
Look at your parameter descriptors for:
- Correct **slave address** (`mb_slave_addr`)
- Correct **function code** (typically 3=read holding registers)
- Correct **starting address** and **register count**
- Data type compatibility with slave response

**Check file**: [misparams.h](main/inc/misparams.h) for parameter definitions

### 4. **Monitor Communication**
Add debugging to rs485.cpp around line 138:

```cpp
// Before mbc_master_get_parameter call:
ESP_LOGI(TAG, "Requesting cid=%d addr=0x%x slave=%d from %s",
         cid, param_descriptor->mb_reg_start, 
         param_descriptor->mb_slave_addr,
         param_descriptor->param_key);

// After call - log response data:
if (err == ESP_OK) {
    ESP_LOGI(TAG, "SUCCESS: Got %d bytes from slave", type);
} else {
    ESP_LOGE(TAG, "FAILED: err=0x%x from slave addr=%d",
             err, param_descriptor->mb_slave_addr);
}
```

### 5. **Increase Timeout Values**
If slaves are slow to respond, increase Modbus timeouts in `mbc_master_init_tcp()` config

### 6. **Check Slave Response Format**
Verify slave is responding with:
- Correct function code in response
- Correct CRC calculation
- Expected byte count
- Valid register values

### 7. **Test with Simpler Protocol**
Consider using `rs485direct.cpp` functions for direct testing:
```cpp
modbus_rec_t* test_record = setUpModbusRecord(
    slave_address,
    3,  // Read Holding Registers
    register_address,
    register_count
);
// Send and analyze raw response
```

## Affected Code

**Master read loop**: [rs485.cpp lines 120-150](main/src/rs485.cpp#L120-L150)
- Iterates through CID (Characteristic IDs)
- Calls `mbc_master_get_parameter()` for each
- Logs errors with slave address and register info

**Configuration sources**:
- Battery task: [batteryTask.cpp line 108](main/src/batteryTask.cpp#L108)
- Main config: `theConf.modbus_battery`, `modbus_inverter`, `modbus_sensors`, `modbus_panels`

## Next Steps

1. **Identify which slave device is failing** - Check the logged error messages for the slave address (mb_slave_addr)
2. **Test that slave independently** - Use RS485 analyzer or another master to confirm slave functionality
3. **Check parameter descriptor** - Verify the CID information matches slave specifications
4. **Monitor with oscilloscope** - Capture RS485 bus traffic to see actual request/response
5. **Enable debug flags** - Check if Modbus debug flags are enabled: `(theConf.debug_flags >> dMODBUS) & 1U`

## Related Files
- I2C/FRAM interface: [FramI2C.cpp](main/src/FramI2C.cpp) - uses I2C_NUM_0, SDA=GPIO2, SCL=GPIO42
- LCD I2C: [lcd.cpp line 95](main/src/lcd.cpp#L95) - also uses I2C with potential conflicts
- Battery monitoring: [batteryTask.cpp](main/src/batteryTask.cpp) - Modbus battery descriptor setup
