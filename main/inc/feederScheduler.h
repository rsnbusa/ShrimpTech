#ifndef FEEDER_SCHEDULER_H
#define FEEDER_SCHEDULER_H

void feeder_scheduler_init(void);
void feeder_scheduler_start_task(void);
void feeder_scheduler_reset_task(void);
void feeder_scheduler_kick(void);
void show_feeder_timers(void);

#endif // FEEDER_SCHEDULER_H
