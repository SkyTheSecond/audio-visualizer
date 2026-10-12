#!/usr/bin/env bash
# Run a GUI program on a virtual X display for a few seconds. Passes if it is still running
# when the timeout stops it and printed no OpenGL/GLFW errors. Needs xvfb-run and Mesa.
# Usage: gui_smoke.sh <program> [args...]
set -euo pipefail

seconds=5
log="$(mktemp)"

# Containers have no session directory; without one, GLFW's Wayland probe prints a
# harmless complaint before falling back to X11.
export XDG_RUNTIME_DIR="$(mktemp -d)"

status=0
xvfb-run -a -s "-screen 0 1600x1000x24" timeout "$seconds" "$@" >"$log" 2>&1 || status=$?
cat "$log"

# timeout exits with 124 when it had to stop the program, i.e. the program was still running.
if [ "$status" -ne 124 ]; then
    echo "FAIL: exited with status $status before the ${seconds} s timeout"
    exit 1
fi
if grep -E '^\[GL (HIGH|MEDIUM)\]|GLFW error|no OpenGL|failed to|texture:' "$log"; then
    echo "FAIL: the program reported the errors above"
    exit 1
fi
echo "OK: ran ${seconds} s without OpenGL or GLFW errors"
