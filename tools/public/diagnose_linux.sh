#!/bin/sh
# Private local diagnostics only. No ROM bytes/modules are copied or uploaded.
set -u
TASK_APPDIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
TASK_DIAG_BASE=${XDG_STATE_HOME:-${HOME}/.local/state}/nes-native-lab
mkdir -p "$TASK_DIAG_BASE" || exit 1
TASK_DIAG_DIR=$(mktemp -d "$TASK_DIAG_BASE/session-XXXXXX") || exit 1
TASK_LOG=$TASK_DIAG_DIR/launch.log
{
  printf 'NES Native Lab Linux launch diagnostics\n'
  date -u
  uname -srmo
  printf '\nPackage/build identity:\n'
  "$TASK_APPDIR/AppRun" --build-info
  printf '\nFrontend dependencies (AppDir library path):\n'
  LD_LIBRARY_PATH="$TASK_APPDIR/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" ldd "$TASK_APPDIR/usr/bin/nes_native"
  printf '\nApplication output:\n'
} > "$TASK_LOG" 2>&1
"$TASK_APPDIR/AppRun" "$@" >> "$TASK_LOG" 2>&1
TASK_EXIT=$?
printf '\nExit status: %s\n' "$TASK_EXIT" >> "$TASK_LOG"
printf 'Local diagnostic log: %s\nExit status: %s\n' "$TASK_LOG" "$TASK_EXIT"
exit "$TASK_EXIT"
