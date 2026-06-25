import serial
import sys
import time

def main():
    try:
        s = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)
    except Exception as e:
        print(f"Error opening serial port: {e}")
        sys.exit(1)
        
    print("[MONITOR] Resetting ESP32...")
    # Pulse DTR/RTS to reset
    s.setDTR(False)
    s.setRTS(True)
    time.sleep(0.1)
    s.setRTS(False)
    time.sleep(0.5)
    
    s.reset_input_buffer()
    print("[MONITOR] Monitoring serial output (35 seconds)...")
    
    start_time = time.time()
    while time.time() - start_time < 35:
        try:
            line = s.readline()
            if line:
                # Decode line and print
                text = line.decode('utf-8', errors='ignore')
                sys.stdout.write(text)
                sys.stdout.flush()
        except KeyboardInterrupt:
            print("\n[MONITOR] Interrupted by user.")
            break
        except Exception as e:
            print(f"\n[MONITOR] Error reading serial: {e}")
            break
            
    s.close()
    print("\n[MONITOR] Finished monitoring.")

if __name__ == "__main__":
    main()
