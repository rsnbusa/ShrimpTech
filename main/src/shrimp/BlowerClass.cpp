
/**
 * @file BlowerClass.cpp
 * @brief Implementation of BlowerClass for solar system data management
 * 
 * This file implements the BlowerClass methods for managing solar energy system data
 * with persistent storage in FRAM (Ferroelectric RAM). Provides thread-safe access
 * to all subsystem data including PV panels, battery, energy tracking, and sensors.
 * 
 * Key features:
 * - Atomic FRAM read/write operations with semaphore protection
 * - Automatic timestamp updates on data changes
 * - Wear leveling tracking (read/write counts)
 * - Data validation using sentinel values
 */

#include "BlowerClass.h"
#include "includes.h"
#include "typedef.h"
#include "defines.h"
#include "time_utils.h"

wschedule_t* BlowerClass::getSchedulePtr()
{
    return &framConfig.solarSystem.wschedule;
}

// ============================================================================
// Lifecycle Management
// ============================================================================

BlowerClass::BlowerClass()
{
    bzero(&framConfig, sizeof(framConfig));
    blowerSize = sizeof(framConfig);
}

// ============================================================================
// Device Identity and Metadata
// ============================================================================

void BlowerClass::setReservedDate(time_t theDate)
{
    framConfig.lastDate = theDate;
}

time_t BlowerClass::getLastUpdate() const
{
    return framConfig.lastUpdate;
}

time_t BlowerClass::getLifeDate() const
{
    return framConfig.lifeDate;
}

time_t BlowerClass::getReservedDate() const
{
    return framConfig.lastDate;
}

// ============================================================================
// FRAM Persistence Operations
// ============================================================================

void BlowerClass::deinit()
{
    uint32_t copyFramW = framConfig.framWrites;
    bzero(&framConfig, sizeof(framConfig));
    framConfig.framWrites = copyFramW;
}

void BlowerClass::format()
{
    if (xSemaphoreTake(framSem, portMAX_DELAY / portTICK_PERIOD_MS))
    {
        fram.format(NULL, 3000, true);
        framConfig.sentinel = SENTINEL;
        xSemaphoreGive(framSem);
    }
    else
    {
        MESP_LOGE(MESH_TAG, "format: failed to acquire semaphore");
        return;
    }
    saveBlower();
}

/**
 * Initialize FRAM and validate stored data using centinel value.
 * Returns ESP_FAIL if FRAM initialization fails or centinel is invalid.
 */
int BlowerClass::initBlower()
{
    framFlag = fram.begin(FSDA, FSCL, &framSem);
    if (framFlag)
    {
        loadBlower();
        if (framConfig.sentinel != SENTINEL)
        {
            MESP_LOGE(MESH_TAG, "FRAM Sentinel %x failed... should format FRAM", framConfig.sentinel);
            return ESP_FAIL;
        }
    }
    else
    {
        return ESP_FAIL;
    }
    return ESP_OK;
}

// ============================================================================
// Direct System Data Access
// ============================================================================

solarSystem_t* BlowerClass::getSolarSystem()
{
    return &(framConfig.solarSystem);
}

const solarSystem_t* BlowerClass::getSolarSystem() const
{
    return &(framConfig.solarSystem);
}

/**
 * Load complete configuration from FRAM into memory.
 * Thread-safe with semaphore protection.
 */
void BlowerClass::loadBlower()
{
    if (xSemaphoreTake(framSem, portMAX_DELAY / portTICK_PERIOD_MS))
    {
        fram.read_Blower((uint8_t*)&framConfig, sizeof(framConfig));
        xSemaphoreGive(framSem);
    }
    else
    {
        MESP_LOGE(MESH_TAG, "loadBlower: failed to acquire semaphore");
    }
    framReads();
}

/**
 * Save complete configuration to FRAM.
 * Automatically updates lastUpdate timestamp and increments write counter.
 * Thread-safe with semaphore protection.
 */
void BlowerClass::saveBlower()
{
    framWrites();
    if (xSemaphoreTake(framSem, portMAX_DELAY / portTICK_PERIOD_MS))
    {
        // Update timestamp every time we save to FRAM
        framConfig.lastUpdate = time(NULL);
        fram.write_Blower((uint8_t*)&framConfig, sizeof(framConfig));
        xSemaphoreGive(framSem);
    }
    else
    {
        MESP_LOGE(MESH_TAG, "saveBlower: failed to acquire semaphore");
    }
}

solarSystem_t* BlowerClass::getPtrSolarsystem()
{
    return getSolarSystem();  // Reuse existing method
}

void BlowerClass::writeCreationDate(time_t ddate)
{
    framConfig.creationDate = ddate;
    saveBlower();
}

void BlowerClass::eraseBlower()
{
    // Reserved for future implementation
    // Currently, FRAM erase is handled by format() method
}

time_t BlowerClass::readCreationDate() const
{
    return framConfig.creationDate;
}

// Internal methods for tracking FRAM wear leveling
void BlowerClass::framWrites()
{
    framConfig.framWrites++;
}

void BlowerClass::framReads()
{
    framConfig.framReads++;
}

uint32_t BlowerClass::getFram_Writes() const
{
    return framConfig.framWrites;
}

uint32_t BlowerClass::getFram_Reads() const
{
    return framConfig.framReads;
}

// ============================================================================
// Network Statistics
// ============================================================================

uint32_t BlowerClass::getStatsMsgIn() const
{
    return framConfig.theStats.msgIn;
}
uint32_t BlowerClass::getStatsMsgOut() const
{
    return framConfig.theStats.msgOut;
}

uint32_t BlowerClass::getStatsBytesOut() const
{
    return framConfig.theStats.bytesOut;
}

uint32_t BlowerClass::getStatsBytesIn() const
{
    return framConfig.theStats.bytesIn;
}

uint8_t BlowerClass::getStatsLastNodeCount() const
{
    return framConfig.theStats.lastNodeCount;
}

uint8_t BlowerClass::getStatsLastBlowerCount() const
{
    return framConfig.theStats.lastBlowerCount;
}

uint8_t BlowerClass::getStatsStaConns() const
{
    return framConfig.theStats.staCons;
}

uint8_t BlowerClass::getStatsStaDiscos() const
{
    return framConfig.theStats.staDisco;
}

time_t BlowerClass::getStatsLastCountTS() const
{
    return framConfig.theStats.lastCountTS;
}

void BlowerClass::setStatsLastNodeCount(uint8_t count)
{
    framConfig.theStats.lastNodeCount = count;
}

void BlowerClass::setStatsLastBlowerCount(uint8_t count)
{
    framConfig.theStats.lastBlowerCount = count;
}

void BlowerClass::setStatsLastCountTS(time_t now)
{
    framConfig.theStats.lastCountTS = now;
}

void BlowerClass::setStatsStaConns()
{
    framConfig.theStats.staCons++;
}

void BlowerClass::setStatsStaDiscos()
{
    framConfig.theStats.staDisco++;
}

void BlowerClass::setStatsMsgIn()
{
    framConfig.theStats.msgIn++;
}

void BlowerClass::setStatsMsgOut()
{
    framConfig.theStats.msgOut++;
}

void BlowerClass::setStatsBytesOut(uint32_t count)
{
    framConfig.theStats.bytesOut += count;
}

void BlowerClass::setStatsBytesIn(uint32_t count)
{
    framConfig.theStats.bytesIn += count;
}

// ============================================================================
// PV Panel Data Access
// ============================================================================

void BlowerClass::setPVPanel(uint8_t chargeCurr, float pv1Volts, float pv2Volts, float pv1Amp, float pv2Amp)
{
    framConfig.solarSystem.pvPanel.chargeCurr = chargeCurr;
    framConfig.solarSystem.pvPanel.pv1Volts = pv1Volts;
    framConfig.solarSystem.pvPanel.pv2Volts = pv2Volts;
    framConfig.solarSystem.pvPanel.pv1Amp = pv1Amp;
    framConfig.solarSystem.pvPanel.pv2Amp = pv2Amp;
    saveBlower();
}

void BlowerClass::getPVPanel(uint8_t *chargeCurr, float *pv1Volts, float *pv2Volts, float *pv1Amp, float *pv2Amp)
{
    if (!chargeCurr || !pv1Volts || !pv2Volts || !pv1Amp || !pv2Amp) {
        MESP_LOGE(MESH_TAG, "getPVPanel: null pointer argument");
        return;
    }
    *chargeCurr = framConfig.solarSystem.pvPanel.chargeCurr;
    *pv1Volts = framConfig.solarSystem.pvPanel.pv1Volts;
    *pv2Volts = framConfig.solarSystem.pvPanel.pv2Volts;
    *pv1Amp = framConfig.solarSystem.pvPanel.pv1Amp;
    *pv2Amp = framConfig.solarSystem.pvPanel.pv2Amp;
}

// ============================================================================
// Battery Data Access
// ============================================================================

void BlowerClass::setBattery(uint8_t batSoc, uint8_t batSOH, uint16_t batteryCycleCount, float batBmsTemp)
{
    framConfig.solarSystem.battery.batSOC = batSoc;
    framConfig.solarSystem.battery.batSOH = batSOH;
    framConfig.solarSystem.battery.batteryCycleCount = batteryCycleCount;
    framConfig.solarSystem.battery.batBmsTemp = batBmsTemp;
    saveBlower();
}

void BlowerClass::getBattery(uint8_t *batSoc, uint8_t *batSOH, uint16_t *batteryCycleCount, float *batBmsTemp)
{
    if (!batSoc || !batSOH || !batteryCycleCount || !batBmsTemp) {
        MESP_LOGE(MESH_TAG, "getBattery: null pointer argument");
        return;
    }
    *batSoc = framConfig.solarSystem.battery.batSOC;
    *batSOH = framConfig.solarSystem.battery.batSOH;
    *batteryCycleCount = framConfig.solarSystem.battery.batteryCycleCount;
    *batBmsTemp = framConfig.solarSystem.battery.batBmsTemp;
}

// ============================================================================
// VFD Data Access
// ============================================================================

void BlowerClass::setVFD(float mcurrent, uint16_t mvolts, float mpower, uint16_t mrpm)
{
    framConfig.solarSystem.vfd.mcurrent = mcurrent;
    framConfig.solarSystem.vfd.mvolts = mvolts;
    framConfig.solarSystem.vfd.mpower = mpower;
    framConfig.solarSystem.vfd.mrpm = mrpm;
    saveBlower();
}

void BlowerClass::getVFD(float *mcurrent, uint16_t *mvolts, float *mpower, uint16_t *mrpm)
{
    if (!mcurrent || !mvolts || !mpower || !mrpm) {
        MESP_LOGE(MESH_TAG, "getVFD: null pointer argument");
        return;
    }
    *mcurrent = framConfig.solarSystem.vfd.mcurrent;
    *mvolts = framConfig.solarSystem.vfd.mvolts;
    *mpower = framConfig.solarSystem.vfd.mpower;
    *mrpm = framConfig.solarSystem.vfd.mrpm;
}

// ============================================================================
// Energy Tracking Data Access
// ============================================================================

void BlowerClass::setEnergy(uint16_t batChgAHToday, uint16_t batDischgAHToday, uint16_t batChgAHTotal,
                             uint16_t batDischgAHTotal, float generateEnergyToday, float usedEnergyToday,
                             float gLoadConsumLineTotal, float batChgkWhToday, float batDischgkWhToday,
                             float genLoadConsumToday)
{
    framConfig.solarSystem.energy.batChgAHToday = batChgAHToday;
    framConfig.solarSystem.energy.batDischgAHToday = batDischgAHToday;
    framConfig.solarSystem.energy.batChgAHTotal = batChgAHTotal;
    framConfig.solarSystem.energy.batDischgAHTotal = batDischgAHTotal;
    framConfig.solarSystem.energy.generateEnergyToday = generateEnergyToday;
    framConfig.solarSystem.energy.usedEnergyToday = usedEnergyToday;
    framConfig.solarSystem.energy.gLoadConsumLineTotal = gLoadConsumLineTotal;
    framConfig.solarSystem.energy.batChgkWhToday = batChgkWhToday;
    framConfig.solarSystem.energy.batDischgkWhToday = batDischgkWhToday;
    framConfig.solarSystem.energy.genLoadConsumToday = genLoadConsumToday;
    saveBlower();
}

void BlowerClass::getEnergy(uint16_t *batChgAHToday, uint16_t *batDischgAHToday, uint16_t *batChgAHTotal,
                             uint16_t *batDischgAHTotal, float *generateEnergyToday, float *usedEnergyToday,
                             float *gLoadConsumLineTotal, float *batChgkWhToday, float *batDischgkWhToday,
                             float *genLoadConsumToday)
{
    if (!batChgAHToday || !batDischgAHToday || !batChgAHTotal || !batDischgAHTotal ||
        !generateEnergyToday || !usedEnergyToday || !gLoadConsumLineTotal ||
        !batChgkWhToday || !batDischgkWhToday || !genLoadConsumToday) {
        MESP_LOGE(MESH_TAG, "getEnergy: null pointer argument");
        return;
    }
    *batChgAHToday = framConfig.solarSystem.energy.batChgAHToday;
    *batDischgAHToday = framConfig.solarSystem.energy.batDischgAHToday;
    *batChgAHTotal = framConfig.solarSystem.energy.batChgAHTotal;
    *batDischgAHTotal = framConfig.solarSystem.energy.batDischgAHTotal;
    *generateEnergyToday = framConfig.solarSystem.energy.generateEnergyToday;
    *usedEnergyToday = framConfig.solarSystem.energy.usedEnergyToday;
    *gLoadConsumLineTotal = framConfig.solarSystem.energy.gLoadConsumLineTotal;
    *batChgkWhToday = framConfig.solarSystem.energy.batChgkWhToday;
    *batDischgkWhToday = framConfig.solarSystem.energy.batDischgkWhToday;
    *genLoadConsumToday = framConfig.solarSystem.energy.genLoadConsumToday;
}

// ============================================================================
// Environmental Sensor Data Access
// ============================================================================

void BlowerClass::getSensors(float *DO, float *PH, float *WTemp, float *ATemp, float *AHum)
{
    if (!DO || !PH || !WTemp || !ATemp || !AHum) {
        MESP_LOGE(MESH_TAG, "getSensors: null pointer argument");
        return;
    }
    *DO = framConfig.solarSystem.sensors.DO;
    *PH = framConfig.solarSystem.sensors.PH;
    *WTemp = framConfig.solarSystem.sensors.WTemp;
    *ATemp = framConfig.solarSystem.sensors.ATemp;
    *AHum = framConfig.solarSystem.sensors.AHum;
};

void BlowerClass::setSensors(float DO, float PH, float WTemp, float ATemp, float AHum)
{
    framConfig.solarSystem.sensors.DO = DO;
    framConfig.solarSystem.sensors.PH = PH;
    framConfig.solarSystem.sensors.WTemp = WTemp;
    framConfig.solarSystem.sensors.ATemp = ATemp;
    framConfig.solarSystem.sensors.AHum = AHum;
    saveBlower();
}

// ============================================================================
// Schedule Management
// ============================================================================

void BlowerClass::setSchedule(uint16_t currentProfile,uint16_t currentCycle, uint16_t currentDay, uint16_t currentHorario,
                              uint16_t currentStartHour, uint16_t currentEndHour, uint16_t currentPwmDuty, uint16_t status)
{
    framConfig.solarSystem.wschedule.currentProfile = currentProfile;
    framConfig.solarSystem.wschedule.currentCycle = currentCycle;
    framConfig.solarSystem.wschedule.currentDay = currentDay;
    framConfig.solarSystem.wschedule.currentHorario = currentHorario;
    framConfig.solarSystem.wschedule.currentStartHour = currentStartHour;
    framConfig.solarSystem.wschedule.currentEndHour = currentEndHour;
    framConfig.solarSystem.wschedule.currentPwmDuty = currentPwmDuty;
    framConfig.solarSystem.wschedule.status = status;
    saveBlower();
}

void BlowerClass::getSchedule(uint16_t *currentCycle, uint16_t *currentDay, uint16_t *currentHorario,
                              uint16_t *currentStartHour, uint16_t *currentEndHour, uint16_t *currentPwmDuty, uint16_t *status)
{
    if (!currentCycle || !currentDay || !currentHorario || !currentStartHour || !currentEndHour || !currentPwmDuty || !status) {
        MESP_LOGE(MESH_TAG, "getSchedule: null pointer argument");
        return;
    }
    *currentCycle = framConfig.solarSystem.wschedule.currentCycle;
    *currentDay = framConfig.solarSystem.wschedule.currentDay;
    *currentHorario = framConfig.solarSystem.wschedule.currentHorario;
    *currentStartHour = framConfig.solarSystem.wschedule.currentStartHour;
    *currentEndHour = framConfig.solarSystem.wschedule.currentEndHour;
    *currentPwmDuty = framConfig.solarSystem.wschedule.currentPwmDuty;
    *status = framConfig.solarSystem.wschedule.status;
}

void BlowerClass::setScheduleStruct(const wschedule_t &sched)
{
    memcpy(&framConfig.solarSystem.wschedule, &sched, sizeof(wschedule_t));
    saveBlower();
}

void BlowerClass::getScheduleStruct(wschedule_t *sched)
{
    if (!sched) {
        MESP_LOGE(MESH_TAG, "getScheduleStruct: null pointer argument");
        return;
    }
    memcpy(sched, &framConfig.solarSystem.wschedule, sizeof(wschedule_t));
}

void BlowerClass::setScheduleStatus(uint16_t status)
{
    framConfig.solarSystem.wschedule.status = status;
    saveBlower();
}

uint16_t BlowerClass::getScheduleStatus() const
{
    return framConfig.solarSystem.wschedule.status;
}

void BlowerClass::saveRecovery(recovery_t *recoveryData)
{
    if (!recoveryData) {
        MESP_LOGE(MESH_TAG, "saveRecovery: null pointer argument");
        return;
    }
    memcpy(&recoveryData,recoveryData,sizeof(recovery_t));
    framWrites();
    if (xSemaphoreTake(framSem, portMAX_DELAY / portTICK_PERIOD_MS))
    {
        // Update timestamp every time we save to FRAM
        framConfig.lastUpdate = time(NULL);
        fram.write_recovery(sizeof(framConfig),(uint8_t*)recoveryData, sizeof(recovery_t));
        xSemaphoreGive(framSem);
    }
    else
    {
        MESP_LOGE(MESH_TAG, "SaveRecovery: failed to acquire semaphore");
    }
}

void BlowerClass::readRecovery(recovery_t *recoveryDatain)
{
    if (!recoveryDatain) {
        MESP_LOGE(MESH_TAG, "readRecovery: null pointer argument");
        return;
    }
    fram.read_recovery(sizeof(framConfig),(uint8_t*)&recoveryData, sizeof(recovery_t));
    memcpy(recoveryDatain, &recoveryData, sizeof(recovery_t));
}