/**
 * @file BlowerClass.h
 * @brief Solar system monitoring and data management class
 * 
 * This module provides the BlowerClass for managing solar energy system data including:
 * - PV panel voltage and current monitoring
 * - Battery state of charge, health, and temperature
 * - Energy generation and consumption tracking
 * - Environmental sensors (temperature, humidity, DO, pH)
 * - System statistics and network metrics
 * - Persistent storage using FRAM (Ferroelectric RAM)
 * 
 * All data is stored in FRAM for non-volatile persistence across power cycles.
 */

#ifndef BLOWERCLASS_H_
#define BLOWERCLASS_H_

#include "includes.h"

// ============================================================================
// PV Panel Data Structures
// ============================================================================

/**
 * @brief Photovoltaic panel monitoring data
 * 
 * Contains voltage and current measurements from two PV strings.
 * Used for tracking solar panel performance and power generation.
 */
typedef struct {
    uint32_t chargeCurr;
    float pv1Volts;             // min-max array pos 20  PV1V
    float pv2Volts;             // pos 20                PV1V
    float pv1Amp;               // pos 19                PV1A
    float pv2Amp;               // pos 19                PV1A
} pvPanel_t;

// ============================================================================
// Battery Data Structures
// ============================================================================

/**
 * @brief Battery management system data
 * 
 * Tracks battery health and state metrics from the BMS (Battery Management System).
 * Essential for battery life estimation and charge/discharge control.
 */
typedef struct {
    uint8_t batSOC;             // pos 18               BMSOC
    uint8_t batSOH;             // pos 17               BMSOH
    uint16_t batteryCycleCount; // pos 16               BMCC
    float batBmsTemp;           // pos 15               BMTEMP
} battery_t;

// ============================================================================
// Energy Tracking Data Structures
// ============================================================================

/**
 * @brief Energy production and consumption metrics
 * 
 * Comprehensive energy tracking including:
 * - Daily and total ampere-hours (charge/discharge)
 * - Energy generation and consumption in kWh
 * - Load consumption metrics
 * 
 * All values reset daily with totals accumulated over device lifetime.
 */
typedef struct {
    uint16_t batChgAHToday;     // pos 14               BMCHAH
    uint16_t batDischgAHToday;  // pos 13               BMDDAH
    uint16_t batChgAHTotal;     // pos 12               BMCHKT
    uint16_t batDischgAHTotal;  // pos 11               BMDDKT
    float generateEnergyToday;  // pos 10               GENEER
    float usedEnergyToday;      // pos 9                USEDEN
    float gLoadConsumLineTotal; // pos 8                LCONLI
    float batChgkWhToday;       // pos 7                BMCHKW
    float batDischgkWhToday;    // pos 6                BMDDKW
    float genLoadConsumToday;   // pos 5                GENLCT
} energy_t;

// ============================================================================
// Environmental Sensor Data Structures
// ============================================================================

/**
 * @brief Environmental sensor readings
 * 
 * Monitors water and air quality parameters critical for aquaculture:
 * - Water temperature and dissolved oxygen (DO)
 * - pH levels
 * - Air temperature and humidity
 * 
 * Note: Packed structure to match hardware data format from sensors.
 */
#pragma pack(push, 1)  // Ensure no padding for direct hardware mapping
typedef struct {
    float  WTemp;               // pos 2               WTEMP  
    float  percentDO;           // 
    float  DO;                  // pos 4               LIMITDO
    float  PH;                  // pos 3               LIMITPH
    float  ATemp;               // pos 1               ATEMP
    float  AHum;                // pos 0               AHUM
} sensor_t;
#pragma pack(pop)  // Restore default packing

// ============================================================================
// Aggregated System Data
// ============================================================================

/**
 * @brief Complete solar energy system data
 * 
 * Aggregates all subsystem data into a single structure:
 * - PV panel metrics
 * - Battery status
 * - Energy tracking
 * - Environmental sensors
 */
typedef struct {
    pvPanel_t pvPanel;
    battery_t battery;
    energy_t  energy;
    sensor_t  sensors;
} solarSystem_t;

/**
 * @brief Network and system statistics
 * 
 * Tracks communication metrics and network activity:
 * - Message counts (in/out)
 * - Data transfer volumes (bytes in/out)
 * - Network topology (node/device counts)
 * - Connection events (connects/disconnects)
 * - Last update timestamp
 */
typedef struct {
    uint32_t msgIn, msgOut, bytesIn, bytesOut;
    uint8_t lastNodeCount, lastBlowerCount, staCons, staDisco;
    time_t lastCountTS;
} stats_t;

// ============================================================================
// FRAM Persistent Storage Structure
// ============================================================================

/**
 * @brief Main persistent data structure stored in FRAM
 * 
 * This structure is written to non-volatile FRAM memory to preserve
 * all system data across power cycles. Contains:
 * - Sentinel value for data validation
 * - Timestamps for tracking data age and lifecycle
 * - FRAM access statistics (wear leveling monitoring)
 * - Subsystem update timestamps for staleness detection
 * - Complete solar system data
 * - Network statistics
 * 
 * The centinel field is used to validate FRAM contents on startup.
 */
typedef struct {
    uint32_t centinel;
    time_t lastUpdate;
    time_t lifeDate, creationDate;
    uint32_t framWrites;
    uint32_t framReads;
    time_t lastDate;
    time_t lastUpdatePV,lastUpdateBat,lastUpdateEnergy,lastUpdateSensors;
    stats_t theStats;
    solarSystem_t solarSystem;
} solarDef_t;

// ============================================================================
// Main Solar System Manager Class
// ============================================================================

/**
 * @class BlowerClass
 * @brief Main class for solar energy system data management
 * 
 * Provides comprehensive management of solar system data with:
 * - Thread-safe access to all system metrics
 * - Persistent storage in FRAM with wear tracking
 * - Atomic updates with semaphore protection
 * - Data validation and integrity checking
 * - Statistics collection and reporting
 * 
 * This class is marked 'final' as it's not designed for inheritance.
 * All data access is synchronized using a semaphore for thread safety.
 */
class BlowerClass final {
public:
    // ========================================================================
    // Lifecycle Management
    // ========================================================================
    
    /**
     * @brief Constructor - initializes the BlowerClass instance
     */
    BlowerClass();
    
    /**
     * @brief Initialize the FRAM storage and load existing data
     * @return 0 on success, error code otherwise
     */
    int initBlower();
    
    /**
     * @brief Deinitialize and release resources
     */
    void deinit();
    
    /**
     * @brief Get the main task handle
     * @return Task handle for the main processing task
     */
    TaskHandle_t getMainTask();

    // ========================================================================
    // FRAM Persistence Operations
    // ========================================================================
    
    /**
     * @brief Load all data from FRAM into memory
     */
    void loadBlower();
    
    /**
     * @brief Save all current data to FRAM
     */
    void saveBlower();
    
    /**
     * @brief Format the FRAM storage (erase all data)
     */
    void format();
    
    /**
     * @brief Erase blower-specific data
     */
    void eraseBlower();
    
    /**
     * @brief Write creation date to FRAM
     * @param ddate Creation timestamp
     */
    void writeCreationDate(time_t ddate);
    
    /**
     * @brief Read creation date from FRAM
     * @return Creation timestamp
     */
    time_t readCreationDate() const;
    
    /**
     * @brief Increment FRAM write counter (for wear tracking)
     */
    void framWrites();
    
    /**
     * @brief Get total FRAM write count
     * @return Number of FRAM writes since initialization
     */
    uint32_t getFram_Writes() const;
    
    /**
     * @brief Increment FRAM read counter
     */
    void framReads();
    
    /**
     * @brief Get total FRAM read count
     * @return Number of FRAM reads since initialization
     */
    uint32_t getFram_Reads() const;

    // ========================================================================
    // Device Identity and Metadata
    // ========================================================================
    
    /**
     * @brief Set device machine ID
     * @param mid Pointer to machine ID string
     */
    void setMID(const char *mid);
    
    /**
     * @brief Get device machine ID
     * @return Pointer to machine ID string
     */
    const char *getMID() const;
    
    /**
     * @brief Set reserved/configuration date
     * @param theDate Timestamp when device was configured
     */
    void setReservedDate(time_t theDate);
    
    /**
     * @brief Get reserved/configuration date
     * @return Configuration timestamp
     */
    time_t getReservedDate() const;
    
    /**
     * @brief Get last update timestamp
     * @return Last data update time
     */
    time_t getLastUpdate() const;
    
    /**
     * @brief Get device lifetime start date
     * @return Date when device was first commissioned
     */
    time_t getLifeDate() const;

    // ========================================================================
    // PV Panel Data Access
    // ========================================================================
    
    /**
     * @brief Set photovoltaic panel data
     * @param chargeCurr Charging current (A)
     * @param pv1Volts PV string 1 voltage (V)
     * @param pv2Volts PV string 2 voltage (V)
     * @param pv1Amp PV string 1 current (A)
     * @param pv2Amp PV string 2 current (A)
     */
    void setPVPanel(uint8_t chargeCurr, float pv1Volts, float pv2Volts, float pv1Amp, float pv2Amp);
    
    /**
     * @brief Get photovoltaic panel data
     * @param[out] chargeCurr Charging current
     * @param[out] pv1Volts PV string 1 voltage
     * @param[out] pv2Volts PV string 2 voltage
     * @param[out] pv1Amp PV string 1 current
     * @param[out] pv2Amp PV string 2 current
     */
    void getPVPanel(uint8_t *chargeCurr, float *pv1Volts, float *pv2Volts, float *pv1Amp, float *pv2Amp);

    // ========================================================================
    // Battery Data Access
    // ========================================================================
    
    /**
     * @brief Set battery management system data
     * @param batSoc Battery state of charge (%)
     * @param batSOH Battery state of health (%)
     * @param batteryCycleCount Total charge/discharge cycles
     * @param batBmsTemp Battery temperature from BMS (°C)
     */
    void setBattery(uint8_t batSoc, uint8_t batSOH, uint16_t batteryCycleCount, float batBmsTemp);
    
    /**
     * @brief Get battery management system data
     * @param[out] batSoc State of charge
     * @param[out] batSOH State of health
     * @param[out] batteryCycleCount Cycle count
     * @param[out] batBmsTemp Temperature
     */
    void getBattery(uint8_t *batSoc, uint8_t *batSOH, uint16_t *batteryCycleCount, float *batBmsTemp);

    // ========================================================================
    // Energy Tracking Data Access
    // ========================================================================
    
    /**
     * @brief Set energy production and consumption metrics
     * @param batChgAHToday Battery charge ampere-hours today
     * @param batDischgAHToday Battery discharge ampere-hours today
     * @param batChgAHTotal Total battery charge ampere-hours
     * @param batDischgAHTotal Total battery discharge ampere-hours
     * @param generateEnergyToday Energy generated today (kWh)
     * @param usedEnergyToday Energy consumed today (kWh)
     * @param gLoadConsumLineTotal Total grid/load consumption
     * @param batChgkWhToday Battery charge energy today (kWh)
     * @param batDischgkWhToday Battery discharge energy today (kWh)
     * @param genLoadConsumToday Generation/load consumption today
     */
    void setEnergy(uint16_t batChgAHToday, uint16_t batDischgAHToday, uint16_t batChgAHTotal,
                   uint16_t batDischgAHTotal, float generateEnergyToday, float usedEnergyToday,
                   float gLoadConsumLineTotal, float batChgkWhToday, float batDischgkWhToday,
                   float genLoadConsumToday);
    
    /**
     * @brief Get energy production and consumption metrics
     * @param[out] batChgAHToday Battery charge AH today
     * @param[out] batDischgAHToday Battery discharge AH today
     * @param[out] batChgAHTotal Total battery charge AH
     * @param[out] batDischgAHTotal Total battery discharge AH
     * @param[out] generateEnergyToday Energy generated today
     * @param[out] usedEnergyToday Energy consumed today
     * @param[out] gLoadConsumLineTotal Total grid consumption
     * @param[out] batChgkWhToday Battery charge kWh today
     * @param[out] batDischgkWhToday Battery discharge kWh today
     * @param[out] genLoadConsumToday Load consumption today
     */
    void getEnergy(uint16_t *batChgAHToday, uint16_t *batDischgAHToday, uint16_t *batChgAHTotal,
                   uint16_t *batDischgAHTotal, float *generateEnergyToday, float *usedEnergyToday,
                   float *gLoadConsumLineTotal, float *batChgkWhToday, float *batDischgkWhToday,
                   float *genLoadConsumToday);

    // ========================================================================
    // Environmental Sensor Data Access
    // ========================================================================
    
    /**
     * @brief Set environmental sensor readings
     * @param DO Dissolved oxygen (mg/L)
     * @param PH pH level (0-14)
     * @param WTemp Water temperature (°C)
     * @param ATemp Air temperature (°C)
     * @param AHum Air humidity (%)
     */
    void setSensors(float DO, float PH, float WTemp, float ATemp, float AHum);
    
    /**
     * @brief Get environmental sensor readings
     * @param[out] DO Dissolved oxygen
     * @param[out] PH pH level
     * @param[out] WTemp Water temperature
     * @param[out] ATemp Air temperature
     * @param[out] AHum Air humidity
     */
    void getSensors(float *DO, float *PH, float *WTemp, float *ATemp, float *AHum);

    // ========================================================================
    // Direct System Data Access
    // ========================================================================
    
    /**
     * @brief Get pointer to complete solar system data structure
     * @return Pointer to solarSystem_t (use with caution - not thread-safe)
     */
    solarSystem_t* getSolarSystem();
    
    /**
     * @brief Get const pointer to complete solar system data structure
     * @return Const pointer to solarSystem_t for read-only access
     */
    const solarSystem_t* getSolarSystem() const;
    
    /**
     * @brief Get pointer to solar system data (alternative name)
     * @return Pointer to solarSystem_t
     */
    solarSystem_t* getPtrSolarsystem();
    
    /**
     * @brief Get const pointer to solar system data
     * @return Const pointer to solarSystem_t for read-only access
     */
    const solarSystem_t* getPtrSolarsystem() const;

    // ========================================================================
    // Network Statistics - Getters
    // ========================================================================
    
    /**
     * @brief Get count of messages received
     * @return Number of incoming messages
     */
    uint32_t getStatsMsgIn() const;
    
    /**
     * @brief Get count of messages sent
     * @return Number of outgoing messages
     */
    uint32_t getStatsMsgOut() const;
    
    /**
     * @brief Get count of bytes received
     * @return Number of incoming bytes
     */
    uint32_t getStatsBytesIn() const;
    
    /**
     * @brief Get count of bytes sent
     * @return Number of outgoing bytes
     */
    uint32_t getStatsBytesOut() const;
    
    /**
     * @brief Get last recorded node count
     * @return Number of nodes in network
     */
    uint8_t getStatsLastNodeCount() const;
    
    /**
     * @brief Get last recorded blower/device count
     * @return Number of blower devices
     */
    uint8_t getStatsLastBlowerCount() const;
    
    /**
     * @brief Get station connection count
     * @return Number of successful connections
     */
    uint8_t getStatsStaConns() const;
    
    /**
     * @brief Get station disconnection count
     * @return Number of disconnections
     */
    uint8_t getStatsStaDiscos() const;
    
    /**
     * @brief Get timestamp of last statistics update
     * @return Last count update time
     */
    time_t getStatsLastCountTS() const;

    // ========================================================================
    // Network Statistics - Setters
    // ========================================================================
    
    /**
     * @brief Set last node count
     * @param count Number of nodes
     */
    void setStatsLastNodeCount(uint8_t count);
    
    /**
     * @brief Set last blower count
     * @param count Number of blowers
     */
    void setStatsLastBlowerCount(uint8_t count);
    
    /**
     * @brief Set statistics update timestamp
     * @param now Current timestamp
     */
    void setStatsLastCountTS(time_t now);
    
    /**
     * @brief Increment station connection counter
     */
    void setStatsStaConns();
    
    /**
     * @brief Increment station disconnection counter
     */
    void setStatsStaDiscos();
    
    /**
     * @brief Increment incoming message counter
     */
    void setStatsMsgIn();
    
    /**
     * @brief Increment outgoing message counter
     */
    void setStatsMsgOut();
    
    /**
     * @brief Add to incoming bytes counter
     * @param count Number of bytes received
     */
    void setStatsBytesIn(uint32_t count);
    
    /**
     * @brief Add to outgoing bytes counter
     * @param count Number of bytes sent
     */
    void setStatsBytesOut(uint32_t count);

private:
    // ========================================================================
    // Private Member Variables
    // ========================================================================
    
    solarDef_t          framConfig;     ///< Main data structure stored in FRAM
    FramI2C             fram;           ///< FRAM I2C interface object
    SemaphoreHandle_t   framSem;        ///< Semaphore for thread-safe FRAM access
    bool                framFlag;       ///< FRAM initialization status (true = ready)
    uint32_t            blowerSize;     ///< Size of framConfig structure in bytes
    
    // Prevent copying and assignment (this class manages hardware resources)
    BlowerClass(const BlowerClass&) = delete;
    BlowerClass& operator=(const BlowerClass&) = delete;
};

#endif  // BLOWERCLASS_H_