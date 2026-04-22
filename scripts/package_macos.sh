#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build-release"
DIST_DIR="$ROOT_DIR/dist/macos"
APP_NAME="FaceVeil.app"
APP_PATH="$BUILD_DIR/$APP_NAME"
DIST_APP="$DIST_DIR/$APP_NAME"
FRAMEWORKS_DIR="$DIST_APP/Contents/Frameworks"
MACOS_DIR="$DIST_APP/Contents/MacOS"
EXECUTABLE="$MACOS_DIR/FaceVeil"

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build "$BUILD_DIR" --config Release

rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR"
ditto "$APP_PATH" "$DIST_APP"

macdeployqt "$DIST_APP" \
  -verbose=1 \
  -libpath=/opt/homebrew/lib \
  -libpath=/opt/homebrew/Frameworks

mkdir -p "$FRAMEWORKS_DIR"

is_bundle_candidate() {
  local dependency="$1"
  [[ "$dependency" == /opt/homebrew/* || "$dependency" == /usr/local/* ]]
}

copy_dependency() {
  local dependency="$1"
  local name
  name="$(basename "$dependency")"

  if [[ ! -f "$FRAMEWORKS_DIR/$name" ]]; then
    cp "$dependency" "$FRAMEWORKS_DIR/$name"
    chmod u+w "$FRAMEWORKS_DIR/$name"
  fi
}

rewrite_dependency() {
  local binary="$1"
  local dependency="$2"
  local name
  name="$(basename "$dependency")"

  if [[ "$binary" == "$EXECUTABLE" ]]; then
    install_name_tool -change "$dependency" "@executable_path/../Frameworks/$name" "$binary" 2>/dev/null || true
  else
    install_name_tool -change "$dependency" "@loader_path/$name" "$binary" 2>/dev/null || true
  fi
}

rewrite_rpath_dependency() {
  local binary="$1"
  local dependency="$2"
  local name
  name="$(basename "$dependency")"

  if [[ ! -f "$FRAMEWORKS_DIR/$name" ]]; then
    return
  fi

  if [[ "$binary" == "$EXECUTABLE" ]]; then
    install_name_tool -change "$dependency" "@executable_path/../Frameworks/$name" "$binary" 2>/dev/null || true
  else
    install_name_tool -change "$dependency" "@loader_path/$name" "$binary" 2>/dev/null || true
  fi
}

rewrite_rpath_dependencies_for() {
  local binary="$1"
  while IFS= read -r dependency; do
    if [[ "$dependency" == @rpath/* ]]; then
      rewrite_rpath_dependency "$binary" "$dependency"
    fi
  done < <(otool -L "$binary" | awk 'NR > 1 {print $1}')
}

bundle_dependencies_for() {
  local binary="$1"
  while IFS= read -r dependency; do
    if is_bundle_candidate "$dependency"; then
      copy_dependency "$dependency"
      rewrite_dependency "$binary" "$dependency"
    fi
  done < <(otool -L "$binary" | awk 'NR > 1 {print $1}')
}

while :; do
  before_count="$(find "$FRAMEWORKS_DIR" -type f -name '*.dylib' | wc -l | tr -d ' ')"
  bundle_dependencies_for "$EXECUTABLE"
  while IFS= read -r dylib; do
    bundle_dependencies_for "$dylib"
  done < <(find "$FRAMEWORKS_DIR" -type f -name '*.dylib')
  after_count="$(find "$FRAMEWORKS_DIR" -type f -name '*.dylib' | wc -l | tr -d ' ')"
  [[ "$before_count" == "$after_count" ]] && break
done

rewrite_rpath_dependencies_for "$EXECUTABLE"
while IFS= read -r dylib; do
  install_name_tool -id "@loader_path/$(basename "$dylib")" "$dylib" 2>/dev/null || true
  rewrite_rpath_dependencies_for "$dylib"
done < <(find "$FRAMEWORKS_DIR" -type f -name '*.dylib')

mkdir -p "$DIST_APP/Contents/Resources/models"
find "$ROOT_DIR/models" -maxdepth 1 -type f -name '*.onnx' -exec cp {} "$DIST_APP/Contents/Resources/models/" \;

while IFS= read -r item; do
  codesign --force --sign - "$item" >/dev/null
done < <(find "$FRAMEWORKS_DIR" -maxdepth 2 \( -name '*.dylib' -o -name '*.framework' \) -print)

codesign --force --deep --sign - "$DIST_APP" >/dev/null

echo "Packaged app: $DIST_APP"
echo "Run with: open \"$DIST_APP\""
