#!/bin/bash

export PYTHON_BIN=$(which python3 || true)

set -e
mkdir -p "$HOME/bin"
export VENV_NAME="arritmic3D"
"$PYTHON_BIN" -m venv "$HOME/bin/venv/$VENV_NAME"
echo "Virtual environment created at $HOME/bin/venv/$VENV_NAME"

echo "Activating the virtual environment..."

source "$HOME/bin/venv/$VENV_NAME/bin/activate"
if [ "$VIRTUAL_ENV" != "$HOME/bin/venv/$VENV_NAME" ]; then
    echo "Error: The virtual environment was not activated correctly."
    exit 1
else
    echo "Virtual environment activated successfully."
    echo "Installing Python dependencies from requirements.txt..."
    pip install --upgrade pip
    pip install -r requirements.txt
    echo "Python dependencies installed successfully."
fi

