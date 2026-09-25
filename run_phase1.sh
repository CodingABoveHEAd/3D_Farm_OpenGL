#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ -n "${MSYS_ROOT:-}" ]]; then
    MSYS_ROOT="$MSYS_ROOT"
elif [[ -d /c/msys64/ucrt64 ]]; then
    MSYS_ROOT="/c/msys64/ucrt64"
else
    MSYS_ROOT="/ucrt64"
fi
CXX="${CXX:-$MSYS_ROOT/bin/g++}"
BUILD_DIR="$PROJECT_ROOT/build"
OUTPUT="$BUILD_DIR/Phase1Farm.exe"

if [[ ! -x "$CXX" ]]; then
    echo "Error: g++ was not found at $CXX" >&2
    echo "Install MSYS2 UCRT64 or set CXX to your g++ executable." >&2
    exit 1
fi

if [[ ! -f "$MSYS_ROOT/include/GLFW/glfw3.h" ]]; then
    echo "Error: GLFW headers were not found under $MSYS_ROOT/include." >&2
    exit 1
fi

if [[ ! -f "$MSYS_ROOT/lib/libglfw3.a" ]]; then
    echo "Error: GLFW library was not found under $MSYS_ROOT/lib." >&2
    exit 1
fi

mkdir -p "$BUILD_DIR"

SOURCES=(
    "$PROJECT_ROOT/src/main.cpp"
    "$PROJECT_ROOT/src/Application.cpp"
    "$PROJECT_ROOT/src/Camera.cpp"
    "$PROJECT_ROOT/src/Input.cpp"
    "$PROJECT_ROOT/src/Scene.cpp"
    "$PROJECT_ROOT/src/Animation.cpp"
    "$PROJECT_ROOT/src/Lighting.cpp"
    "$PROJECT_ROOT/src/graphics/Primitives.cpp"
)

echo "Building Phase 1..."
"$CXX" \
    -std=c++17 \
    -I"$MSYS_ROOT/include" \
    -I"$PROJECT_ROOT/include" \
    "${SOURCES[@]}" \
    -L"$MSYS_ROOT/lib" \
    -lglfw3 \
    -lopengl32 \
    -lgdi32 \
    -luser32 \
    -lshell32 \
    -o "$OUTPUT"

export PATH="$MSYS_ROOT/bin:$PATH"
echo "Launching $OUTPUT"
"$OUTPUT"