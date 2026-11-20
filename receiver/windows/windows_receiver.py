import serial
import time
try:
    import vgamepad as vg
except ImportError:
    print("Error: The 'vgamepad' library is not installed.")
    print("Please install it by running this in your terminal/command prompt:")
    print("pip install vgamepad pyserial")
    exit(1)

# --- Configuration ---
# IMPORTANT: Change this to the COM port your ESP32 is using in the Arduino IDE (e.g., 'COM3', 'COM5')
SERIAL_PORT = 'COM3' 
BAUD_RATE = 115200

print("Setting up Virtual Xbox Controller...")
try:
    gamepad = vg.VX360Gamepad()
    print("Virtual gamepad created successfully!")
except Exception as e:
    print("Error: Could not create virtual gamepad.")
    print("You might need to install the ViGEmBus driver on your Windows machine.")
    print("Download from: https://github.com/nefarius/ViGEmBus/releases")
    exit(1)

print(f"Connecting to ESP32 on {SERIAL_PORT} at {BAUD_RATE} baud...")

while True:
    try:
        with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
            print(f"Connected to {SERIAL_PORT}! Ready to play.")
            while True:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if not line:
                    continue
                
                parts = line.split(',')
                if len(parts) == 8:
                    try:
                        joyX = int(parts[0])
                        joyY = int(parts[1])
                        joySw = int(parts[2])
                        btn1 = int(parts[3])
                        btn2 = int(parts[4])
                        btn3 = int(parts[5])
                        btn4 = int(parts[6])
                        btn5 = int(parts[7])
                        
                        # Map Analog Stick (ESP32 is 0-4095. vgamepad expects -32768 to 32767)
                        # We map [0, 4095] to [-32768, 32767]
                        mapped_X = int(((joyX / 4095.0) * 65535) - 32768)
                        mapped_Y = int(((joyY / 4095.0) * 65535) - 32768)
                        
                        # Invert Y if necessary (joysticks often report Y inverted physically)
                        gamepad.left_joystick(x_value=mapped_X, y_value=-mapped_Y)
                        
                        # Map Buttons (1 = pressed, 0 = released)
                        if joySw: gamepad.press_button(button=vg.XUSB_BUTTON.XUSB_GAMEPAD_LEFT_THUMB)
                        else: gamepad.release_button(button=vg.XUSB_BUTTON.XUSB_GAMEPAD_LEFT_THUMB)
                        
                        if btn1: gamepad.press_button(button=vg.XUSB_BUTTON.XUSB_GAMEPAD_A)
                        else: gamepad.release_button(button=vg.XUSB_BUTTON.XUSB_GAMEPAD_A)
                        
                        if btn2: gamepad.press_button(button=vg.XUSB_BUTTON.XUSB_GAMEPAD_B)
                        else: gamepad.release_button(button=vg.XUSB_BUTTON.XUSB_GAMEPAD_B)
                        
                        if btn3: gamepad.press_button(button=vg.XUSB_BUTTON.XUSB_GAMEPAD_X)
                        else: gamepad.release_button(button=vg.XUSB_BUTTON.XUSB_GAMEPAD_X)
                        
                        if btn4: gamepad.press_button(button=vg.XUSB_BUTTON.XUSB_GAMEPAD_Y)
                        else: gamepad.release_button(button=vg.XUSB_BUTTON.XUSB_GAMEPAD_Y)
                        
                        if btn5: gamepad.press_button(button=vg.XUSB_BUTTON.XUSB_GAMEPAD_LEFT_SHOULDER)
                        else: gamepad.release_button(button=vg.XUSB_BUTTON.XUSB_GAMEPAD_LEFT_SHOULDER)
                        
                        # Update the virtual gamepad state
                        gamepad.update()
                        
                    except ValueError:
                        pass
    except serial.SerialException:
        print(f"Waiting for ESP32 on {SERIAL_PORT}... (Check if Arduino Serial Monitor is closed!)")
        time.sleep(2)
    except KeyboardInterrupt:
        print("\nExiting...")
        break
