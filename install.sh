#!/bin/bash

set -euo pipefail

# Preferred Python version
PREFERRED_PYTHON_VERSION="3.12"

echo "Checking for python3 in PATH..."

PYTHON_BIN=$(command -v python3 2>/dev/null || true)

if [ -z "$PYTHON_BIN" ]; then
  echo "Error: python3 not found in PATH." >&2
  exit 1
fi

echo "Found python3 at: $PYTHON_BIN"

# Get major.minor (e.g. 3.13)
PY_VER=$("$PYTHON_BIN" -c 'import sys; print("{}.{}".format(sys.version_info.major, sys.version_info.minor))' 2>/dev/null || true)

if [ -z "$PY_VER" ]; then
  echo "Error: unable to determine python3 version." >&2
  exit 1
fi

echo "Detected python3 version: $PY_VER"

if [ "$PY_VER" != "$PREFERRED_PYTHON_VERSION" ]; then
  echo "Warning: this script is tested with Python $PREFERRED_PYTHON_VERSION but Python $PY_VER was detected. Continuing anyway."
else
  echo "Python matches preferred version: $PREFERRED_PYTHON_VERSION."
fi

echo "Installing required system dependencies..."

# Fail early with clear messages if apt commands fail
if ! sudo apt update; then
  echo "Error: 'apt update' failed. Aborting." >&2
  exit 1
fi

# Use a noninteractive frontend for automation and check exit status
if ! sudo DEBIAN_FRONTEND=noninteractive apt install -y \
  build-essential \
  libcairo2-dev \
  libgirepository1.0-dev \
  gir1.2-gtk-3.0 \
  pkg-config \
  gcc g++ \
  meson ninja-build \
  libeigen3-dev \
  python3-pybind11 \
  python3-dev \
  python3-venv
then
  echo "Error: 'apt install' failed. Aborting." >&2
  exit 1
fi

echo "System dependencies successfully configured."
