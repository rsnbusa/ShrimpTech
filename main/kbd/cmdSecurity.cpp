#ifndef TYPESsecurity_H_
#define TYPESsecurity_H_
#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

extern void write_to_flash();
extern esp_err_t esp_console_setup_prompt(const char *prompt, esp_console_repl_com_t *repl_com);

int cmdSecurity(int argc, char **argv)
{
    char aca[4];
    int nerrors = arg_parse(argc, argv, (void **)&kbdsedcurity);
    if (nerrors != 0) {
        arg_print_errors(stderr, kbdsedcurity.end, argv[0]);
        return 0;
    }

  if (kbdsedcurity.password->count) 
  {   
    if(loginf)
    {
      ESP_LOGI(MESH_TAG,"Already logged in");
      return 0;
    }
    if(strlen(theConf.kpass)==0)
      strcpy(theConf.kpass,"csttpstt");
    if(strcmp(kbdsedcurity.password->sval[0],theConf.kpass )==0)
    {
      printf("Login successful\n");

      loginf=true;
      if (kbdsedcurity.newpass->count)
       {
          strcpy(theConf.kpass,kbdsedcurity.newpass->sval[0]);
          printf("Password change successful\n");
          write_to_flash();
       }

      // prompt change gimnastics
      esp_console_repl_t* veamos=repl;    //new structure type
      void * dale=(void*)veamos;          //make it byte oriented
      // dale+=4;                            //offset to prompt
           
    char * prompt=(char*)calloc(1,20);
    sprintf(prompt,"Meter%02d-%02d>",theConf.poolid,theConf.unitid);
    esp_console_setup_prompt(prompt,(esp_console_repl_com_t*)dale);

    ESP_ERROR_CHECK(esp_console_cmd_register(&meter_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&config_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&erase_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&loglevel_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&resetconf_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&aes_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&app_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&log_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&findunit_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&meshreset_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&fram_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&node_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&debug_cmd));

    }
    else
      printf("Incorrect password\n");
  }
  if (kbdsedcurity.nopass->count)
    {
      strcpy(aca,kbdsedcurity.nopass->sval[0]);
      int ch=(int)aca[0];
      if(ch=='Y' || ch=='y' )
      { 
        theConf.subnode=1;
        ESP_LOGI(MESH_TAG,"Autologin");
      }
      else
      {
        theConf.subnode=0;
        ESP_LOGI(MESH_TAG,"Password required");
      }
      write_to_flash();
    }
  return 0;
}
#endif