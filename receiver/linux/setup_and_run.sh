#!/bin/bash

echo "====================================="
echo " Arduino Uno Controller Setup for Ubuntu   "
echo "====================================="

echo "[1/3] Installing required system libraries..."
sudo apt update
sudo apt install -y gcc linux-libc-dev

echo "[1.5/3] Adding user to dialout group for serial access..."
sudo usermod -a -G dialout $USER
echo "Note: You may need to log out and log back in for the group change to take effect, or restart your session."

echo "[1.6/3] Loading uinput kernel module for virtual keyboard support..."
sudo modprobe uinput

echo "[2/3] Compiling keyboard_receiver.c..."
if [ -f "keyboard_receiver.c" ]; then
    gcc keyboard_receiver.c -o keyboard_receiver -O2
    if [ $? -ne 0 ]; then
        echo "ERROR: Compilation failed!"
        exit 1
    fi
    echo "Compilation successful!"
else
    echo "ERROR: keyboard_receiver.c not found."
    exit 1
fi

echo ""
echo "[3/3] Starting the C receiver program..."
echo "Press Ctrl+C at any time to stop it."
echo "-------------------------------------"
sudo ./keyboard_receiver


