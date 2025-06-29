#!/bin/bash
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
docker build -t cp2077-coop "$SCRIPT_DIR" -f "$SCRIPT_DIR/Dockerfile"
