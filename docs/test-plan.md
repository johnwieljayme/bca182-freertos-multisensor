# Verification Plan

This document records verification results from the completed BCA182
FreeRTOS Multisensor project. Results are based on the current firmware
build and Wokwi simulation runs.

## Automated Checks

| Check | Command | Result |
|---|---|---|
| Firmware build | `pio run -e bluepill_f103c8` | PASS |
| Native decision tests | `pio test -e native` | PASS, 17/17 tests |
| Static analysis | `pio check -e bluepill_f103c8` | PASS, 0 high, 0 medium, 18 low style findings |
| JSON syntax | Parse `diagram.json` | PASS |

## Wokwi Functional Verification

The following functional tests were performed using the Wokwi simulation
with the completed FreeRTOS multisensor firmware.

| Test ID | Input / Stimulus | Expected Result | Actual Observed Result | Result |
|---|---|---|---|---|
| FT-01 | Set DHT22 temperature to 25°C | Temperature value updates | Serial Monitor showed `Temperature: 25.00 C` and the OLED displayed the corresponding temperature | PASS |
| FT-02 | Change DHT22 humidity | Humidity value updates | Serial Monitor showed `Humidity: 63.50 %` and the OLED displayed the corresponding humidity | PASS |
| FT-03 | Change LDR brightness | Light percentage changes | Light reading changed according to the LDR input, including a verified 4% reading | PASS |
| FT-04 | Rotate encoder clockwise | Display selection advances and wraps | Selection changed HUMIDITY → LIGHT → MOTION → TEMPERATURE with wraparound | PASS |
| FT-05 | Rotate encoder counterclockwise | Display selection moves in reverse and wraps | Selection changed in the reverse order, including wraparound to the previous measurement | PASS |
| FT-06 | Set DHT22 temperature above 30°C | HIGH alarm and buzzer activate | At 31°C, Serial Monitor showed `ALARM: temperature HIGH` and `ALARM: buzzer on`; temperature showed 31.00°C | PASS |
| FT-07 | Return temperature to normal range | Alarm and buzzer deactivate | Temperature returned to 20.30°C, alarm returned to NORMAL, and buzzer turned off | PASS |
| FT-08 | Trigger PIR while ACTIVE | Motion is detected and system remains ACTIVE | Serial Monitor showed `MOTION: detected`, system remained ACTIVE, and Motion displayed Yes | PASS |
| FT-09 | Leave PIR clear for 15 seconds | System enters INACTIVE | Serial Monitor showed `MOTION: clear` followed by `STATE: INACTIVE (no motion for 15 s)` | PASS |
| FT-10 | Trigger PIR while INACTIVE | System returns to ACTIVE | PIR motion changed the system from INACTIVE back to ACTIVE and motion was detected | PASS |

## Native Unit Test Coverage

The project contains 17 native unit tests covering alarm logic,
display navigation, rotary encoder logic, and system-state transitions.

| Test Group | Test Cases | Result |
|---|---:|---|
| Alarm logic | 5 | PASS |
| Display navigation | 4 | PASS |
| Input / encoder logic | 4 | PASS |
| System state | 4 | PASS |
| **Total** | **17** | **17/17 PASS** |

## Fault Experiments

Three deliberate fault experiments were performed and then restored.

### Fault Experiment 1: Removed InputTask Periodic Delay

The `vTaskDelayUntil()` call in `InputTask` was temporarily removed.

Observed behavior included repeated button events, frequent mode changes,
and disruption of normal OLED/task behavior. The experiment demonstrated
the importance of periodic task blocking.

**Result: PASS**

The original `vTaskDelayUntil()` implementation was restored.

### Fault Experiment 2: Increased InputTask Priority

The priority of `InputTask` was temporarily increased from priority 3 to
priority 4.

The system continued operating normally because the task still blocked
periodically using `vTaskDelayUntil()`.

**Result: PASS**

The original priority was restored.

### Fault Experiment 3: Removed Serial Logging Mutex

The serial logging mutex was temporarily removed.

During the experiment, normal startup/output progression was disrupted and
the expected sequence of task startup messages did not complete.

**Result: PASS**

The original mutex-protected serial logging implementation was restored.

## Final Verification Status

The completed project passed:

- Firmware compilation
- 17 native unit tests
- Static analysis with 0 high-severity and 0 medium-severity findings
- Wokwi functional tests FT-01 through FT-10
- Three deliberate fault experiments

All temporary fault modifications were restored after testing.