#ifndef TYPESkbd_H_
#define TYPESkbd_H_
#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

void kbd(void *pArg)
{
  repl=NULL;
  repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
  repl_config.prompt=(char*)"Login>";
  // repl_config.prompt=(char*)"LoginPls>";
  esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();

  framArg.format =                arg_str0(NULL, "format", "slow | fast", "Format Fram");
  framArg.midw =                  arg_str0(NULL, "wmid","id",  "Set id of working meter ");
  framArg.midr =                  arg_int0(NULL, "rmid","dummy",  "Read id of working meter ");
  framArg.seed =                  arg_int0(NULL, "seed", "any value", "Seed values to fram for testing");
  framArg.rstart =                arg_int0(NULL, "rkwh", "dummy", "Read kwh start of working meter ");
  framArg.mtime =                 arg_int0(NULL, "metert", "1 write 0 read", "Write/read working meter date ");
  framArg.mbeat =                 arg_int0(NULL, "beat", "value", "Write beat of working meter ");
  framArg.stats =                 arg_str0(NULL, "stats", "Show stats", "Root node stats");
  framArg.end =                   arg_end(8);

  dbgArg.schedule =               arg_str0(NULL, "schedule", "on | off", "Schedule debugging");
  dbgArg.mesh =                   arg_str0(NULL, "mesh","on | off",  "Mesh debugging");
  dbgArg.ble =                    arg_str0(NULL, "ble","on | off",  "BLE debugging ");
  dbgArg.mqtt =                   arg_str0(NULL, "mqtt","on | off",  "MQTT debugging ");
  dbgArg.xcmds =                  arg_str0(NULL, "xcmds","on | off",  "MQTT cmds debugging ");
  dbgArg.blow =                   arg_str0(NULL, "blow","on | off",  "Show Blower data ");
  dbgArg.logic =                  arg_str0(NULL, "logic","on | off",  "Show logic stuff");
  dbgArg.all =                    arg_str0(NULL, "all","on | off",  "Set/Reset all cmds ");
  dbgArg.end =                    arg_end(6);


  loglevel.level=                 arg_str0(NULL, "l", "None-Error-Warn-Info-Debug-Verbose", "Log Level");
  loglevel.end=                   arg_end(1);
  
  resetlevel.cflags=               arg_str0(NULL, "f", "All Meter Mesh Collect Send Mqtt Conf Status Sch Ble)", "Reset Flags");
  resetlevel.end=                  arg_end(1);

  endec.key=                       arg_int0(NULL, "k", "AES key numeric", "Aes Key");
  endec.end=                       arg_end(1);

  kbdsedcurity.password=           arg_str0(NULL, "p", "password", "Kbd Security");
  kbdsedcurity.newpass=            arg_str0(NULL, "n", "neww password", "Kbd Security");
  kbdsedcurity.nopass=             arg_str0(NULL, "a", "no password", "Kbd Security");
  kbdsedcurity.end=                arg_end(3);

  appSSID.password=                 arg_str0(NULL, "p", "password", "SSID Change");
  appSSID.newpass=                  arg_str0(NULL, "n", "neww SSID", "SSID Change");
  appSSID.nopass=                   arg_str0(NULL, "l", "nA", "SSID Change");
  appSSID.end=                      arg_end(3);

  appNode.password=                 arg_str0(NULL, "p", "password", "Node Change");
  appNode.newpass=                  arg_str0(NULL, "n", "new Node", "Node Change");
  appNode.nopass=                   arg_str0(NULL, "l", "nA", "Node Change");
  appNode.end=                      arg_end(3);

  logArgs.show =                    arg_int0(NULL, "show", "# of lines", "Show logs");
  logArgs.erase =                   arg_int0(NULL, "erase", "0/1" ,"Erase logs");
  logArgs.end =                     arg_end(2);

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

    unsigned char mac_base[6] = {0};
    char * prompt=(char*)calloc(1,20);

  //only register security cmd. He will register all other cmds if password ok
    ESP_ERROR_CHECK(esp_console_cmd_register(&security_cmd));
    if(theConf.subnode==1)    //subnode used to store Autologin
    {
      loginf=true;

      sprintf(prompt,"Meter%02d-%02d>",theConf.poolid,theConf.unitid);
      // sprintf(prompt,"Meter%02d-%s>",theConf.controllerid,theMeter.getMID());

      repl_config.prompt=prompt;
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
    }
  ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));
  ESP_ERROR_CHECK(esp_console_start_repl(repl));
  vTaskDelete(NULL);   //we're done here
}
#endif