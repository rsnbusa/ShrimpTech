#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "feederScheduler.h"

typedef struct {
    uint8_t cycle;
    uint8_t day;
    uint8_t horario;
    uint8_t weight;
    uint8_t num_lines_cfg;
    uint16_t grams_per_liter;
    uint16_t feeder_flow;
    bool isLastScheduled;
} feeder_timer_ctx_t;

static SemaphoreHandle_t feedWorkTaskSem = NULL;
static SemaphoreHandle_t feedDaySem = NULL;
static TaskHandle_t feedScheduleHandle = NULL;
static esp_timer_handle_t feeder_timers[MAXHORARIOS];
static feeder_timer_ctx_t *feeder_ctx_timers[MAXHORARIOS];

static void dump_active_feed_profile(void)
{
    return;
    if (theConf.activeProfile >= MAXPROFILES) {
        MESP_LOGW(TAG, "feedprofile dump skipped: activeProfile out of range (%u)",
                  (unsigned int)theConf.activeProfile);
        return;
    }

    const feedprofile_t *p = &theConf.feedprofiles[theConf.activeProfile];
    MESP_LOGI(TAG,
              "feedprofile dump: activeProfile=%u name=%s version=%s numCycles=%u dayCycle=%u feeder numlines=%d gramsliter=%d feederFlow=%u",
              (unsigned int)theConf.activeProfile,
              p->name,
              p->version,
              (unsigned int)p->numCycles,
              (unsigned int)theConf.dayCycle,
              theConf.feederData.numlines,
              theConf.feederData.gramsliter,
              (unsigned int)theConf.feederFlow);

    for (int line = 0; line < theConf.feederData.numlines && line < 6; line++) {
        MESP_LOGI(TAG,
                  "feedprofile line[%d]: number=%d length_m=%d l_c_t=%d",
                  line,
                  theConf.feederData.lines[line].linenum,
                  theConf.feederData.lines[line].length_m,
                  theConf.feederData.lines[line].l_c_t);
    }

    for (uint8_t c = 0; c < p->numCycles && c < MAXCICLOS; c++) {
        const ciclo_t *cycle = &p->cycle[c];
        MESP_LOGI(TAG,
                  "feedprofile cycle[%u]: day=%u duration=%lu numHorarios=%u",
                  (unsigned int)c,
                  (unsigned int)cycle->day,
                  (unsigned long)cycle->duration,
                  (unsigned int)cycle->numHorarios);

        for (uint8_t h = 0; h < cycle->numHorarios && h < MAXHORARIOS; h++) {
            const horario_t *hr = &cycle->horarios[h];
            MESP_LOGI(TAG,
                      "feedprofile cycle[%u].horario[%u]: %02u:%02u weight=%u horarioLen=%lu",
                      (unsigned int)c,
                      (unsigned int)h,
                      (unsigned int)hr->hourStart,
                      (unsigned int)hr->minutesStart,
                      (unsigned int)hr->weight,
                      (unsigned long)hr->horarioLen);
        }
    }
}

static time_t feeder_get_today_midnight(void)
{
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    tm_info->tm_hour = 0;
    tm_info->tm_min = 0;
    tm_info->tm_sec = 0;

    return mktime(tm_info);
}

static void feeder_find_cycle_day(const profile_t *profile, uint8_t *cycleOut, uint8_t *dayOut)
{
    if (!profile || !cycleOut || !dayOut) {
        return;
    }

    for (int i = 0; i < profile->numCycles; i++) {
        const ciclo_t *cycle = &profile->cycle[i];
        const int cycle_end_day = cycle->day + cycle->duration;
        if (cycle_end_day > theConf.dayCycle) {
            *dayOut = theConf.dayCycle - (cycle->day - 1);
            *cycleOut = (uint8_t)i;
            return;
        }
    }

    *cycleOut = 0;
    *dayOut = 0;
}

static void cleanup_feeder_timers(int howmany)
{
    for (int i = 0; i < howmany; i++) {
        if (feeder_timers[i]) {
            esp_timer_stop(feeder_timers[i]);
            esp_timer_delete(feeder_timers[i]);
            feeder_timers[i] = NULL;
        }
        if (feeder_ctx_timers[i]) {
            free(feeder_ctx_timers[i]);
            feeder_ctx_timers[i] = NULL;
        }
    }
}

static void feed_now(const feeder_timer_ctx_t *ctx)
{
    if (!ctx) {
        MESP_LOGW(TAG, "feed_now skipped: null context");
        return;
    }

    const int num_lines_cfg = (int)ctx->num_lines_cfg;
    const int grams_per_liter = (int)ctx->grams_per_liter;
    const int feeder_flow = (int)ctx->feeder_flow;
    const uint8_t weight = ctx->weight;

    if (num_lines_cfg <= 0 || grams_per_liter <= 0 || feeder_flow <= 0) {
        MESP_LOGW(TAG,
                 "feed_now skipped: invalid feeder config numlines=%d gramsliter=%d feederFlow=%d",
                 num_lines_cfg, grams_per_liter, feeder_flow);
        return;
    }

    const uint32_t denominator = (uint32_t)grams_per_liter * (uint32_t)feeder_flow * (uint32_t)num_lines_cfg;
    const uint32_t dispense_ms = ((uint32_t)weight * 1000U * 60U) / denominator*1000U;  // Convert to ms
    const int num_lines = (num_lines_cfg > 4) ? 4 : num_lines_cfg;

    if ((theConf.debug_flags >> dSCH) & 1U) {
        MESP_LOGI(TAG,
                 "%sfeed_now start weight=%u dispenseMs=%lu lines=%d",
                 DBG_SCH, weight, (unsigned long)dispense_ms, num_lines);
    }

    // get current wight of the Hopper
    float hopper_weight_start = hxweight;
    float hooper_weight_end=0.0f;
// cycle is Open Valve, Start Pump, Open Feeder (feeding), Close Feeder, Clear Line, Close Valve, Shut down Pump
    for (int x = 0; x < num_lines; x++) {
        const uint32_t line_clear_ms = (uint32_t)theConf.feederData.lines[x].l_c_t * 100U;

        line_valves[x].open();
        MESP_LOGW(TAG, "%sStarting Pump", DBG_SCH);

        // start_vfd_blower(true);  // Dummy feeder VFD start call for now
        feeder_valve.open();
        if (dispense_ms > 0) {
            vTaskDelay(pdMS_TO_TICKS(dispense_ms));
        }
        else {
            MESP_LOGW(TAG, "%sfeed_now calculated dispense_ms is 0, skipping delay", DBG_SCH);
            vTaskDelay(pdMS_TO_TICKS(3000));  // Default to 3 seconds if calculation is invalid
        }
        feeder_valve.close();

        if (line_clear_ms > 0) {
            MESP_LOGI(TAG, "Clear line %d for %lu ms", x, (unsigned long)line_clear_ms);
            vTaskDelay(pdMS_TO_TICKS(line_clear_ms));
        }
        MESP_LOGW(TAG, "%sStopping Pump", DBG_SCH);

        // start_vfd_blower(false);  // Dummy feeder VFD stop call for now
        line_valves[x].close();
        printf("\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    MESP_LOGI(TAG, "%sfeed_now completed weight=%u dispenseMs=%lu lines=%dn starting weight=%.2f ending weight=%.2f",
             DBG_SCH, weight, (unsigned long)dispense_ms, num_lines, hopper_weight_start, hooper_weight_end);
}

static void feeder_timer_fire(void *pArg)
{
    feeder_timer_ctx_t *ctx = (feeder_timer_ctx_t *)pArg;
    if (!ctx) {
        return;
    }

    time_t now = time(NULL);
    char fired_at[32] = {0};
    struct tm now_tm;
    localtime_r(&now, &now_tm);
    strftime(fired_at, sizeof(fired_at), "%Y-%m-%d %H:%M:%S", &now_tm);
    
    if ((theConf.debug_flags >> dSCH) & 1U) 
        MESP_LOGI(TAG, "%sFeeder timer Start C-%d D-%d H-%d Weight %d At %s",
             DBG_SCH, ctx->cycle, ctx->day, ctx->horario, ctx->weight, fired_at);

    feed_now(ctx);
    if (ctx->isLastScheduled && feedDaySem) {
        xSemaphoreGive(feedDaySem);
    }
}

static feeder_timer_ctx_t *create_feeder_timer_context(uint8_t cycle, uint8_t day, int horario)
{
    feeder_timer_ctx_t *ctx = (feeder_timer_ctx_t *)calloc(1, sizeof(feeder_timer_ctx_t));
    if (!ctx) {
        MESP_LOGE(TAG, "No ram for feeder timer context");
        return NULL;
    }

    ctx->cycle = cycle;
    ctx->day = day;
    ctx->horario = (uint8_t)horario;
    ctx->weight = theConf.feedprofiles[theConf.activeProfile].cycle[cycle].horarios[horario].weight;
    ctx->num_lines_cfg = (uint8_t)theConf.feederData.numlines;
    ctx->grams_per_liter = (uint16_t)theConf.feederData.gramsliter;
    ctx->feeder_flow = theConf.feederFlow;
    ctx->isLastScheduled = false;

    return ctx;
}

static bool make_feeder_timers(uint8_t cycle, uint8_t day, int cuantos)
{
    if (cuantos <= 0 || cuantos >= MAXHORARIOS) {
        MESP_LOGE(TAG, "Invalid feeder horarios count %d", cuantos);
        return false;
    }

    for (int i = 0; i < cuantos; i++) {

        
        feeder_ctx_timers[i] = create_feeder_timer_context(cycle, day, i);
        if (!feeder_ctx_timers[i]) {
            cleanup_feeder_timers(i);
            return false;
        }

        const horario_t *feedHorario = &theConf.feedprofiles[theConf.activeProfile]
            .cycle[cycle].horarios[i];

        esp_timer_create_args_t feed_timer;
        memset(&feed_timer, 0, sizeof(feed_timer));
        feed_timer.callback = &feeder_timer_fire;
        feed_timer.arg = (void *)feeder_ctx_timers[i];
        feed_timer.name = "TFEED";
        feed_timer.dispatch_method = ESP_TIMER_TASK;
        feed_timer.skip_unhandled_events = false;

        esp_timer_create(&feed_timer, &feeder_timers[i]);
        if (!feeder_timers[i]) {
            MESP_LOGE(TAG, "Failed to create feeder timer for cycle %d day %d hour %d", cycle, day, i);
            cleanup_feeder_timers(i + 1);
            return false;
        }
    if ((theConf.debug_flags >> dSCH) & 1U) 
        MESP_LOGI(TAG, "%sFeeder timer created id=%d start=%02d:%02d weight=%d",DBG_SCH,
               i,
               feedHorario->hourStart,
               feedHorario->minutesStart,
               feeder_ctx_timers[i]->weight);
    }

    return true;
}

static bool start_feeder_timer_for_horario(time_t midnight, time_t now, int horario)
{
    const feeder_timer_ctx_t *ctx = feeder_ctx_timers[horario];
    const horario_t *feedHorario = &theConf.feedprofiles[theConf.activeProfile]
        .cycle[ctx->cycle].horarios[horario];

    const time_t starttime = midnight + (feedHorario->hourStart * 3600) + (feedHorario->minutesStart * 60);
    if (starttime <= now) {
        return false;
    }

    const uint64_t start_delay = ((starttime - now) * TIMERUNITS) / theConf.test_timer_div;
    if (start_delay < 10) {
        MESP_LOGW(TAG, "%sFeeder timer delay too short for cycle %d day %d hour %d", DBG_SCH,
                 ctx->cycle, ctx->day, horario);
        return false;
    }

    ESP_ERROR_CHECK(esp_timer_start_once(feeder_timers[horario], start_delay));

    if ((theConf.debug_flags >> dSCH) & 1U) {
        char fire_time[32] = {0};
        struct tm fire_tm;
        localtime_r(&starttime, &fire_tm);
        strftime(fire_time, sizeof(fire_time), "%Y-%m-%d %H:%M:%S", &fire_tm);

        // MESP_LOGI(TAG, "%sFeeder timer scheduled C-%d D-%d H-%d Weight %d Delay(us) %llu FireAt %s",
        //          DBG_SCH, ctx->cycle, ctx->day, horario, ctx->weight, start_delay, fire_time);
    }
    return true;
}

static void start_feeder_schedule_timers(void *pArg)
{
    (void)pArg;

    while (true) {
        if (!feedWorkTaskSem || !feedDaySem) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        if (!xSemaphoreTake(feedWorkTaskSem, portMAX_DELAY)) {
            MESP_LOGE(TAG, "ERROR taking feedWorkTaskSem in feeder scheduler");
            continue;
        }

        if (theConf.activeProfile >= MAXPROFILES) {
            MESP_LOGE(TAG, "Invalid active profile for feeder scheduler: %d", theConf.activeProfile);
            continue;
        }

        const profile_t *activeFeedProfile = (const profile_t *)&theConf.feedprofiles[theConf.activeProfile];
        if (activeFeedProfile->numCycles == 0) {
            MESP_LOGW(TAG, "No feed cycles configured in active feeder profile %d", theConf.activeProfile);
            continue;
        }

        if ((theConf.debug_flags >> dSCH) & 1U) {
            dump_active_feed_profile();
        }

        uint8_t cycleStart = 0;
        uint8_t dayStart = 0;
        feeder_find_cycle_day(activeFeedProfile, &cycleStart, &dayStart);

        for (int ck = cycleStart; ck < activeFeedProfile->numCycles; ck++) {
            const int dayOffset = (ck == cycleStart) ? dayStart : 0;
            const int numHorarios = activeFeedProfile->cycle[ck].numHorarios;
            if (numHorarios <= 0) {
                continue;
            }

            for (int ck_d = dayOffset; ck_d < activeFeedProfile->cycle[ck].duration; ck_d++) {
                cleanup_feeder_timers(MAXHORARIOS);

                if (!make_feeder_timers((uint8_t)ck, (uint8_t)ck_d, numHorarios)) {
                    MESP_LOGE(TAG, "Failed to create feeder timers for cycle %d day %d", ck, ck_d);
                    continue;
                }

                time_t now = time(NULL);
                const time_t midnight = feeder_get_today_midnight();

                int lastScheduled = -1;
                for (int h = 0; h < numHorarios; h++) {
                    if (start_feeder_timer_for_horario(midnight, now, h)) {
                        lastScheduled = h;
                    }
                }

                if (lastScheduled >= 0) {
                    feeder_ctx_timers[lastScheduled]->isLastScheduled = true;
                    xSemaphoreTake(feedDaySem, 0);
                    xSemaphoreTake(feedDaySem, portMAX_DELAY);
                }

                time_t done_now = time(NULL);
                char done_at[32] = {0};
                struct tm done_tm;
                localtime_r(&done_now, &done_tm);
                strftime(done_at, sizeof(done_at), "%Y-%m-%d %H:%M:%S", &done_tm);

                if ((theConf.debug_flags >> dSCH) & 1U) 
                    MESP_LOGI(TAG, "%sFeeder schedule completed all days for profile %d at %s...waiting for next day",
                    DBG_SCH, theConf.activeProfile, done_at);

                cleanup_feeder_timers(numHorarios);

                const time_t current = time(NULL);
                struct tm *timeinfo = localtime(&current);
                timeinfo->tm_hour = 0;
                timeinfo->tm_min = 0;
                timeinfo->tm_sec = 0;
                timeinfo->tm_mday++;
                const time_t nextMidnight = mktime(timeinfo);
                if (nextMidnight > current) {
                    const uint32_t waitMs = (uint32_t)((nextMidnight - current) * 1000 / theConf.test_timer_div) + 1000;
                    vTaskDelay(waitMs / portTICK_PERIOD_MS);
                }
            }
        }
    }
}

void feeder_scheduler_init(void)
{
    if (!feedWorkTaskSem) {
        feedWorkTaskSem = xSemaphoreCreateBinary();
    }
    if (!feedDaySem) {
        feedDaySem = xSemaphoreCreateBinary();
    }
}

void feeder_scheduler_start_task(void)
{
    if (feedScheduleHandle) {
        return;
    }

    xTaskCreate(&start_feeder_schedule_timers, "feedsched", 1024 * 8, NULL, 5, &feedScheduleHandle);
}

void feeder_scheduler_reset_task(void)
{
    if (feedScheduleHandle) {
        vTaskDelete(feedScheduleHandle);
        feedScheduleHandle = NULL;
    }

    feeder_scheduler_start_task();
}

void feeder_scheduler_kick(void)
{
    if (feedWorkTaskSem) {
        xSemaphoreGive(feedWorkTaskSem);
    }
}
