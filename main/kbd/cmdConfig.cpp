#ifndef TYPESconfig_H_
#define TYPESconfig_H_
#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"
extern void show_profiles();

char lims[][10]={"AHUM","ATEMP","WTEMP","LIMITPH","LIMITDO","GENLCT","BMDDKW","BMCHKW","LCONLI","USEDEN","GENEER",
"BMDDKT","BMCHKT","BMDDAH","BMCHAH","BMTEMP","BMCC","BMSOH","BMSOC","PV1A","PV1V","PV2A","PV2V"};

char modb_names[][30]={
 "ChargeState","PV1Volts","PV1AMps","PV2Volts","PV2AMps",
"SOC","SOH","CycleCount","BatTemp",
"DO","PH","WTemp","A Temp","Humidity",
"BatChToday","BatDscToday","BatChgTotal","BatDscTotal","GenToday","UsedToday","LoadUsedTotal","BatChdToday","BatDscToday","LoadUsedToday","BatTemp"
};

void show_mimodbus()
{
  char aca[100];

    printf("\t\t================== Modbus Configuration ======================\n\n");
    sprintf(aca,"\t\t========== PV Panels Addr:[%02d] Refresh %d ========\n",theConf.modbus_panels.PVAddress,theConf.modbus_panels.refresh_rate);
    int len=strlen(aca);
    printf(aca);
    printf("\t\tName\t\tFCode\tStart\tPoints\tMux\n");
    bzero(aca, sizeof(aca));
    memset(aca,'=',len-3);  // for the 2 \t and the \n
    printf("\t\t%s\n",aca);
    printf("\t\t%12s\t%4d\t%04X\t%4d\t%.02f\n",modb_names[0],theConf.modbus_panels.Charge_State,theConf.modbus_panels.ChargeStart,theConf.modbus_panels.ChargePoints,theConf.modbus_panels.ChargeMux);
    printf("\t\t%12s\t%4d\t%04X\t%4d\t%.02f\n",modb_names[1],theConf.modbus_panels.PV1Volts,theConf.modbus_panels.PV1VStart,theConf.modbus_panels.PV1VPoints,theConf.modbus_panels.PV1VMux);
    printf("\t\t%12s\t%4d\t%04X\t%4d\t%.02f\n",modb_names[2],theConf.modbus_panels.PV2Volts,theConf.modbus_panels.PV2VStart,theConf.modbus_panels.PV2VPoints,theConf.modbus_panels.PV2VMux);
    printf("\t\t%12s\t%4d\t%04X\t%4d\t%.02f\n",modb_names[3],theConf.modbus_panels.PV1_Amps,theConf.modbus_panels.PV1AmpsStart,theConf.modbus_panels.PV1AmpsPoints,theConf.modbus_panels.PV1AmpsMux);
    printf("\t\t%12s\t%4d\t%04X\t%4d\t%.02f\n",modb_names[4],theConf.modbus_panels.PV2Amps,theConf.modbus_panels.PV2AmpsStart,theConf.modbus_panels.PV2AmpsPoints,theConf.modbus_panels.PV2AmpsMux);
    printf("\n\n"); 

    sprintf(aca,"\t\t======= Battery Addr:[%02d] Refresh %d ========\n",theConf.modbus_battery.batAddress,theConf.modbus_battery.refresh_rate);
    len=strlen(aca);
    printf(aca);
    printf("\t\tName\t\tFCode\tStart\tPoints\tMux\n");
    bzero(aca, sizeof(aca));
    memset(aca,'=',len-3);
    printf("\t\t%s\n",aca);
    printf("\t\t%12s\t%4d\t%04X\t%4d\t%.02f\n",modb_names[5],theConf.modbus_battery.SOCfc,theConf.modbus_battery.SOCStart,theConf.modbus_battery.SOCPoints,theConf.modbus_battery.SOCMux);
    printf("\t\t%12s\t%4d\t%04X\t%4d\t%.02f\n",modb_names[6],theConf.modbus_battery.SOHfc,theConf.modbus_battery.SOHStart,theConf.modbus_battery.SOHPoints,theConf.modbus_battery.SOHMux);
    printf("\t\t%12s\t%4d\t%04X\t%4d\t%.02f\n",modb_names[7],theConf.modbus_battery.cyclefc,theConf.modbus_battery.cycleStart,theConf.modbus_battery.cyclePoints,theConf.modbus_battery.cycleMux);
    printf("\t\t%12s\t%4d\t%04X\t%4d\t%.02f\n",modb_names[8],theConf.modbus_battery.tempfc,theConf.modbus_battery.tempStart,theConf.modbus_battery.tempPoints,theConf.modbus_battery.tempMux);
    printf("\n\n"); 

    sprintf(aca,"\t\t============== Sensors  Refresh %d =========================\n",theConf.modbus_sensors.refresh_rate);
    len=strlen(aca);
    printf(aca);
    printf("\t\tName\t\t\tAddr\tFCode\tStart\tPoints\tMux\n");
    bzero(aca, sizeof(aca));
    memset(aca,'=',len-3);
    printf("\t\t%s\n",aca);
    printf("\t\t%12s\t\t%2d\t%4d\t%04X\t%4d\t%.02f\n",modb_names[9],theConf.modbus_sensors.DOAddress,theConf.modbus_sensors.DOfc,theConf.modbus_sensors.DOStart,theConf.modbus_sensors.DOPoints,theConf.modbus_sensors.DOMux);
    printf("\t\t%12s\t\t%2d\t%4d\t%04X\t%4d\t%.02f\n",modb_names[10],theConf.modbus_sensors.PHAddress,theConf.modbus_sensors.PHfc,theConf.modbus_sensors.PHStart,theConf.modbus_sensors.PHPoints,theConf.modbus_sensors.PHMux);
    printf("\t\t%12s\t\t%2d\t%4d\t%04X\t%4d\t%.02f\n",modb_names[11],theConf.modbus_sensors.WAddress,theConf.modbus_sensors.Wfc,theConf.modbus_sensors.WStart,theConf.modbus_sensors.WPoints,theConf.modbus_sensors.WMux);
    printf("\t\t%12s\t\t%2d\t%4d\t%04X\t%4d\t%.02f\n",modb_names[12],theConf.modbus_sensors.AAddress,theConf.modbus_sensors.Afc,theConf.modbus_sensors.AStart,theConf.modbus_sensors.APoints,theConf.modbus_sensors.AMux);
    printf("\t\t%12s\t\t%2d\t%4d\t%04X\t%4d\t%.02f\n",modb_names[13],theConf.modbus_sensors.HAddress,theConf.modbus_sensors.Humfc,theConf.modbus_sensors.humStart,theConf.modbus_sensors.humPoints,theConf.modbus_sensors.humMux);
    printf("\n\n");

    sprintf(aca,"\t\t========= Inverter Addr:[%02d] Refresh %d ========\n",theConf.modbus_inverter.InverterAddress,theConf.modbus_inverter.refresh_rate);
    len=strlen(aca);
    printf(aca);
    printf("\t\tName\t\tFCode\tStart\tPoints\tMux\n");
    bzero(aca, sizeof(aca));
    memset(aca,'=',len-3);
    printf("\t\t%s\n",aca);
    printf("\t\t%12s\t%4d\t%04X\t%4d\t%.02f\n",modb_names[14],theConf.modbus_inverter.I1_BatChHoy,theConf.modbus_inverter.I1_BatChHoyStart,theConf.modbus_inverter.I1_BatChHoyPoints,theConf.modbus_inverter.I1_BatChHoyMux);
    printf("\t\t%12s\t%4d\t%04X\t%4d\t%.02f\n",modb_names[15],theConf.modbus_inverter.I2_BatDscHoy,theConf.modbus_inverter.I2_BatDscHoyStart,theConf.modbus_inverter.I2_BatDscHoyPoints,theConf.modbus_inverter.I2_BatDscHoyMux);
    printf("\t\t%12s\t%4d\t%04X\t%4d\t%.02f\n",modb_names[16],theConf.modbus_inverter.I3_BatChgTotal,theConf.modbus_inverter.I3_BatChgTotalStart,theConf.modbus_inverter.I3_BatChgTotalPoints,theConf.modbus_inverter.I3_BatChgTotalMux);
    printf("\t\t%12s\t%4d\t%04X\t%4d\t%.02f\n",modb_names[17],theConf.modbus_inverter.I4_BatDscTotal,theConf.modbus_inverter.I4_BatDscTotalStart,theConf.modbus_inverter.I4_BatDscTotalPoints,theConf.modbus_inverter.I4_BatDscTotalMux);
    printf("\t\t%12s\t%4d\t%04X\t%4d\t%.02f\n",modb_names[18],theConf.modbus_inverter.I5_GenkWhHoy,theConf.modbus_inverter.I5_GenkWhHoyStart,theConf.modbus_inverter.I5_GenkWhHoyPoints,theConf.modbus_inverter.I5_GenkWhHoyMux);
    printf("\t\t%12s\t%4d \t%04X\t%4d\t%.02f\n",modb_names[19],theConf.modbus_inverter.I6_UsedkwhHoy,theConf.modbus_inverter.I6_UsedkwhHoyStart,theConf.modbus_inverter.I6_UsedkwhHoyPoints,theConf.modbus_inverter.I6_UsedkwhHoyMux);
    printf("\t\t%12s\t%4d\t%04X\t%4d\t%.02f\n",modb_names[20],theConf.modbus_inverter.I7_LoadUsedTotal,theConf.modbus_inverter.I7_LoadUsedTotalStart,theConf.modbus_inverter.I7_LoadUsedTotalPoints,theConf.modbus_inverter.I7_LoadUsedTotalMux);
    printf("\t\t%12s\t%4d\t%04X\t%4d\t%.02f\n",modb_names[21],theConf.modbus_inverter.I8_BatChdHoy,theConf.modbus_inverter.I8_BatChdHoyStart,theConf.modbus_inverter.I8_BatChdHoyPoints,theConf.modbus_inverter.I8_BatChdHoyMux);
    printf("\t\t%12s\t%4d\t%04X\t%4d\t%.02f\n",modb_names[22],theConf.modbus_inverter.I9_BatDscHoy,theConf.modbus_inverter.I9_BatDscHoyStart,theConf.modbus_inverter.I9_BatDscHoyPoints,theConf.modbus_inverter.I9_BatDscHoyMux);
    printf("\t\t%12s\t%4d\t%04X\t%4d\t%.02f\n",modb_names[23],theConf.modbus_inverter.I10_LoadUsedHoy,theConf.modbus_inverter.I10_LoadUsedHoyStart,theConf.modbus_inverter.I10_LoadUsedHoyPoints,theConf.modbus_inverter.I10_LoadUsedHoyMux);
    printf("\t\t%12s\t%4d\t%04X\t%4d\t%.02f\n",modb_names[24],theConf.modbus_inverter.I11_BatTemp,theConf.modbus_inverter.I11_BatTempStart,theConf.modbus_inverter.I11_BatTempPoints,theConf.modbus_inverter.I11_BatTempMux);

    printf("\n\n"); 

}

void show_limits()
{

    printf("\n\t\t============== Limits ===============\n");
    printf("\t\tName\t\t   Min\t\t  Max\n");
    printf("\t\t=====================================\n");
    printf("\t\t%s\t\t%4d\t\t%4d\n",lims[0],theConf.milim.hummin,theConf.milim.hummax);
    printf("\t\t%s\t\t%4d\t\t%4d\n",lims[1],theConf.milim.atempmin,theConf.milim.atempmax);
    printf("\t\t%s\t\t%4d\t\t%4d\n",lims[2],theConf.milim.wtempmin,theConf.milim.wtempmax);
    printf("\t\t%s\t\t%4d\t\t%4d\n",lims[3],theConf.milim.phmin,theConf.milim.phmax);
    printf("\t\t%s\t\t%4d\t\t%4d\n",lims[4],theConf.milim.domin,theConf.milim.domax);
    printf("\t\t%s\t\t%4d\t\t%4d\n",lims[5],theConf.milim.kwchoymin,theConf.milim.kwchoymax);
    printf("\t\t%s\t\t%4d\t\t%4d\n",lims[6],theConf.milim.kwbatdhoymin,theConf.milim.kwbatdhoymax);
    printf("\t\t%s\t\t%4d\t\t%4d\n",lims[7],theConf.milim.kwbatchoymin,theConf.milim.kwbatchoymax);
    printf("\t\t%s\t\t%4d\t\t%4d\n",lims[8],theConf.milim.kwloadhoymin,theConf.milim.kwloadhoymax);
    printf("\t\t%s\t\t%4d\t\t%4d\n",lims[9],theConf.milim.kwctodaymin,theConf.milim.kwctodaymax);
    printf("\t\t%s\t\t%4d\t\t%4d\n",lims[10],theConf.milim.kwgtodaymin,theConf.milim.kwgtodaymax);
    printf("\t\t%s\t\t%4d\t\t%4d\n",lims[11],theConf.milim.bdATotmin,theConf.milim.bdATotmax);
    printf("\t\t%s\t\t%4d\t\t%4d\n",lims[12],theConf.milim.bcATotmin,theConf.milim.bcATotmax);
    printf("\t\t%s\t\t%4d\t\t%4d\n",lims[13],theConf.milim.bdAhoymin,theConf.milim.bcAhoymax);
    printf("\t\t%s\t\t%4d\t\t%4d\n",lims[14],theConf.milim.btempmin,theConf.milim.btempmax);
    printf("\t\t%s\t\t%4d\t\t%4d\n",lims[15],theConf.milim.bcyclemin,theConf.milim.bcyclemax);
    printf("\t\t%s\t\t%4d\t\t%4d\n",lims[16],theConf.milim.bSOHmin,theConf.milim.bSOHmax);
    printf("\t\t%s\t\t%4d\t\t%4d\n",lims[17],theConf.milim.bSOCmin,theConf.milim.bSOCmax);
    printf("\t\t%s\t\t%4d\t\t%4d\n",lims[18],theConf.milim.amin,theConf.milim.amax);
    printf("\t\t%s\t\t%4d\t\t%4d\n",lims[19],theConf.milim.vmin,theConf.milim.vmax);
    printf("\t\t%s\t\t%4d\t\t%4d\n",lims[20],theConf.milim.amin,theConf.milim.amax);
    printf("\t\t%s\t\t%4d\t\t%4d\n",lims[21],theConf.milim.vmin,theConf.milim.vmax);


    printf("\n");

}
void showconf(void *pArg)
{

  char            buf[50],buf2[50],fecha[60],myssid[20];
  time_t          lastwrite,now;
  struct tm       timeinfo;
  // portMUX_TYPE    xTimerLock = portMUX_INITIALIZER_UNLOCKED;
  TickType_t      xRemainingTime;
  int             routet;
  mesh_type_t     typ;
  char            my_mac[8]={0};
  uint8_t         min,secs;
  mesh_addr_t     bssid;
  time_t         guardDate,bootdate;

  time(&now);
  localtime_r(&now, &timeinfo);

  bzero(myssid,sizeof(myssid));


  char *tipo[]={"Idle","ROOT","NODE","LEAF","STA"};
  wifi_config_t conf;

  unsigned char mac_base[6] = {0};
  esp_efuse_mac_get_default(mac_base);
  esp_read_mac(mac_base, ESP_MAC_WIFI_STA);

  if(mesh_started)
  {
      memcpy(my_mac,mesh_netif_get_station_mac(),6);
  if(esp_wifi_get_config(WIFI_IF_STA, &conf)!=ESP_OK)
  {
    printf("Error readinmg wifi config\n");
    strcpy(myssid,(char*)conf.sta.ssid);
  }

  typ=esp_mesh_get_type();
}

  min=10;
  secs=10*2-(min*60);

  timeinfo.tm_min=min;
  timeinfo.tm_sec=secs; 
  time_t nexthour = mktime(&timeinfo);
  int faltan=nexthour-now;
  int fhora=faltan/3600;
  int fmin=faltan/60;
  int fsecs=faltan-(fmin*60);

  
  esp_mesh_get_parent_bssid(&bssid);
  const esp_app_desc_t *mip=esp_app_get_description();
if(mip)
  printf("\t\t Mesh Configuration Date %s ",ctime(&now));

  if(!theConf.ptch)
      printf("Virgin Chip\n");

uint32_t nada;    // this is compiler error, it goes crazy if done directly like fram.read_fdate(uint8_t*)&guarddate)

    nada=theBlower.getLastUpdate();
    // fram.read_fdate((uint8_t*)&nada);				// read last saved datetime.
    guardDate=(time_t)nada;

  bootdate=(time_t)theConf.lastRebootTime;    //same compiler error

  printf("\n\t\t\t Device Stuff\n");
  printf("\t\t\t ============\n");
  printf("BootCount:%d LastReset:%d Reason:%d LogLevel:%d DownTime: %lus LastReboot %s",theConf.bootcount,theConf.lastResetCode,theConf.lastResetCode,
            theConf.loglevel,theConf.downtime,ctime((time_t*)&bootdate));
  printf("MeterConf %d Mqttf %d sendMeterf %d\n",theConf.meterconf,mqttf,sendMeterf);
  printf("Guard Date %s",ctime(&guardDate));
  // printf("Securty Check: %s\n",theConf.useSec?"Yes":"No");
  printf("Display Active: %s\n",gdispf?"Yes":"No");
  printf("App Version: %s IDF: %s Project: %s\nCompile Date %s & %s\n",mip->version,mip->idf_ver,mip->project_name,mip->date,mip->time);
  printf("APP latest sent Version %s\n",theConf.lastVersion);
  printf("Delay %s Login Time %d\n",theConf.delay_mesh?"Yes":"No",theConf.loginwait);

  printf("\n\t\t\t Production Stuff\n");
  printf("\t\t\t =============\n");

  printf("Blower Mode %d\n",theConf.blower_mode);
  printf("Debug Flags(0x%X): ",theConf.debug_flags);
      if((theConf.debug_flags >> dSCH) & 1U)   printf("Schedule ");
      if((theConf.debug_flags >> dMESH) & 1U)  printf("Mesh ");
      if((theConf.debug_flags >> dBLE) & 1U)   printf("Ble ");
      if((theConf.debug_flags >> dMQTT) & 1U)  printf("Mqtt ");
      if((theConf.debug_flags >> dXCMDS) & 1U) printf("Xcmds ");
      if((theConf.debug_flags >> dBLOW) & 1U)  printf("Blower ");
      if((theConf.debug_flags >> dLOGIC) & 1U) printf("Logic ");
  printf("\n");
  
  printf("Active Profile %d Start Day %d%\n",theConf.activeProfile,theConf.dayCycle);
  printf("Is Master %S\n",theConf.masternode?"Yes":"No");
  if(theConf.blower_mode)
    printf("Cycle %d Day %d Mux %d\n",ck,ck_d,theConf.test_timer_div);
  else
    printf("Waiting for Production Cycle start\n");

  printf("\n\t\t\t Mqtt Topics\n");
  printf("\t\t\t ===========\n");
  printf("Command Topic \t\t%s\n",cmdQueue);
  printf("Info Topic \t\t%s\n",infoQueue);
  printf("Alarm Topic \t\t%s\n",alarmQueue);
  // printf("Install Topic \t\t%s\n",installQueue);
  printf("Mqtt server [%s] pass [%s] user [%s]\n",theConf.mqttServer,theConf.mqttPass,theConf.mqttUser);


  // is mesh active?
  if(meshf)
  {
      
  printf("\n\t\t\t Network Stuff\n");
  printf("\t\t\t =============\n");
  printf("Mesh Id:"MACSTR" SubNode:%d Pool Id:%d Unit Id:%d\n",MAC2STR(MESH_ID),theConf.subnode,theConf.poolid,theConf.unitid);
  printf("[Sta:[%s]-Psw:[%s]] ",conf.sta.ssid,conf.sta.password);
  printf("[FlashConfig_Sta:[%s] & Psw:[%s]]\n",theConf.thessid,theConf.thepass);
  printf("Parent BSSID " MACSTR "\n",MAC2STR(bssid.addr));  
  esp_wifi_get_config(WIFI_IF_AP, &conf);
  printf("AP:[%s]-Pswd:[%s]\n",conf.ap.ssid,conf.ap.password);

  mesh_addr_t mmeshid;
  esp_mesh_get_id(&mmeshid);
  printf("\n\t\t\t MESH Stuff\n");
  printf("\t\t\t ==========\n");
  printf("NodeType:%s MAC:" MACSTR ", MeshID<"MACSTR">\n", tipo[typ],MAC2STR(mac_base),MAC2STR(mmeshid.addr));
  printf("Device state %s\n",esp_mesh_is_device_active?"Up":"Down");
  printf("\n");
  printf("\n\t\t\t Timing Stuff\n");
  printf("\t\t\t ============\n");

  if(esp_mesh_is_root())
  {
    if(collectTimer)
      if(xTimerIsTimerActive(collectTimer))
      {
        xRemainingTime = xTimerGetExpiryTime( collectTimer ) - xTaskGetTickCount();
      }
    printf("Next SendData %dms BaseTime %d Repeat Timer %d\n",xRemainingTime,theConf.baset,theConf.repeat);

  }

      esp_err_t err=esp_mesh_get_routing_table((mesh_addr_t *) &s_route_table,CONFIG_MESH_ROUTE_TABLE_SIZE * 6, &routet);

      if(err==ESP_OK)
      {
        printf("Mesh Network\n");
        if(routet<11)
        {
          for (int a=0;a<routet;a++)
            printf("\tMAC[%d]:" MACSTR " %s MeterId [%s] Send [%s] PWR[%s] \n",a,MAC2STR(s_route_table[a].addr),MAC_ADDR_EQUAL(s_route_table[a].addr, my_mac)?"ME":"",masterNode.theTable.meterName[a],
                masterNode.theTable.sendit[a]?"Y":"N",masterNode.theTable.onoff[a]?"Off":"On");
        }
        else
          printf("There are %d nodes\n",routet);
      }
  }
    // time_t  ahora=theBlower.getStatsLastCountTS();
    printf("\n\t\t\t Statistics\n");
    printf("\t\t\t ===============\n");
    printf("BytesOut:%d\tBytesIn:%d\tMsgIn:%d\t\tMsgOut:%d\n",theBlower.getStatsBytesOut(),theBlower.getStatsBytesIn(),theBlower.getStatsMsgIn(),theBlower.getStatsMsgOut());
    printf("STA Conns: %d\tStaDisco: %d LastDate %s",theBlower.getStatsStaConns(),theBlower.getStatsStaDiscos(),ctime(&now));

    printf("\n\t\t\t Location Stuff\n");
    printf("\t\t\t ============\n");
    // printf("Provincia:%d Canton:%d Parroquia:%d CodigoPostal:%d\n",1,theConf.canton,theConf.parroquia,theConf.codpostal);
    printf("Expected Nodes %lu Expected Conns %lu\n",theConf.totalnodes,theConf.conns);

    print_blower("Blower",theBlower.getSolarSystem(),false);

    show_limits();
    show_mimodbus();
      vTaskDelete(NULL);
}

int cmdConfig(int argc, char **argv)
{

    xTaskCreate(&showconf,"show",4096,NULL, 10, NULL); 	        // long display allow for others to do stuff
    return 0;
}

#endif