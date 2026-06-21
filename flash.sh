#!/usr/bin/env bash
# flash.sh - Build and flash micro-radar firmware to ESP32-S3
#
# Usage:
#   ./flash.sh                  # Auto-detect USB port and flash
#   ./flash.sh -p /dev/cu.X    # Flash to specific port
#   ./flash.sh --build-only    # Only build, don't upload

set -euo pipefail

# Project directory (where this script lives)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# PlatformIO binary path
PIO="/Users/shaivure/.platformio/penv/bin/pio"

if [[ ! -x "$PIO" ]]; then
    echo "❌ PlatformIO not found at $PIO"
    exit 1
fi

# Defaults
PORT=""
BUILD_ONLY=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        -p|--port)
            PORT="$2"
            shift 2
            ;;
        -b|--build-only)
            BUILD_ONLY=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [-p|--port PORT] [-b|--build-only] [-h|--help]"
            echo ""
            echo "Build and flash micro-radar firmware to ESP32-S3."
            echo ""
            echo "Options:"
            echo "  -p, --port PORT   Flash to specific serial port"
            echo "  -b, --build-only  Only build firmware, don't upload"
            echo "  -h, --help        Show this help"
            exit 0
            ;;
        *)
            echo "❌ Unknown option: $1"
            exit 1
            ;;
    esac
done

# Build first
echo "🔨 Building firmware..."
if ! "$PIO" run 2>&1 | tail -5; then
    echo "❌ Build failed"
    exit 1
fi

if [[ "$BUILD_ONLY" == "true" ]]; then
    echo "✅ Build complete"
    exit 0
fi

# Auto-detect port if not specified
if [[ -z "$PORT" ]]; then
    # Find USB serial ports, excluding Bluetooth
    PORTS=($(ls /dev/cu.usb* 2>/dev/null | grep -v Bluetooth || true))

    if [[ ${#PORTS[@]} -eq 0 ]]; then
        echo "❌ No USB serial device found. Check connection and cable."
        exit 1
    elif [[ ${#PORTS[@]} -eq 1 ]]; then
        PORT="${PORTS[0]}"
    else
        echo "Multiple USB serial ports detected:"
        select port in "${PORTS[@]}"; do
            if [[ -n "$port" ]]; then
                PORT="$port"
                break
            fi
        done
    fi
fi

if [[ ! -e "$PORT" ]]; then
    echo "❌ Port $PORT does not exist"
    exit 1
fi

echo "📦 Uploading to $PORT ..."
if "$PIO" run --target upload --upload-port "$PORT" 2>&1 | tail -20; then
    echo ""
    echo "✅ Flash complete!"
    echo "   Device should reboot and start the radar"
else
    echo "❌ Upload failed"
    exit 1
fi
