#ifndef _BlowerC_H_
#define _BlowerC_H_
#include "includes.h"
typedef struct {
    uint32_t chargeCurr;
    float pv1Volts;
    float pv2Volts;
    float pv1Amp;
    float pv2Amp;
} pvPanel_t;

typedef struct {
    uint8_t batSoc;
    uint8_t batSOH;
    uint16_t batteryCycleCount;
    float batBmsTemp;
} battery_t;

typedef struct {
    uint16_t batChgAHToday;
    uint16_t batDischgAHToday;
    uint16_t batChgAHTotal;
    uint16_t batDischgAHTotal;
    float generateEnergyToday;
    float usedEnergyToday;
    float gLoadConsumLineTotal;
    float batChgkWhToday;
    float batDischgkWhToday;
    float genLoadConsumToday;
} energy_t;

typedef struct {
    float DO; 
    float  PH;
    float  WTemp;
    float  ATemp;
    float  AHum;
} sensor_t;

typedef struct {
    pvPanel_t pvPanel;
    battery_t battery;
    energy_t  energy;
    sensor_t  sensors;
} solarSystem_t;

typedef struct {
    uint32_t msgIn, msgOut, bytesIn, bytesOut;
    uint8_t lastNodeCount, lastBlowerCount, staCons, staDisco;
    time_t lastCountTS;
} stats_t;

typedef struct {
    uint32_t centinel;
    // char mid[12];
    time_t lastUpdate;
    time_t lifeDate, creationDate;
    uint32_t framWrites;
    uint32_t framReads;
    time_t lastDate;
    time_t lastUpdatePV,lastUpdateBat,lastUpdateEnergy,lastUpdateSensors;
    stats_t theStats;
    solarSystem_t solarSystem;
} solarDef_t;

class BlowerClass final {
public:
    BlowerClass();

    void loadBlower();
    void saveBlower();
    void setMID(char *mid);
    void setReservedDate(time_t theDate);
    char *getMID();
    time_t getLastUpdate();
    time_t getLifeDate();
    time_t getReservedDate();
    solarSystem_t* getSolarSystem();
    
    int initBlower();
    void deinit();
    void format();
    void eraseBlower();
    void writeCreationDate(time_t ddate);
    time_t readCreationDate();
    void framWrites();
    uint32_t getFram_Writes();
    void framReads();
    uint32_t getFram_Reads();

    void setPVPanel(uint8_t chargeCurr, float pv1Volts, float pv2Volts, float pv1Amp, float pv2Amp);
    void getPVPanel(uint8_t *chargeCurr, float *pv1Volts, float *pv2Volts, float *pv1Amp, float *pv2Amp);
    void setBattery(uint8_t batSoc, uint8_t batSOH, uint16_t batteryCycleCount, float batBmsTemp);
    void getBattery(uint8_t *batSoc, uint8_t *batSOH, uint16_t *batteryCycleCount, float *batBmsTemp);
    void setEnergy(uint16_t batChgAHToday, uint16_t batDischgAHToday, uint16_t batChgAHTotal,
                   uint16_t batDischgAHTotal, float generateEnergyToday, float usedEnergyToday,
                   float gLoadConsumLineTotal, float batChgkWhToday, float batDischgkWhToday,
                   float genLoadConsumToday);
    void getEnergy(uint16_t *batChgAHToday, uint16_t *batDischgAHToday, uint16_t *batChgAHTotal,
                   uint16_t *batDischgAHTotal, float *generateEnergyToday, float *usedEnergyToday,
                   float *gLoadConsumLineTotal, float *batChgkWhToday, float *batDischgkWhToday,
                   float *genLoadConsumToday);

    uint32_t getStatsMsgIn();
    uint32_t getStatsMsgOut();
    uint32_t getStatsBytesIn();
    uint32_t getStatsBytesOut();
    uint8_t getStatsLastNodeCount();
    uint8_t getStatsLastBlowerCount();
    uint8_t getStatsStaConns();
    uint8_t getStatsStaDiscos();
    time_t getStatsLastCountTS();
    void getSensors(float *DO, float *PH, float *WTemp, float *ATemp, float *AHum);;

    solarSystem_t* getPtrSolarsystem();
    void setStatsLastNodeCount(uint8_t count);
    void setStatsLastBlowerCount(uint8_t count);
    void setStatsLastCountTS(time_t now);
    void setStatsStaConns();
    void setStatsStaDiscos();
    void setStatsMsgIn();
    void setStatsMsgOut();
    void setStatsBytesIn(uint32_t count);
    void setStatsBytesOut(uint32_t count);
    void setSensors(float DO, float PH, float WTemp, float ATemp, float AHum);
    

    TaskHandle_t getMainTask();

private:
    solarDef_t framConfig;
    FramI2C fram;
    SemaphoreHandle_t framSem;
    bool framFlag;
    uint32_t blowerSize;
};

#endif