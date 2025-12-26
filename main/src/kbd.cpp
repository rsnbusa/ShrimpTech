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
#define DEBUG_MAX_ARGUMENTS         6
#define SECURITY_MAX_ARGUMENTS      3
#define LOG_MAX_ARGUMENTS           2

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
    ESP_ERROR_CHECK(esp_console_cmd_register(&aes_cmd));
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
        ESP_LOGE("KBD", "Prompt buffer is NULL");
        return false;
    }
    
    int written = snprintf(buffer, PROMPT_BUFFER_SIZE, "Meter%02d-%02d>", poolId, unitId);
    if(written < 0 || written >= PROMPT_BUFFER_SIZE)
    {
        ESP_LOGE("KBD", "Failed to create prompt string");
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
  dbgArg.all      = arg_str0(NULL, "all",      "on | off", "Set/Reset all cmds");
  dbgArg.end      = arg_end(DEBUG_MAX_ARGUMENTS);

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
  
  endec.key = arg_int0(NULL, "k", "AES key numeric", "AES Key");
  endec.end = arg_end(1);

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
  blowArgs.end  = arg_end(LOG_MAX_ARGUMENTS);

  // ===================================================================
  // Register Command Structures
  // ===================================================================
  
  fram_cmd = {
        .command = "fram",
        .help = "Manage Fram",
        .hint = NULL,
        .func = &cmdFram,
        .argtable = &framArg
    };

  debug_cmd = {
        .command = "debug",
        .help = "Debug flags",
        .hint = NULL,
        .func = &cmdDebug,
        .argtable = &dbgArg
    };

  meter_cmd = {
        .command = "meter",
        .help = "Show Meter",
        .hint = NULL,
        .func = &cmdMeter,
        .argtable = NULL
    };

  config_cmd = {
        .command = "config",
        .help = "Show Configuration",
        .hint = NULL,
        .func = &cmdConfig,
        .argtable = NULL
    };

  erase_cmd = {
        .command = "erase",
        .help = "Erase Configuration",
        .hint = NULL,
        .func = &cmdErase,
        .argtable = NULL
    };

  findunit_cmd = {
        .command = "find",
        .help = "Find Unit Blink",
        .hint = NULL,
        .func = &cmdFindUnit,
        .argtable = NULL
    };

  meshreset_cmd = {
        .command = "install",
        .help = "Set all units to config mode",
        .hint = NULL,
        .func = &cmdMetersreset,
        .argtable = NULL
    };


  loglevel_cmd = {
        .command = "loglevel",
        .help = "Set log level",
        .hint = NULL,
        .func = &cmdLogLevel,
        .argtable = &loglevel
    };

    resetconf_cmd = {
        .command = "resetconf",
        .help = "Reset Conf flags",
        .hint = NULL,
        .func = &cmdResetConf,
        .argtable = &resetlevel
    };

    blow_cmd = {
        .command = "blow",
        .help = "Manage Blower reading",
        .hint = NULL,
        .func = &cmdBlow,
        .argtable = &blowArgs
    };

    aes_cmd = {
        .command = "aes",
        .help = "Encrypt Decrypt",
        .hint = NULL,
        .func = &cmdEnDecrypt,
        .argtable = &endec
    };

     security_cmd = {
        .command = "login",
        .help = "Login to system",
        .hint = NULL,
        .func = &cmdSecurity,
        .argtable = &kbdsedcurity
    };

     app_cmd = {
        .command = "app",
        .help = "Set app SSID",
        .hint = NULL,
        .func = &cmdApp,
        .argtable = &appSSID
    };

     node_cmd= {
        .command = "node",
        .help = "Set node Id",
        .hint = NULL,
        .func = &cmdNode,
        .argtable = &appNode
    };

       log_cmd = {
        .command = "log",
        .help = "Log options",
        .hint = NULL,
        .func = &cmdLog,
        .argtable = &logArgs
    };

  // ===================================================================
  // Initialize Prompt and Register Commands
  // ===================================================================
  
  char *prompt = (char*)calloc(1, PROMPT_BUFFER_SIZE);
  if(!prompt)
  {
        ESP_LOGE("KBD", "Failed to allocate prompt buffer");
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
          ESP_LOGW("KBD", "Using default prompt due to creation failure");
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