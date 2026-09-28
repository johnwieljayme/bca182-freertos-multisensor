# Static Analysis

Command: `pio check -e bluepill_f103c8`

Result on 2026-09-29: cppcheck completed successfully with 0 high, 0 medium,
and 18 low-severity style findings.

The PlatformIO check runs cppcheck per source file. The configuration
suppresses `unusedFunction`, because exported task/driver functions are called
from other translation units and cppcheck cannot see those call sites in its
per-file pass. The remaining low findings are C-style-cast reports associated
with CMSIS/HAL register and pointer expressions in the DHT22, display, startup,
and UART code. There are no high- or medium-severity findings in this run.

The finding set should be regenerated and reviewed after any later edits with
the same command. Do not treat the suppression as evidence that an unused
function is referenced; confirm application task and driver wiring in source.
