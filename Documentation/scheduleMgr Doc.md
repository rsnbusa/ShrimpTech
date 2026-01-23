Main Purpose
Orchestrates automated blower operation by processing production profiles stored in configuration, creating timed start/stop events throughout multiple production cycles.
Key Operations
1. Schedule Recovery
* Calls find_cycle_day() to determine where to resume after power loss/reboot
* Uses flash-stored theConf.dayCycle counter to maintain position across reboots
2. Triple-Nested Loop Processing
* Outer loop: Iterates through production cycles (ck)
* Middle loop: Iterates through days within each cycle (ck_d)
* Inner loop: Processes hourly schedules ("horarios") for each day (ck_h)
3. Timer Creation
* For each horario, creates two FreeRTOS timers:
    * Start timer: Triggers blower_start() callback at scheduled time
    * End timer: Triggers blower_end() callback when period completes
* Handles past schedules that started before current time but haven't ended yet
4. Day Transitions
* Waits for midnight (handle_day_end())
* Increments theConf.dayCycle and saves to flash for recovery
* Cleans up completed timers
5. Synchronization
* Waits on workTaskSem semaphore before starting
* Uses pausef flag for external pause control
* Updates blower schedule status throughout execution
6. Completion
* When all cycles complete, sets status to BLOWEROFF
* Resets dayCycle counter to 0
* Waits for next activation
This task essentially converts profile configuration data into real-time timer events for automated aquaculture system control.
