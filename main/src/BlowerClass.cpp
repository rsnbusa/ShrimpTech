#ifndef BlowerC_H_
#define BlowerC_H_
#include "BlowerClass.h"
#include "includes.h"
#include "typedef.h"
#include "defines.h"
extern void delay(uint32_t a); 

BlowerClass::BlowerClass()
{
    bzero(&framConfig, sizeof(framConfig));
    blowerSize = sizeof(framConfig);
}

void BlowerClass::setReservedDate(time_t theDate)
{
    framConfig.lastDate = theDate;
}

time_t BlowerClass::getLastUpdate()
{
    return framConfig.lastUpdate;
}

time_t BlowerClass::getLifeDate()
{
    return framConfig.lifeDate;
}

time_t BlowerClass::getReservedDate()
{
    return framConfig.lastDate;
}


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
        framConfig.centinel = CENTINEL;
        xSemaphoreGive(framSem);
    }
    saveBlower();
}

int BlowerClass::initBlower()
{
    framFlag = fram.begin(FSDA, FSCL, &framSem);
    if (framFlag)
    {
        loadBlower();
        if (framConfig.centinel != CENTINEL)
        {
            ESP_LOGE(MESH_TAG, "FRAM Centinel %x failed... should format FRAM", framConfig.centinel);
            return ESP_FAIL;
        }
    }
    else
    {
        return ESP_FAIL;
    }
    return ESP_OK;
}

solarSystem_t* BlowerClass::getSolarSystem()
{
    return &(framConfig.solarSystem);
}
void BlowerClass::loadBlower()
{
    if (xSemaphoreTake(framSem, portMAX_DELAY / portTICK_PERIOD_MS))
    {
        fram.read_Blower((uint8_t*)&framConfig, sizeof(framConfig));
        xSemaphoreGive(framSem);
    }
    framReads();
}

// void * BlowerClass::getLimits()
// {
//     return (void*)&limits;
// }

// void BlowerClass::setLimits(void * lims)
// {
//     memcpy(&limits,lims,sizeof(limits));
//     saveBlower();
// }

void BlowerClass::saveBlower()
{
    framWrites();
    if (xSemaphoreTake(framSem, portMAX_DELAY / portTICK_PERIOD_MS))
    {
        framConfig.lastUpdate = time(NULL); //every time we do this save we update last update time
        fram.write_Blower((uint8_t*)&framConfig, sizeof(framConfig));
        xSemaphoreGive(framSem);
    }
}

solarSystem_t* BlowerClass::getPtrSolarsystem()
{
    return &framConfig.solarSystem;
}

void BlowerClass::writeCreationDate(time_t ddate)
{
    framConfig.creationDate = ddate;
    saveBlower();
}

void BlowerClass::eraseBlower()
{
}

time_t BlowerClass::readCreationDate()
{
    return framConfig.creationDate;
}

void BlowerClass::framWrites()
{
    framConfig.framWrites++;
}

void BlowerClass::framReads()
{
    framConfig.framReads++;
}

uint32_t BlowerClass::getFram_Writes()
{
    return framConfig.framWrites;
}

uint32_t BlowerClass::getFram_Reads()
{
    return framConfig.framReads;
}

uint32_t BlowerClass::getStatsMsgIn()
{
    return framConfig.theStats.msgIn;
}
uint32_t BlowerClass::getStatsMsgOut()
{
    return framConfig.theStats.msgOut;
}

uint32_t BlowerClass::getStatsBytesOut()
{
    return framConfig.theStats.bytesOut;
}

uint32_t BlowerClass::getStatsBytesIn()
{
    return framConfig.theStats.bytesIn;
}

uint8_t BlowerClass::getStatsLastNodeCount()
{
    return framConfig.theStats.lastNodeCount;
}

uint8_t BlowerClass::getStatsLastBlowerCount()
{
    return framConfig.theStats.lastBlowerCount;
}

uint8_t BlowerClass::getStatsStaConns()
{
    return framConfig.theStats.staCons;
}

uint8_t BlowerClass::getStatsStaDiscos()
{
    return framConfig.theStats.staDisco;
}

time_t BlowerClass::getStatsLastCountTS()
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

void BlowerClass::setPVPanel(uint8_t chargeCurr, float pv1Volts, float pv2Volts, float pv1Amp, float pv2Amp)
{
    framConfig.solarSystem.pvPanel.chargeCurr = chargeCurr;
    framConfig.solarSystem.pvPanel.pv1Volts = pv1Volts;
    framConfig.solarSystem.pvPanel.pv2Volts = pv2Volts;
    framConfig.solarSystem.pvPanel.pv1Amp = pv1Amp;
    framConfig.solarSystem.pvPanel.pv2Amp = pv2Amp;
    framConfig.lastUpdatePV = time(NULL);
    saveBlower();
}

void BlowerClass::getPVPanel(uint8_t *chargeCurr, float *pv1Volts, float *pv2Volts, float *pv1Amp, float *pv2Amp)
{
    *chargeCurr = framConfig.solarSystem.pvPanel.chargeCurr;
    *pv1Volts = framConfig.solarSystem.pvPanel.pv1Volts;
    *pv2Volts = framConfig.solarSystem.pvPanel.pv2Volts;
    *pv1Amp = framConfig.solarSystem.pvPanel.pv1Amp;
    *pv2Amp = framConfig.solarSystem.pvPanel.pv2Amp;
}

void BlowerClass::setBattery(uint8_t batSoc, uint8_t batSOH, uint16_t batteryCycleCount, float batBmsTemp)
{
    framConfig.solarSystem.battery.batSoc = batSoc;
    framConfig.solarSystem.battery.batSOH = batSOH;
    framConfig.solarSystem.battery.batteryCycleCount = batteryCycleCount;
    framConfig.solarSystem.battery.batBmsTemp = batBmsTemp;
    framConfig.lastUpdateBat = time(NULL);
    saveBlower();
}

void BlowerClass::getBattery(uint8_t *batSoc, uint8_t *batSOH, uint16_t *batteryCycleCount, float *batBmsTemp)
{
    *batSoc = framConfig.solarSystem.battery.batSoc;
    *batSOH = framConfig.solarSystem.battery.batSOH;
    *batteryCycleCount = framConfig.solarSystem.battery.batteryCycleCount;
    *batBmsTemp = framConfig.solarSystem.battery.batBmsTemp;
}

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
    framConfig.lastUpdateEnergy = time(NULL);
    saveBlower();
}

void BlowerClass::getEnergy(uint16_t *batChgAHToday, uint16_t *batDischgAHToday, uint16_t *batChgAHTotal,
                             uint16_t *batDischgAHTotal, float *generateEnergyToday, float *usedEnergyToday,
                             float *gLoadConsumLineTotal, float *batChgkWhToday, float *batDischgkWhToday,
                             float *genLoadConsumToday)
{
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

void BlowerClass::getSensors(float *DO, float *PH, float *WTemp, float *ATemp, float *AHum)
{
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
    framConfig.lastUpdateSensors = time(NULL);
    saveBlower();
}   
#endif