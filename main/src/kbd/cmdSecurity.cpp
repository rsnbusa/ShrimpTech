#ifndef TYPESsecurity_H_
#define TYPESsecurity_H_
#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

extern void write_to_flash();
extern esp_err_t esp_console_setup_prompt(const char *prompt, esp_console_repl_com_t *repl_com);

// Security constants
#define DEFAULT_PASSWORD "csttpstt"
#define PROMPT_BUFFER_SIZE 20
#define CHOICE_BUFFER_SIZE 4
#define MAX_PASSWORD_LENGTH 32

/**
 * @brief Register all secure console commands
 * 
 * Registers all privileged commands that require authentication.
 * These commands are only accessible after successful login.
 */
void registerSecureCommands()
{
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
    ESP_ERROR_CHECK(esp_console_cmd_register(&blow_cmd));
}

/**
 * @brief Update console prompt to show unit identification
 * 
 * Updates the REPL prompt to display pool ID and unit ID after login.
 */
void updateConsolePrompt()
{
    esp_console_repl_t* replInstance = repl;
    void* replBytes = (void*)replInstance;
    // Note: Direct offset manipulation (replBytes += 4) is unsafe and platform-dependent
    
    char* prompt = (char*)calloc(1, PROMPT_BUFFER_SIZE);
    sprintf(prompt, "Meter%02d-%02d>", theConf.poolid, theConf.unitid);
    esp_console_setup_prompt(prompt, (esp_console_repl_com_t*)replBytes);
}

/**
 * @brief Initialize password to default if not set
 */
void initializePassword()
{
    if(strlen(theConf.kpass) == 0) {
        snprintf(theConf.kpass, sizeof(theConf.kpass), "%s", DEFAULT_PASSWORD);
    }
}

/**
 * @brief Verify login credentials
 * @param password Password to verify
 * @return true if password matches, false otherwise
 */
bool verifyPassword(const char* password)
{
    return strcmp(password, theConf.kpass) == 0;
}

/**
 * @brief Handle password change operation
 * @param newPassword New password to set
 * @return true if password changed successfully, false if invalid
 */
bool handlePasswordChange(const char* newPassword)
{
    if (newPassword == NULL) {
        printf("Error: Password cannot be NULL\n");
        return false;
    }
    
    size_t passwordLen = strlen(newPassword);
    if (passwordLen == 0) {
        printf("Error: Password cannot be empty\n");
        return false;
    }
    
    if (passwordLen >= sizeof(theConf.kpass)) {
        printf("Error: Password too long (max %zu characters)\n", sizeof(theConf.kpass) - 1);
        return false;
    }
    
    snprintf(theConf.kpass, sizeof(theConf.kpass), "%s", newPassword);
    printf("Password change successful\n");
    write_to_flash();
    return true;
}

/**
 * @brief Handle successful login
 * @param newPassword Optional new password (NULL if not changing)
 */
void handleSuccessfulLogin(const char* newPassword)
{
    printf("Login successful\n");
    loginf = true;
    
    if (newPassword != NULL) {
        if (!handlePasswordChange(newPassword)) {
            printf("Warning: Password change failed, continuing with old password\n");
        }
    }
    
    updateConsolePrompt();
    registerSecureCommands();
}

/**
 * @brief Configure auto-login setting
 * @param enable true to enable auto-login, false to require password
 */
void configureAutoLogin(bool enable)
{
    theConf.subnode = enable ? 1 : 0;
    if (enable) {
        MESP_LOGI(MESH_TAG, "Autologin");
    } else {
        MESP_LOGI(MESH_TAG, "Password required");
    }
    write_to_flash();
}

/**
 * @brief Console command handler for security and authentication operations
 * 
 * Handles user authentication, password management, and auto-login configuration.
 * Provides secure access control to privileged console commands.
 * 
 * @param argc Argument count
 * @param argv Argument values
 * @return 0 on success
 * 
 * Usage:
 *   Login with password:
 *     security -p <PASSWORD>
 * 
 *   Login and change password:
 *     security -p <OLD_PASSWORD> -n <NEW_PASSWORD>
 * 
 *   Configure auto-login (bypass password):
 *     security --nopass <Y|N>
 *     Y = Enable auto-login (set subnode=1)
 *     N = Require password (set subnode=0)
 * 
 * Security features:
 *   - Default password: "csttpstt" (if not configured)
 *   - Password validation with length checks
 *   - Buffer overflow protection with snprintf
 *   - Prevents multiple simultaneous logins
 *   - Registers 14 secure commands upon successful login
 *   - Updates console prompt to show unit identification
 * 
 * Notes:
 *   - Auto-login mode reduces security - use with caution
 *   - Password is stored in theConf.kpass and persisted to flash
 *   - Login state is tracked globally via loginf flag
 */
int cmdSecurity(int argc, char **argv)
{
    char autoLoginChoice[CHOICE_BUFFER_SIZE];
    
    int nerrors = arg_parse(argc, argv, (void **)&kbdsedcurity);
    if (nerrors != 0) {
        arg_print_errors(stderr, kbdsedcurity.end, argv[0]);
        return 0;
    }

  // Handle password login and optional password change
  if (kbdsedcurity.password->count) 
  {   
    if(loginf) {
      ESP_LOGI(MESH_TAG, "Already logged in");
      return 0;
    }
    
    initializePassword();
    
    if(verifyPassword(kbdsedcurity.password->sval[0])) {
      const char* newPassword = kbdsedcurity.newpass->count ? kbdsedcurity.newpass->sval[0] : NULL;
      handleSuccessfulLogin(newPassword);
    }
    else {
      printf("Incorrect password\n");
    }
  }
  
  // Handle auto-login configuration
  if (kbdsedcurity.nopass->count) {
    snprintf(autoLoginChoice, sizeof(autoLoginChoice), "%s", kbdsedcurity.nopass->sval[0]);
    int ch = (int)autoLoginChoice[0];
    bool enableAutoLogin = (ch == 'Y' || ch == 'y');
    configureAutoLogin(enableAutoLogin);
  }
  
  return 0;
}
#endif