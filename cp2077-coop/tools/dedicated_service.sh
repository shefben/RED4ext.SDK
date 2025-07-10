#!/bin/sh
# Simple helper to manage coop_dedicated as a background service.
# Usage: dedicated_service.sh start|stop [args]

DIR="$(dirname "$0")/.."
BIN="$DIR/build/coop_dedicated"
LOG_DIR="$DIR/logs/server"
PID_FILE="$LOG_DIR/coop_dedicated.pid"

case "$1" in
    start)
        mkdir -p "$LOG_DIR"
        shift
        if [ -f "$PID_FILE" ] && kill -0 $(cat "$PID_FILE") 2>/dev/null; then
            echo "Server already running"
            exit 0
        fi
        "$BIN" "$@" >> "$LOG_DIR/output.log" 2>&1 &
        echo $! > "$PID_FILE"
        echo "Server started with PID $(cat "$PID_FILE")"
        ;;
    stop)
        if [ -f "$PID_FILE" ] && kill -0 $(cat "$PID_FILE") 2>/dev/null; then
            kill $(cat "$PID_FILE")
            rm -f "$PID_FILE"
            echo "Server stopped"
        else
            echo "Server not running"
        fi
        ;;
    *)
        echo "Usage: $0 start|stop [args]"
        exit 1
        ;;
esac
