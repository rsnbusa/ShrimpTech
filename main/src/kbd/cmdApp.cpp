
#define GLOBAL            // important fo not remove
#include "includes.h"
#include "globals.h"
#include "forwards.h"

/**
 * @brief Command handler for changing application WiFi SSID
 * 
 * @param argc Argument count
 * @param argv Argument values
 * @return 0 on success or error
 */
int cmdApp(int argc, char **argv)
{
    // Parse command line arguments
    int nerrors = arg_parse(argc, argv, (void **)&appSSID);
    if (nerrors != 0) {
        arg_print_errors(stderr, appSSID.end, argv[0]);
        return 0;
    }

    // Check if password parameter was provided
    if (appSSID.password->count) {
        // Validate password against stored configuration
        if (strcmp(appSSID.password->sval[0], theConf.kpass) == 0) {
            // Password is correct, check if new SSID was provided
            if (appSSID.newpass->count) {
                // Update SSID in configuration
                strcpy(theConf.thessid, appSSID.newpass->sval[0]);
                MESP_LOGI(TAG,"SSID changed to [%s]", theConf.thessid);
                
                // Save to flash and restart
                write_to_flash();
                MESP_LOGI(TAG, "Restarting to apply changes...");
                esp_restart();
            } else {
                MESP_LOGE(TAG, "Error: New SSID not provided. Use -n <new_ssid>");
            }
        } else {
            MESP_LOGE(TAG, "Error: Wrong password");
        }
    } else {
        MESP_LOGE(TAG, "Error: Password required. Use -p <password>");
    }

    return 0;
}
