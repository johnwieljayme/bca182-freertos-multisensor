# Design Notes

## Hardware and Timing

- STM32F103C8 runs from the reset HSI clock at 8 MHz; PLL is disabled.
- HAL's 1 ms tick uses TIM4. FreeRTOS owns SysTick through the project port.
- DHT22 is on PA0 and has an external 4.7 kOhm pull-up in Wokwi. Its
  microsecond transaction is enclosed in a short FreeRTOS critical section so
  the Wokwi-safe port does not switch tasks in the middle of a pulse.
- The LDR uses ADC1 channel 1 / PA1. Its reported value is relative light in
  the range 0-100%, not calibrated lux.
- OLED is on I2C1 PB6/PB7. Only DisplayTask sends OLED commands/data.
- The buzzer is driven by TIM1_CH1 on PA8 at approximately 1 kHz.

## Tasks and Priorities

| Task | Priority | Period / trigger | Reason |
|---|---:|---|---|
| MotionTask | 3 | 100 ms | Bounds motion detection and reactivation latency. |
| InputTask | 3 | 5 ms | Samples encoder edges frequently enough for responsive navigation. |
| SensorTask | 2 | 2000 ms | Matches the DHT22 minimum sample interval; uses `vTaskDelayUntil()` to keep the period stable. |
| AlarmTask | 2 | Sample queue or 500 ms timeout | Evaluates new sensor samples and notices INACTIVE promptly. |
| StateTask | 2 | 100 ms | Evaluates the inactivity timeout and state transitions. |
| DisplayTask | 1 | Queue event or 250 ms event-bit poll | OLED refresh can tolerate more latency than sensor/input response. |

Every periodic task blocks using `vTaskDelayUntil()`. AlarmTask blocks on its
queue with a bounded timeout. DisplayTask blocks on the queue set with a
bounded timeout.

## RTOS Objects

| Object | Producer / owner | Consumer(s) | Purpose |
|---|---|---|---|
| `xDisplayQueue` (length 1) | SensorTask overwrites | DisplayTask | Latest measurement for the screen; stale samples are discarded. |
| `xAlarmQueue` (length 1) | SensorTask overwrites | AlarmTask | Independent copy so the display cannot consume the alarm's sample. |
| `xModeQueue` (length 1) | InputTask overwrites | DisplayTask | Latest selected OLED page. |
| `xDisplayEvents` queue set | Contains display and mode queues | DisplayTask | Lets the display task block on either update source. |
| `xSystemEvents` | MotionTask owns MOTION; StateTask owns ACTIVE; AlarmTask owns ALARM | Sensor, input, alarm, display, state tasks | Level flags for system state, motion, and active alarm. |
| `serialMutex` | Created with the other RTOS objects | Every task calling `log_line()` | Protects USART1 so messages from different tasks cannot interleave. The mutex covers one complete line. |

Event-bit rules:

- `EVENT_ACTIVE`: set at object creation; StateTask clears after 15 s without
  motion and sets again when motion is detected.
- `EVENT_MOTION`: MotionTask mirrors the current PIR output; readers do not
  clear it.
- `EVENT_ALARM`: AlarmTask sets it while the buzzer is sounding and clears it
  when the buzzer is off.

## State Machine

ACTIVE begins at boot. The last-motion tick is refreshed whenever the PIR is
high. After 15 seconds without motion, StateTask clears `EVENT_ACTIVE` and the
system becomes INACTIVE. SensorTask skips sensor reads while INACTIVE, the
buzzer is silenced, DisplayTask blanks the OLED, and InputTask ignores mode
changes. MotionTask remains scheduled and a new motion event restores ACTIVE.

## Decisions and Limitations

- Hardware-independent rules are isolated in `*_logic.cpp` and tested with
  the native PlatformIO environment.
- The DHT22 driver may reject a read on checksum or timeout failure. SensorTask
  logs that outcome and skips the sample; consumers retain their last valid
  value.
- The state and task design has been build/unit-tested locally. Wokwi
  interaction results must be recorded separately in `test-plan.md` after
  running this project in the simulator.
