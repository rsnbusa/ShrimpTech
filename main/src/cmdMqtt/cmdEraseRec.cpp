#ifndef TYPESeraserec_H_
#define TYPESeraserec_H_
#define GLOBAL
#include "includes.h"
#include "globals.h"

extern void writeLog(const char* que);

/**
 * @brief MQTT command handler: "eraserec"
 *
 * Erases the feeder recovery record stored in FRAM by zeroing
 * the recoveryData structure and saving it back.
 *
 * Command format:
 *   {"cmd":"eraserec","unitid":X}
 *
 * No additional parameters required.
 */
int cmdEraseRec(void *argument)
{
    MESP_LOGI(MESH_TAG, "EraseRec CMD: erasing feeder recovery record");

    bzero(&recoveryData, sizeof(recoveryData));
    theBlower.saveRecovery(&recoveryData);

    writeLog("Feeder recovery record erased via MQTT command");

    return ESP_OK;
}

#endif // TYPESeraserec_H_
