#!/usr/bin/env bash

# -----------------------------------------------------------------------------
# @brief  Setup or activate a Python virtual environment.
# @details
#   Checks for an existing ".venv" or "venv" directory. If neither exists,
#   creates ".venv". Then sources the activate script to enable the environment.
# -----------------------------------------------------------------------------

# Determine or create virtual environment directory
if [[ -d "./.venv" ]]; then
    VENV_DIR=".venv"
    source "${VENV_DIR}/bin/activate"
elif [[ -d "./venv" ]]; then
    VENV_DIR="venv"
    source "${VENV_DIR}/bin/activate"
else
    if [[ -f "./requirements.txt" ]]; then
        python3 -m venv .venv
        VENV_DIR=".venv"
        source ${VENV_DIR}/bin/activate
        pip install --upgrade pip
        pip install -r requirements.txt
    fi
fi

# Activate the virtual environment
return 0
