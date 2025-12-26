#ifndef TYPESrconf_H_
#define TYPESrconf_H_
#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

extern void erase_config();
extern void root_collect_meter_data(TimerHandle_t  algo);

// Reset flag enumeration
typedef enum {
    RESET_ALL = 0,
    RESET_METER,
    RESET_MESH,
    RESET_COLLECT,
    RESET_SEND,
    RESET_MQTT,
    RESET_CONF,
    RESET_STATUS,
    RESET_SCH,
    RESET_BLE,
    RESET_FLAG_COUNT
} reset_flag_t;

const char* RESET_FLAG_NAMES[RESET_FLAG_COUNT] = {
    "ALL", "METER", "MESH", "COLLECT", "SEND", 
    "MQTT", "CONF", "STATUS", "SCH", "BLE"
};

/**
 * @brief Find the index of a reset flag by name
 * @param flagName The flag name to search for (case-sensitive)
 * @return The index of the flag (0-9) or -1 if not found
 */
int findFlag(char* flagName)
{
  for (int i = 0; i < RESET_FLAG_COUNT; i++)
    if(strcmp(flagName, RESET_FLAG_NAMES[i]) == 0)
      return i;
  
  return -1;
}

// ============================================================================
// Reset Handler Functions
// ============================================================================

/**
 * @brief Reset all configuration settings to factory defaults
 * Erases all stored configuration and resets meter configuration
 */
void handleResetAll()
{
  printf("All flags resetted\n");
  theConf.meterconf = 0;
  // theConf.meshconf = 0;
  erase_config();
}

/**
 * @brief Reset meter/blower configuration
 * Sets meter configuration to state 2
 */
void handleResetMeter()
{
  printf("Blower resetted\n");
  theConf.meterconf = 2;
}

/**
 * @brief Reset mesh network configuration
 * Currently disabled (commented out)
 */
void handleResetMesh()
{
  printf("Mesh resetted\n");
  // theConf.meshconf = 0;
}

/**
 * @brief Trigger data collection from meters
 * Only executes if this device is the mesh root node
 * @return 0 to indicate no restart needed
 */
int handleResetCollect()
{
  printf("Collect Data\n");
  if(esp_mesh_is_root()) { //only non ROOT 
    root_collect_meter_data(NULL);
  }
  return 0;
}

/**
 * @brief Reset the meter sending flag
 * Clears the flag that controls meter data transmission
 */
void handleResetSend()
{
  printf("Sendmeterf resetted\n");
  sendMeterf = 0;
}

/**
 * @brief Release the MQTT flag
 * Enables MQTT communication by setting the flag to true
 */
void handleResetMqtt()
{
  printf("Mqttf released\n");
  mqttf = true;
}

/**
 * @brief Enter reconfiguration mode
 * Sets meter configuration to state 3 and saves to flash
 * @return 0 to indicate restart should proceed
 */
int handleResetConf()
{
  printf("Reconfigure mode\n");
  theConf.meterconf = 3;
  write_to_flash();
  return 0;
}

/**
 * @brief Toggle the TEST divisor value
 * Switches TEST between 1 and 10 for debugging purposes
 * @return 1 to skip restart
 */
int handleResetStatus()
{
  printf("Divisor\n");
  if(TEST == 1)
    TEST = 10;
  else
    TEST = 1;
  return 1; // No restart needed
}

/**
 * @brief Start the work task schedule
 * Gives the work task semaphore to trigger scheduled operations
 * @return 1 to skip restart
 */
int handleResetSchedule()
{
  printf("Start Schedule\n");
  xSemaphoreGive(workTaskSem);
  return 1; // No restart needed
}

/**
 * @brief Reset BLE boot flag and restart
 * Clears the BLE boot flag, saves to flash, and triggers a restart
 * @return 1 to indicate restart is handled internally
 */
int handleResetBle()
{
  printf("Boot Ble\n");
  theConf.bleboot = 0;
  write_to_flash();
  esp_restart();
  return 1; // No restart needed (already restarting)
}

/**
 * @brief Console command handler for reset/reconfigure operations
 * 
 * Handles various reset and reconfiguration commands based on flag parameter.
 * Most operations will trigger a device restart after saving changes to flash.
 * 
 * @param argc Argument count
 * @param argv Argument values (expects -f <FLAG>)
 * @return 0 on success or error
 * 
 * Valid flags:
 * - ALL: Reset all configuration to factory defaults
 * - METER: Reset meter/blower configuration
 * - MESH: Reset mesh network configuration
 * - COLLECT: Trigger data collection (root node only)
 * - SEND: Reset meter sending flag
 * - MQTT: Release MQTT flag
 * - CONF: Enter reconfiguration mode
 * - STATUS: Toggle TEST divisor (1 <-> 10)
 * - SCH: Start work task schedule
 * - BLE: Reset BLE boot flag and restart
 */
int cmdResetConf(int argc, char **argv)
{
  int flagIndex;
  char flagName[10];

  int nerrors = arg_parse(argc, argv, (void **)&resetlevel);
  if (nerrors != 0) {
      arg_print_errors(stderr, resetlevel.end, argv[0]);
      return 0;
  }
  
  // Early return if no flag specified
  if (!resetlevel.cflags->count) {
    printf("No reset flag specified\n");
    printf("Usage: resetconf -f <FLAG>\n");
    printf("Valid flags: ALL, METER, MESH, COLLECT, SEND, MQTT, CONF, STATUS, SCH, BLE\n");
    return 0;
  }
  
  // Copy and convert flag name to uppercase for case-insensitive comparison
  snprintf(flagName, sizeof(flagName), "%s", resetlevel.cflags->sval[0]);
  for (int x = 0; x < strlen(flagName); x++) {
      flagName[x] = toupper(flagName[x]);
  }
  
  // Find and validate flag
  flagIndex = findFlag(flagName);
  if (flagIndex < 0) {
    printf("Unknown reset flag: %s\n", flagName);
    printf("Valid flags: ALL, METER, MESH, COLLECT, SEND, MQTT, CONF, STATUS, SCH, BLE\n");
    return 0;
  }
  
  // Execute the appropriate reset handler
  int skipRestart = 0;
      
      switch(flagIndex) {
        case RESET_ALL:
          handleResetAll();
          break;
        case RESET_METER:
          handleResetMeter();
          break;
        case RESET_MESH:
          handleResetMesh();
          break;
        case RESET_COLLECT:
          skipRestart = handleResetCollect();
          break;
        case RESET_SEND:
          handleResetSend();
          break;
        case RESET_MQTT:
          handleResetMqtt();
          break;
        case RESET_CONF:
          skipRestart = handleResetConf();
          break;
        case RESET_STATUS:
          skipRestart = handleResetStatus();
          break;
        case RESET_SCH:
          skipRestart = handleResetSchedule();
          break;
        case RESET_BLE:
          skipRestart = handleResetBle();
          break;
        default:
          printf("Wrong choice of reset\n");
          return 0;
    }
    
    // Skip restart if handler indicates so
    if (skipRestart) {
      return 0;
    }
    
    // Save configuration and restart
    write_to_flash();
    esp_restart();
    return 0;
}

#endif