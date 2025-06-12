#!/bin/bash

set -e

echo "Checking if Python 3.12 is installed..."

PYTHON_BIN=$(which python3.12 || true)

if [ -x "$PYTHON_BIN" ]; then
  echo "Python 3.12 is installed at: $PYTHON_BIN"
else
  exit 1
fi

echo "Checking if python3.12 works..."
if [ -x "$PYTHON_BIN" ]; then
  echo "Python 3.12 is installed at: $PYTHON_BIN"
else
  echo "Error: Python 3.12 is not working correctly."
  exit 1
fi

echo "Installing required system dependencies for requirements.txt..."

sudo apt update
sudo apt install -y \
  build-essential \
  libcairo2-dev \
  libgirepository1.0-dev \
  gir1.2-gtk-3.0 \
  pkg-config \
  gcc-12 g++-12 \
  meson ninja-build \
  libeigen3-dev \
  python3-pybind11 \
  python3-dev \
  python3-venv \
  paraview

echo "System dependencies successfully configured."