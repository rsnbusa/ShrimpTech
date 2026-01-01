/**
 * @file modbus_task_utils.h
 * @brief Shared utilities for Modbus monitoring tasks
 * 
 * Provides common constants, type definitions, and helper functions used across
 * all Modbus device monitoring tasks (energy, battery, panels, sensors).
 * 
 * @note Part of the ShrimpIoT Modbus device monitoring system
 */

#ifndef MODBUS_TASK_UTILS_H
#define MODBUS_TASK_UTILS_H

#include "esp_modbus_master.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Common Modbus Descriptor Field Indices
// ============================================================================

#define DADDR      (4)  ///< Device address field index
#define DOFFSET    (3)  ///< Data offset field index
#define DSTART     (2)  ///< Register start address field index
#define DPOINTS    (1)  ///< Number of registers field index
#define DTYPE      (0)  ///< Data type field index

// ============================================================================
// Common Constants
// ============================================================================

#define BYTE_MASK (0xFF)  ///< Byte mask for data parsing

// ============================================================================
// Type Definitions
// ============================================================================

/**
 * @brief Generic Modbus device specification structure
 * 
 * Base structure that can represent specs from different sensor types.
 * Memory layout matches four_t and five_t from typedef.h:
 * - four_t: {double mux; int devices[4]} for battery/panels/inverter (Type, Points, Start, Offset)
 * - five_t: {double mux; int devices[5]} for sensors (Type, Points, Start, Offset, Addr)
 */
typedef struct {
    double mux;         ///< Multiplier for data scaling
    int devices[5];     ///< Device configuration [DTYPE, DPOINTS, DSTART, DOFFSET, DADDR]
                        ///< Note: Only sensors use DADDR (index 4), others only use 0-3
} modbus_device_spec_t;

/**
 * @brief Generic sensor information container
 * 
 * Used to pass sensor configuration to generic initialization functions.
 */
typedef struct {
    uint8_t addr;                   ///< Default Modbus slave address (for non-sensor tasks)
    uint8_t regfresh;               ///< Refresh rate in minutes
    void *specs;                    ///< Pointer to first spec (either four_t or five_t array)
    int max_sensors;                ///< Maximum number of sensors in specs array
    int spec_size_bytes;            ///< Size of each spec entry (24 for four_t, 32 for five_t)
    bool use_per_device_addr;       ///< True if each device has its own address (sensors task)
} modbus_sensor_config_t;

/**
 * @brief Callback function for printing sensor data
 * 
 * @param data_ptr Pointer to sensor data structure
 * @param errors Pointer to error array
 */
typedef void (*print_sensor_data_fn)(void *data_ptr, const int *errors);

// ============================================================================
// Function Declarations
// ============================================================================

/**
 * @brief Initialize Modbus parameter descriptors from configuration
 * 
 * Generic function that creates Modbus parameter descriptors for any sensor type.
 * Handles memory allocation, register configuration, and data type mapping.
 * Uses byte-offset arithmetic to handle different spec sizes (four_t vs five_t).
 * 
 * @param descriptors Pointer to array of mb_parameter_descriptor_t (must be pre-allocated)
 * @param config Pointer to sensor configuration structure
 * @param task_name Prefix for parameter labels (e.g., "Energy", "Battery")
 * @param log_color ANSI color code for debug logging
 * @return int Number of successfully initialized descriptors, -1 on error
 * 
 * @note Allocates memory for param_key and param_units - caller must free
 * @note Skips sensors with invalid offset (< 0)
 */
int modbus_init_descriptors(mb_parameter_descriptor_t *descriptors,
                            const modbus_sensor_config_t *config,
                            const char *task_name,
                            const char *log_color);

/**
 * @brief Free memory allocated for descriptor labels
 * 
 * Cleans up param_key and param_units strings allocated during initialization.
 * 
 * @param descriptors Pointer to descriptor array
 * @param count Number of descriptors to free
 */
void modbus_free_descriptors(mb_parameter_descriptor_t *descriptors, int count);

/**
 * @brief Get human-readable name for parameter type
 * 
 * @param type_index Type index (0-7)
 * @return const char* Type name string
 */
const char* modbus_get_type_name(int type_index);

/**
 * @brief Generic Modbus monitoring task runner
 * 
 * Executes the common task loop pattern used by all Modbus monitoring tasks:
 * 1. Allocate descriptors
 * 2. Initialize from configuration
 * 3. Send read requests to RS485 queue
 * 4. Wait for data
 * 5. Call print callback
 * 6. Repeat with configured delay
 * 
 * @param config Sensor configuration structure
 * @param data_receiver Pointer to data structure that receives sensor values
 * @param print_callback Function to print/log the sensor data
 * @param max_sensors Maximum number of sensors
 * @param task_name Task name for logging (e.g., "Energy", "Battery")
 * @param log_color ANSI color code for log messages
 * 
 * @note This function never returns - it runs indefinitely
 * @note Task will delete itself if initialization fails
 */
void modbus_task_runner(const modbus_sensor_config_t *config,
                       void *data_receiver,
                       print_sensor_data_fn print_callback,
                       int max_sensors,
                       const char *task_name,
                       const char *log_color);

#ifdef __cplusplus
}
#endif

#endif // MODBUS_TASK_UTILS_H
