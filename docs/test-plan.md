# Verification Plan

Record only behavior observed from this project build. The reference
screenshots/project are not evidence for this workspace's results.

## Automated Checks

| Check | Command | Current result |
|---|---|---|
| Firmware build | `pio run -e bluepill_f103c8` | PASS, 2026-09-29 |
| Native decision tests | `pio test -e native` | PASS, 17 tests, 2026-09-29 |
| Static analysis | `pio check -e bluepill_f103c8` | PASS, 0 high, 0 medium, 18 low style findings, 2026-09-29 |
| JSON syntax | Parse `diagram.json` | PASS, 2026-09-29 |

## Wokwi Results

Results below are from the current firmware run on 2026-09-29. Mark only
behavior observed in that run; unobserved portions remain partial or pending.

| ID | Stimulus | Expected observation | Result / date |
|---|---|---|---|
| B-01 | Start simulation | Startup banner, all six task-start lines, OLED init, dashboard appears after first valid read | PASS, 2026-09-29: all six tasks started; clocks reported 8 MHz; I2C init/probe returned 0; user confirmed the OLED dashboard works. |
| OLED-01 | Start with a valid sample | Dashboard shows temperature, humidity, light, and motion together; encoder marks the selected row | PENDING: previous screenshot predates the dashboard change. |
| FT-01 | Change DHT22 temperature | Dashboard temperature value updates after the next valid read | PENDING |
| FT-02 | Change DHT22 humidity | Dashboard humidity value updates with the next sample | PARTIAL, 2026-09-29: the pre-dashboard OLED showed 40.0% and samples logged 40.00%; sensor value was not changed and the dashboard is not yet visually verified. |
| FT-03 | Change LDR brightness; select LIGHT | Relative light changes from 0-100% | PENDING |
| FT-04 | Rotate encoder clockwise | Selection marker advances TEMP -> HUMIDITY -> LIGHT -> MOTION -> TEMP while all values remain visible | PARTIAL, 2026-09-29: terminal logged HUMIDITY and LIGHT selections; dashboard marker, full cycle, and wraparound were not observed. |
| FT-05 | Rotate encoder counterclockwise | Selection marker moves in reverse and wraps while all values remain visible | PENDING |
| FT-06 | Set temperature above 30 C | HIGH alarm line and buzzer activate | PENDING |
| FT-07 | Return temperature to 18-30 C | Alarm and buzzer stop | PENDING |
| FT-08 | Trigger PIR while ACTIVE | Motion is logged; system remains ACTIVE | PENDING |
| FT-09 | Leave PIR clear for 15 s | System enters INACTIVE; OLED blanks and sampling pauses | PARTIAL, 2026-09-29: `STATE: INACTIVE (no motion for 15 s)` appeared; OLED blanking and sampling pause were not confirmed. |
| FT-10 | Trigger PIR while INACTIVE | System returns ACTIVE; OLED/sampling resume | PENDING |
| F-01 | Disconnect DHT22 data | Failed read is logged; no invalid sample is published | PENDING |
| F-02 | Disconnect OLED I2C | Other tasks continue; record observed display failure behavior | PENDING |

## Native Test Coverage

- Alarm thresholds: below 18 C, exactly 18 C, normal, exactly 30 C, above 30 C.
- Display navigation: clockwise, counterclockwise, and both wrap directions.
- Encoder: clockwise and counterclockwise falling edges, ignored rising edge,
  active-low button.
- State logic: ACTIVE without timeout, ACTIVE timeout, INACTIVE without
  motion, and motion reactivation.
