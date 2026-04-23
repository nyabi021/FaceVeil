#!/usr/bin/env bash
set -euo pipefail

# Generates assets/FaceVeil.icns from assets/icon.png (must be 1024x1024).
# Re-run whenever assets/icon.png changes.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_PNG="$ROOT_DIR/assets/icon.png"
ICONSET_DIR="$ROOT_DIR/assets/FaceVeil.iconset"
OUT_ICNS="$ROOT_DIR/assets/FaceVeil.icns"

if [[ ! -f "$SRC_PNG" ]]; then
    echo "❌ Source icon not found: $SRC_PNG"
    exit 1
fi

rm -rf "$ICONSET_DIR"
mkdir -p "$ICONSET_DIR"

sizes=(
    "16 icon_16x16.png"
    "32 icon_16x16@2x.png"
    "32 icon_32x32.png"
    "64 icon_32x32@2x.png"
    "128 icon_128x128.png"
    "256 icon_128x128@2x.png"
    "256 icon_256x256.png"
    "512 icon_256x256@2x.png"
    "512 icon_512x512.png"
    "1024 icon_512x512@2x.png"
)

for entry in "${sizes[@]}"; do
    size="${entry%% *}"
    name="${entry##* }"
    sips -z "$size" "$size" "$SRC_PNG" --out "$ICONSET_DIR/$name" >/dev/null
done

iconutil -c icns "$ICONSET_DIR" -o "$OUT_ICNS"
rm -rf "$ICONSET_DIR"

echo "✅ Icon generated: $OUT_ICNS"
