#!/usr/bin/env bash
#
# Flash the QCC744M EVK (BL618) with QConn_Flash_Cmd, following the Zephyr
# board docs layout (boot2 @ 0x0, zephyr.bin @ 0x2000):
#   https://docs.zephyrproject.org/latest/boards/qcom/qcc744m_evk/doc/index.html
#
# Usage:
#   ./flash.sh                 # flash the app (zephyr.bin @ 0x2000) -- default
#   ./flash.sh --boot2         # flash only boot2 (@ 0x0)
#   ./flash.sh --all           # flash boot2 (0x0) + app (0x2000)
#   ./flash.sh --port /dev/ttyACM0
#   ./flash.sh --baud 115200   # slower, if the 2M shake-hand keeps failing
#   PORT=/dev/ttyACM0 BAUDRATE=500000 ./flash.sh
#
# Put the board in download mode first: hold BOOT, tap RESET, release BOOT.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE="$(cd "$SCRIPT_DIR/.." && pwd)"          # .../zephyrproject
SDK="$WORKSPACE/QCCSDK-QCC74x-master"

# Select the platform-specific flashing binary automatically.
OS_NAME="$(uname -s 2>/dev/null || echo unknown)"
ARCH_NAME="$(uname -m 2>/dev/null || echo unknown)"
case "$OS_NAME" in
Darwin)
	TOOL_SUFFIX="-macos"
	;;
Linux)
	case "$ARCH_NAME" in
	aarch64|arm64|armv7*|armv6*) TOOL_SUFFIX="-arm" ;;
	*)                           TOOL_SUFFIX="-ubuntu" ;;
	esac
	;;
MINGW*|MSYS*|CYGWIN*|Windows_NT)
	TOOL_SUFFIX=".exe"
	;;
*)
	echo "unsupported OS: $OS_NAME (arch: $ARCH_NAME)" >&2
	echo "set TOOL=/absolute/path/to/QConn_Flash_Cmd manually" >&2
	exit 1
	;;
esac
TOOL="${TOOL:-$SCRIPT_DIR/QConn_Flash/QConn_Flash_Cmd$TOOL_SUFFIX}"
BOOT2="$SCRIPT_DIR/boot2_config/boot2_qcc743_isp_release_v8.1.9.bin"

# Overridable via env or flags.
PORT="${PORT:-/dev/ttyACM1}"
BAUDRATE="${BAUDRATE:-2000000}"
BUILD_DIR="${BUILD_DIR:-$PWD/build}"               # ./build under the invocation dir
MODE="app"                                         # app | boot2 | all

while [ $# -gt 0 ]; do
	case "$1" in
	--app-only|--app) MODE="app" ;;
	--boot2)          MODE="boot2" ;;
	--all)            MODE="all" ;;
	--port)           PORT="$2"; shift ;;
	--baud|--baudrate) BAUDRATE="$2"; shift ;;
	--build-dir)      BUILD_DIR="$2"; shift ;;
	-h|--help)
		sed -n '3,15p' "$0"
		exit 0 ;;
	*) echo "unknown argument: $1" >&2; exit 2 ;;
	esac
	shift
done

APP_BIN="$BUILD_DIR/zephyr/zephyr.bin"

# --- sanity checks ---------------------------------------------------------
[ -f "$TOOL" ] || { echo "flash tool not found: $TOOL" >&2; exit 1; }
if [[ "$TOOL" != *.exe ]] && [ ! -x "$TOOL" ]; then
	echo "flash tool is not executable: $TOOL" >&2
	exit 1
fi
if [ "$MODE" != "boot2" ]; then
	[ -f "$APP_BIN" ] || { echo "app image not found: $APP_BIN (run 'west build' first)" >&2; exit 1; }
fi
if [ "$MODE" != "app" ]; then
	[ -f "$BOOT2" ] || { echo "boot2 image not found: $BOOT2" >&2; exit 1; }
fi
[ -e "$PORT" ] || { echo "serial port not present: $PORT (check 'ls /dev/ttyACM*')" >&2; exit 1; }

# --- generate a config with absolute paths, so cwd never matters -----------
CFG="$(mktemp -t qcc744_flash_cfg.XXXXXX.ini)"
trap 'rm -f "$CFG"' EXIT
{
	echo "[cfg]"
	echo "erase = 1"
	echo "skip_mode = 0x0, 0x0"
	echo "boot2_isp_mode = 0"
	echo ""
	if [ "$MODE" != "app" ]; then
		echo "[boot2]"
		echo "filedir = $BOOT2"
		echo "address = 0x000000"
		echo ""
	fi
	if [ "$MODE" != "boot2" ]; then
		echo "[FW]"
		echo "filedir = $APP_BIN"
		echo "address = 0x002000"
	fi
} > "$CFG"

case "$MODE" in
app)   MODE_DESC="app @0x2000 only" ;;
boot2) MODE_DESC="boot2 @0x0 only" ;;
all)   MODE_DESC="boot2 @0x0 + app @0x2000" ;;
esac

echo "Flashing QCC744M EVK"
echo "  mode     : $MODE ($MODE_DESC)"
echo "  port     : $PORT"
echo "  baudrate : $BAUDRATE"
[ "$MODE" != "app" ]   && echo "  boot2    : $BOOT2"
[ "$MODE" != "boot2" ] && echo "  app      : $APP_BIN"
echo "  (board must be in download mode: hold BOOT, tap RESET, release BOOT)"
echo ""

exec "$TOOL" \
	--interface=uart \
	--baudrate="$BAUDRATE" \
	--port="$PORT" \
	--cpu_id= \
	--config="$CFG"
