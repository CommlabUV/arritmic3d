#!/bin/bash

set -e

# Checking if Python 3.11 is installed...
echo "Checking if Python 3.11 is installed..."

PYTHON_BIN=$(which python3.11 || true)

if [ -x "$PYTHON_BIN" ]; then
  echo "Python 3.11 is installed at: $PYTHON_BIN"
else
  echo "Python 3.11 is not installed."
fi

# Checking if python3.11 works...
echo "Checking if python3.11 works..."
if [ -x "$PYTHON_BIN" ]; then
  echo "Python 3.11 is installed at: $PYTHON_BIN"
else
  echo "Error: Python 3.11 is not working correctly."
  exit 1
fi

# Installing required system dependencies for requirements.txt...
echo "Installing required system dependencies for requirements.txt..."

sudo apt install -y \
  build-essential \
  libcairo2-dev \
  libgirepository1.0-dev \
  gir1.2-gtk-3.0 \
  pkg-config \
  gcc-12 g++-12 \
  meson ninja-build \
  libeigen3-dev


echo "System dependencies successfully configured."