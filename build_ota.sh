#!/bin/bash
# ============================================
# Script: build_ota.sh
# Deskripsi: Build firmware PlatformIO dan
#   generate file OTA (firmware.bin + version.json)
#   siap upload ke server.
# ============================================

set -e

# --- Konfigurasi ---
OUTPUT_DIR="ota_output"
CONFIG_FILE="include/Config.h"

# --- Ambil versi dari Config.h ---
VERSION=$(grep -oP '#define FIRMWARE_VERSION\s+"\K[^"]+' "$CONFIG_FILE")
if [ -z "$VERSION" ]; then
    echo "❌ ERROR: Tidak menemukan FIRMWARE_VERSION di $CONFIG_FILE"
    exit 1
fi

echo "====================================="
echo "  OTA Build Script - Brangkas V2"
echo "  Versi: v$VERSION"
echo "====================================="

# --- Build firmware ---
echo ""
echo "📦 Menjalankan PlatformIO build..."
pio run

# --- Salin file hasil build ---
BIN_SRC=".pio/build/esp-wrover-kit/firmware.bin"
if [ ! -f "$BIN_SRC" ]; then
    echo "❌ ERROR: File $BIN_SRC tidak ditemukan!"
    exit 1
fi

mkdir -p "$OUTPUT_DIR"

BIN_NAME="brangkas_v${VERSION}.bin"
cp "$BIN_SRC" "$OUTPUT_DIR/$BIN_NAME"

echo "✅ Firmware disalin ke: $OUTPUT_DIR/$BIN_NAME"

# --- Generate version.json ---
# PENTING: Ganti BASE_URL dengan URL server Anda
BASE_URL="http://yourserver.com/brangkas"

cat > "$OUTPUT_DIR/version.json" <<EOF
{
    "version": "$VERSION",
    "url": "$BASE_URL/$BIN_NAME"
}
EOF

echo "✅ version.json dibuat di: $OUTPUT_DIR/version.json"

# --- Ringkasan ---
BIN_SIZE=$(stat --printf="%s" "$OUTPUT_DIR/$BIN_NAME" 2>/dev/null || stat -f%z "$OUTPUT_DIR/$BIN_NAME" 2>/dev/null)
BIN_SIZE_KB=$((BIN_SIZE / 1024))

echo ""
echo "====================================="
echo "  BUILD SELESAI!"
echo "====================================="
echo "  Firmware : $OUTPUT_DIR/$BIN_NAME ($BIN_SIZE_KB KB)"
echo "  Metadata : $OUTPUT_DIR/version.json"
echo ""
echo "  Upload kedua file di atas ke server Anda."
echo "  Pastikan URL di version.json sudah benar."
echo "====================================="
