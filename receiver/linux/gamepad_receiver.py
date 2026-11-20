import serial
import time
import evdev
from evdev import UInput, AbsInfo, ecodes as e

# --- Configuration ---
SERIAL_PORT = '/dev/ttyACM0'  # Changed for Arduino Uno
BAUD_RATE = 115200

print("Setting up Virtual Xbox Controller...")

# Define the virtual GAMEPAD capabilities
cap = {
    e.EV_ABS: [
        (e.ABS_X, AbsInfo(512, 0, 1023, 0, 0, 0)),  # Left stick X
        (e.ABS_Y, AbsInfo(512, 0, 1023, 0, 0, 0)),  # Left stick Y
        (e.ABS_Z, AbsInfo(0, 0, 255, 0, 0, 0)),     # Left trigger
        (e.ABS_RZ, AbsInfo(0, 0, 255, 0, 0, 0)),    # Right trigger
    ],
    e.EV_KEY: [
        e.BTN_A, e.BTN_B, e.BTN_X, e.BTN_Y,  # Face buttons
        e.BTN_TL, e.BTN_TR,                   # Shoulder buttons
        e.BTN_THUMBL, e.BTN_THUMBR,           # Thumb buttons
        e.BTN_START, e.BTN_SELECT,            # Start/Select
    ]
}

try:
    ui = UInput(cap, name="Microsoft X-Box 360 pad", vendor=0x045e, product=0x028e, version=0x0110)
except PermissionError:
    print("ERROR: Permission denied.")
    print("You MUST run this script with 'sudo', or add your user to the 'input' group.")
    print("Try running: sudo python3 gamepad_receiver.py")
    exit(1)
except Exception as err:
    print(f"Error creating UInput: {err}")
    exit(1)

print(f"Connecting to Arduino Uno on {SERIAL_PORT} at {BAUD_RATE} baud...")

while True:
    try:
        with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
            print(f"Connected to {SERIAL_PORT}! Ready to play.")
            last_conn_send = time.time()
            try:
                ser.write(b"CONNECTED\n")
            except serial.SerialException:
                raise
            while True:
                now = time.time()
                if now - last_conn_send > 0.25:
                    try:
                        ser.write(b"CONNECTED\n")
                    except serial.SerialException:
                        break
                    last_conn_send = now

                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if not line:
                    continue
                
                parts = line.split(',')
                if len(parts) == 7:  # Changed from 8 to 7
                    try:
                        joyX = int(parts[0])
                        joyX = 1023 - joyX  # Invert X axis (left/right)
                        joyY = int(parts[1])
                        joySw = int(parts[2])
                        btn1 = int(parts[3])
                        btn2 = int(parts[4])
                        btn3 = int(parts[5])
                        btn4 = int(parts[6])
                        btn5 = 0  # Set to 0 since Arduino sends only 4 buttons
                        
                        # --- Send Joystick Axes ---
                        ui.write(e.EV_ABS, e.ABS_X, joyX)
                        ui.write(e.EV_ABS, e.ABS_Y, joyY)
                        ui.write(e.EV_ABS, e.ABS_Z, 0)    # Left trigger
                        ui.write(e.EV_ABS, e.ABS_RZ, 0)   # Right trigger
                        
                        # --- Send Buttons ---
                        ui.write(e.EV_KEY, e.BTN_START, joySw)  # Joystick Click = Start
                        ui.write(e.EV_KEY, e.BTN_A, btn1)       # Button 1 = A
                        ui.write(e.EV_KEY, e.BTN_B, btn2)       # Button 2 = B
                        ui.write(e.EV_KEY, e.BTN_X, btn3)       # Button 3 = X
                        ui.write(e.EV_KEY, e.BTN_Y, btn4)       # Button 4 = Y
                        ui.write(e.EV_KEY, e.BTN_TL, 0)         # Left shoulder
                        ui.write(e.EV_KEY, e.BTN_TR, 0)         # Right shoulder
                        ui.write(e.EV_KEY, e.BTN_THUMBL, 0)     # Left thumb
                        ui.write(e.EV_KEY, e.BTN_THUMBR, 0)     # Right thumb
                        ui.write(e.EV_KEY, e.BTN_SELECT, 0)     # Select
                        
                        # Send events to the OS
                        ui.syn()
                        
                    except ValueError:
                        pass 
    except serial.SerialException:
        print(f"Waiting for Arduino Uno on {SERIAL_PORT}...")
        time.sleep(2)
    except KeyboardInterrupt:
        print("\nExiting...")
        ui.close()
        break
