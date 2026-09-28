# Pre-build script: keep the stock (PendSV-based, hangs under Wokwi) ARM_CM3
# port out of the build, so src/port.c + include/portmacro.h are the only port.
#
#  - port.c:       filtered with a build middleware (works no matter when the
#                  kernel gets git-cloned by the library's own build script).
#  - portmacro.h:  headers can't be filtered, and the library's ARM_CM3 folder
#                  outranks include/ in the search order. platformio.ini
#                  force-includes our header (same include guard => the stock
#                  one becomes empty); we also delete the stock copy if present.

Import("env")
import os

def skip_stock_port(node):
    print("[remove_stock_port] Skipping stock port file: %s" % node.get_path())
    return None

env.AddBuildMiddleware(skip_stock_port, "*FreeRTOS-Kernel*ARM_CM3*port.c")

cm3_dir = os.path.join(
    env.subst("$PROJECT_LIBDEPS_DIR"), env.subst("$PIOENV"),
    "PlatformIO-FreeRTOS", "FreeRTOS-Kernel", "portable", "GCC", "ARM_CM3"
)
for name in ("port.c", "portmacro.h"):
    f = os.path.join(cm3_dir, name)
    if os.path.isfile(f):
        os.remove(f)
        print("[remove_stock_port] Also removed file on disk: %s" % f)