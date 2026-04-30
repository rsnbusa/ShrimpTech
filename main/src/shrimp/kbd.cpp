/**
 * @file kbd.cpp
 * @brief Console/keyboard interface task for ESP32 debugging and configuration
 * 
 * This module implements an interactive UART-based console using ESP-IDF's REPL
 * (Read-Eval-Print Loop) component. Provides command-line access to system
 * configuration, debugging, and diagnostics.
 * 
 * Features:
 * - Password-protected access with auto-login option
 * - Multiple command categories: FRAM, debug, meter, config, logs
 * - Dynamic prompt showing pool ID and unit ID
 * - Argument parsing for complex commands
 * 
 * Security:
 * - Optional password protection (disabled if theConf.subnode==1)
 * - Commands only registered after successful login
 * - Password management through login command
 */

#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

// ===================================================================
// Console Configuration Constants
// ===================================================================

#define PROMPT_BUFFER_SIZE          20
#define CONSOLE_MAX_ARGUMENTS       8
#define DEBUG_MAX_ARGUMENTS         8
#define SECURITY_MAX_ARGUMENTS      3
#define LOG_MAX_ARGUMENTS           2
#define CONFIG_MAX_ARGUMENTS        12

// ===================================================================
// Helper Functions
// ===================================================================

/**
 * @brief Registers multiple console commands in auto-login mode
 * @note This function registers all available commands when password is bypassed
 */
static void registerAllCommands()
{
    ESP_ERROR_CHECK(esp_console_cmd_register(&meter_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&fram_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&config_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&erase_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&loglevel_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&resetconf_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&app_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&log_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&findunit_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&meshreset_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&node_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&debug_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&blow_cmd));
}

/**
 * @brief Creates a formatted prompt string for the console
 * @param buffer Buffer to store the prompt (must be at least PROMPT_BUFFER_SIZE)
 * @param poolId Pool ID to display
 * @param unitId Unit ID to display
 * @return true if successful, false if buffer allocation failed
 */
static bool createPrompt(char *buffer, uint8_t poolId, uint8_t unitId)
{
    if(!buffer)
    {
        MESP_LOGE("KBD", "Prompt buffer is NULL");
        return false;
    }
    
    int written = snprintf(buffer, PROMPT_BUFFER_SIZE, "Meter%02d-%02d>", poolId, unitId);
    if(written < 0 || written >= PROMPT_BUFFER_SIZE)
    {
        MESP_LOGE("KBD", "Failed to create prompt string");
        return false;
    }
    
    return true;
}

void kbd(void *pArg)
{
  // ===================================================================
  // Initialize Console REPL
  // ===================================================================
  
  repl = NULL;
  repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
  repl_config.prompt = (char*)"Login>";
  esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();

  // ===================================================================
  // Define Command Arguments - FRAM Management
  // ===================================================================
  
  framArg.format  = arg_str0(NULL, "format",  "slow | fast",                      "Format Fram");
  framArg.midw    = arg_str0(NULL, "wmid",    "id",                               "Set id of working meter");
  framArg.midr    = arg_int0(NULL, "rmid",    "dummy",                            "Read id of working meter");
  framArg.seed    = arg_int0(NULL, "seed",    "any value",                        "Seed values to fram for testing");
  framArg.rstart  = arg_int0(NULL, "rkwh",    "dummy",                            "Read kwh start of working meter");
  framArg.mtime   = arg_int0(NULL, "metert",  "1 write 0 read",                   "Write/read working meter date");
  framArg.mbeat   = arg_int0(NULL, "beat",    "value",                            "Write beat of working meter");
  framArg.stats   = arg_str0(NULL, "stats",   "Show stats",                       "Root node stats");
  framArg.end     = arg_end(CONSOLE_MAX_ARGUMENTS);

  // ===================================================================
  // Define Command Arguments - Debug Flags
  // ===================================================================
  
  dbgArg.schedule = arg_str0(NULL, "schedule", "on | off", "Schedule debugging");
  dbgArg.mesh     = arg_str0(NULL, "mesh",     "on | off", "Mesh debugging");
  dbgArg.ble      = arg_str0(NULL, "ble",      "on | off", "BLE debugging");
  dbgArg.mqtt     = arg_str0(NULL, "mqtt",     "on | off", "MQTT debugging");
  dbgArg.xcmds    = arg_str0(NULL, "xcmds",    "on | off", "MQTT cmds debugging");
  dbgArg.blow     = arg_str0(NULL, "blow",     "on | off", "Show Blower data");
  dbgArg.logic    = arg_str0(NULL, "logic",    "on | off", "Show logic stuff");
  dbgArg.modbus   = arg_str0(NULL, "modbus",   "on | off", "Show modbus stuff");
  dbgArg.rs485    = arg_str0(NULL, "rs485",    "on | off", "Show RS485 debugging");
  dbgArg.DO       = arg_str0(NULL, "do",       "on | off", "Show Dissolved Oxygen debugging");
  dbgArg.temp     = arg_str0(NULL, "temp",     "on | off", "Show Temperature debugging");
  dbgArg.gps      = arg_str0(NULL, "gps",      "on | off", "GPS debugging");
  dbgArg.mtxrx    = arg_str0(NULL, "mtxrx",    "on | off", "Dump Modbus TX/RX raw bytes");
  dbgArg.all      = arg_str0(NULL, "all",      "on | off", "Set/Reset all cmds");
  dbgArg.end      = arg_end(DEBUG_MAX_ARGUMENTS);

  // ===================================================================
  // Define Command Arguments - Configuration Display
  // ===================================================================
  
  configArgs.sch      = arg_lit0(NULL, "sch",       "Show Schedule ");
  configArgs.meshnet  = arg_lit0(NULL, "mesh",      "Show Mesh & Network ");
  configArgs.mqtt     = arg_lit0(NULL, "mqtt",      "Show MQTT  ");
  configArgs.profile  = arg_lit0(NULL, "profile",   "Show Profile configuration");
    configArgs.feedprofile = arg_lit0(NULL, "feedprofile", "Show feeder profile configuration");
  configArgs.blow     = arg_lit0(NULL, "blow",      "Show Blower ");
  configArgs.modbus   = arg_lit0(NULL, "modbus",    "Show modbus ");
  configArgs.prod     = arg_lit0(NULL, "produc",    "Show Porduction ");
  configArgs.stats    = arg_lit0(NULL, "stats",     "Show stats");
  configArgs.system   = arg_lit0(NULL, "sys",       "Show system ");
  configArgs.timers   = arg_lit0(NULL, "timers",    "Show Timers status");
  configArgs.DO       = arg_lit0(NULL, "do",        "Show DO ");
  configArgs.all      = arg_lit0(NULL, "all",       "Show all ");
  configArgs.end      = arg_end(CONFIG_MAX_ARGUMENTS);

  // ===================================================================
  // Define Command Arguments - Log Level & Reset
  // ===================================================================
  
  loglevel.level  = arg_str0(NULL, "l", "None-Error-Warn-Info-Debug-Verbose", "Log Level");
  loglevel.end    = arg_end(1);
  
  resetlevel.cflags = arg_str0(NULL, "f", "All Meter Mesh Collect Send Mqtt Conf Status Sch Ble", "Reset Flags");
  resetlevel.end    = arg_end(1);

  // ===================================================================
  // Define Command Arguments - Security & Network
  // ===================================================================
  
  kbdsedcurity.password = arg_str0(NULL, "p", "password",      "Kbd Security");
  kbdsedcurity.newpass  = arg_str0(NULL, "n", "new password",  "Kbd Security");
  kbdsedcurity.nopass   = arg_str0(NULL, "a", "no password",   "Kbd Security");
  kbdsedcurity.end      = arg_end(SECURITY_MAX_ARGUMENTS);

  appSSID.password = arg_str0(NULL, "p", "password",  "SSID Change");
  appSSID.newpass  = arg_str0(NULL, "n", "new SSID",  "SSID Change");
  appSSID.nopass   = arg_str0(NULL, "l", "N/A",       "SSID Change");
  appSSID.end      = arg_end(SECURITY_MAX_ARGUMENTS);

  appNode.password = arg_str0(NULL, "p", "password",  "Node Change");
  appNode.newpass  = arg_str0(NULL, "n", "new Node",  "Node Change");
  appNode.nopass   = arg_str0(NULL, "l", "N/A",       "Node Change");
  appNode.end      = arg_end(SECURITY_MAX_ARGUMENTS);

  logArgs.show  = arg_int0(NULL, "show",  "# of lines", "Show logs");
  logArgs.erase = arg_int0(NULL, "erase", "0/1",        "Erase logs");
  logArgs.end   = arg_end(LOG_MAX_ARGUMENTS);

  blowArgs.seed = arg_str0(NULL, "seed",  "x", "Seed blower readings");
  blowArgs.init = arg_str0(NULL, "erase", "x", "Set 0 to blower readings");
  blowArgs.minute = arg_int0(NULL, "minute", "x", "Set MINUTES value for sims");
  blowArgs.modbus = arg_int0(NULL, "modbus", "x", "Set Multiplier value for modbus delay");
  blowArgs.end  = arg_end(LOG_MAX_ARGUMENTS);

  // ===================================================================
  // Register Command Structures
  // ===================================================================

  auto init_console_cmd = [](esp_console_cmd_t &cmd,
                             const char *command,
                             const char *help,
                             const char *hint,
                             esp_console_cmd_func_t func,
                             void *argtable) {
      memset(&cmd, 0, sizeof(cmd));
      cmd.command = command;
      cmd.help = help;
      cmd.hint = hint;
      cmd.func = func;
      cmd.argtable = argtable;
      cmd.func_w_context = NULL;
      cmd.context = NULL;
  };
  
  init_console_cmd(fram_cmd, "fram", "Manage Fram", NULL, &cmdFram, &framArg);
  init_console_cmd(debug_cmd, "debug", "Debug flags", NULL, &cmdDebug, &dbgArg);
  init_console_cmd(meter_cmd, "meter", "Show Meter", NULL, &cmdMeter, NULL);
  init_console_cmd(config_cmd, "config", "Show Configuration", NULL, &cmdConfig, &configArgs);
  init_console_cmd(erase_cmd, "erase", "Erase Configuration", NULL, &cmdErase, NULL);
  init_console_cmd(findunit_cmd, "find", "Find Unit Blink", NULL, &cmdFindUnit, NULL);
  init_console_cmd(meshreset_cmd, "install", "Set all units to config mode", NULL, &cmdMetersreset, NULL);
  init_console_cmd(loglevel_cmd, "loglevel", "Set log level", NULL, &cmdLogLevel, &loglevel);
  init_console_cmd(resetconf_cmd, "resetconf", "Reset Conf flags", NULL, &cmdResetConf, &resetlevel);
  init_console_cmd(blow_cmd, "blow", "Manage Blower reading", NULL, &cmdBlow, &blowArgs);
  init_console_cmd(security_cmd, "login", "Login to system", NULL, &cmdSecurity, &kbdsedcurity);
  init_console_cmd(app_cmd, "app", "Set app SSID", NULL, &cmdApp, &appSSID);
  init_console_cmd(node_cmd, "node", "Set node Id", NULL, &cmdNode, &appNode);
  init_console_cmd(log_cmd, "log", "Log options", NULL, &cmdLog, &logArgs);

  // ===================================================================
  // Initialize Prompt and Register Commands
  // ===================================================================
  
  char *prompt = (char*)calloc(1, PROMPT_BUFFER_SIZE);
  if(!prompt)
  {
        MESP_LOGE("KBD", "Failed to allocate prompt buffer");
        vTaskDelete(NULL);
        return;
    }

  // Register security command first - other commands registered after successful login
    ESP_ERROR_CHECK(esp_console_cmd_register(&security_cmd));
    
    // Auto-login mode: subnode==1 indicates auto-login is enabled
    if(theConf.subnode == 1)
    {
      loginf = true;

      // Create custom prompt with pool and unit IDs
      if(createPrompt(prompt, theConf.poolid, theConf.unitid))
      {
          repl_config.prompt = prompt;
      }
      else
      {
          MESP_LOGW("KBD", "Using default prompt due to creation failure");
          free(prompt);
          prompt = NULL;
      }
      
      // Register all commands for auto-login mode
      registerAllCommands();
    }
    else
    {
        // Clean up unused prompt buffer in password-protected mode
        free(prompt);
        prompt = NULL;
    }
    
  // Start REPL console
  ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));
  ESP_ERROR_CHECK(esp_console_start_repl(repl));
  
  // Cleanup allocated resources
  if(prompt)
  {
      free(prompt);
  }
  
  // Task complete - delete self
  vTaskDelete(NULL);
}