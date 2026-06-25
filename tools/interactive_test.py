import serial
import sys
import time
import threading
import socket
import re

esp32_ip = None

def read_serial_loop(ser):
    global esp32_ip
    while True:
        try:
            if ser.in_waiting:
                line = ser.readline()
                if line:
                    text = line.decode('utf-8', errors='ignore')
                    sys.stdout.write(text)
                    sys.stdout.flush()
                    
                    # Parse IP Address from logs
                    # e.g., "IP Address: 192.168.1.18"
                    match = re.search(r"IP Address:\s*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)", text)
                    if match:
                        esp32_ip = match.group(1)
                        print(f"\n[MONITOR] Parsed ESP32 IP: {esp32_ip}")
            time.sleep(0.01)
        except Exception as e:
            break

def print_menu():
    print("\n" + "="*60)
    print("      SecurIoT-Quantum Shield Interactive Console")
    print("="*60)
    print("  --- SERIAL PORT SIMULATED ACTIONS ---")
    print("  [1] Perform Ephemeral Post-Quantum Key Exchange (Kyber-768)")
    print("  [2] Send Normal Telemetry Packet")
    print("  [3] Simulate IP Rate-Limiting Attack")
    print("  [4] Simulate DDoS Fragmentation Flood")
    print("  [5] Reset Alarm Pin (LEDs) & Drop Counters")
    print("  --- REAL WI-FI NETWORK ACTIONS (requires ESP32 IP) ---")
    if esp32_ip:
        print(f"  [6] Send REAL Normal UDP Packet (to {esp32_ip}:8080)")
        print(f"  [7] Send REAL UDP Rate-Limiting Flood (to {esp32_ip}:8080)")
        print(f"  [8] Send REAL UDP DDoS Fragmentation Flood (to {esp32_ip}:8080)")
    else:
        print("  [6-8] (Waiting for ESP32 to connect to Wi-Fi and report IP...)")
    print("  --- SYSTEM ACTIONS ---")
    print("  [9] Reset ESP32")
    print("  [q] Quit")
    print("="*60)

def main():
    port = '/dev/ttyUSB0'
    baud = 115200
    try:
        ser = serial.Serial(port, baud, timeout=1)
    except Exception as e:
        print(f"Error opening serial port {port}: {e}")
        sys.exit(1)

    print(f"[SYSTEM] Connected to ESP32 on {port} at {baud} baud.")
    
    # Start reader thread
    t = threading.Thread(target=read_serial_loop, args=(ser,), daemon=True)
    t.start()

    # Initial reset to start cleanly
    print("[SYSTEM] Resetting ESP32...")
    ser.setDTR(False)
    ser.setRTS(True)
    time.sleep(0.1)
    ser.setRTS(False)
    time.sleep(0.5)
    ser.reset_input_buffer()

    time.sleep(1.5) # Wait for boot output

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(1.0)

    while True:
        try:
            print_menu()
            choice = input("Enter choice: ").strip()
            
            if choice == 'q':
                break
            elif choice in ['1', '2', '3', '4', '5']:
                ser.write(choice.encode('utf-8'))
                time.sleep(2.0 if choice in ['1', '4'] else 0.5)
            elif choice in ['6', '7', '8']:
                if not esp32_ip:
                    print("[ERROR] ESP32 Wi-Fi IP address not detected yet. Check serial output.")
                    continue
                
                if choice == '6':
                    print(f"\n[NETWORK] Sending 1 normal UDP packet to {esp32_ip}:8080...")
                    sock.sendto(b"NormalNetworkTelemetryPacketData", (esp32_ip, 8080))
                    try:
                        data, addr = sock.recvfrom(1024)
                        print(f"[NETWORK] ESP32 Response: {data.decode('utf-8')}")
                    except socket.timeout:
                        print("[NETWORK] No response (packet may have been dropped or network blocked).")
                
                elif choice == '7':
                    print(f"\n[NETWORK] Sending rate-limiting UDP flood (15 packets in 100ms) to {esp32_ip}:8080...")
                    for i in range(15):
                        sock.sendto(b"RateLimitFloodPacket", (esp32_ip, 8080))
                        time.sleep(0.005)
                    print("[NETWORK] Flood finished. Check serial logs for rate limit blocks.")
                
                elif choice == '8':
                    print(f"\n[NETWORK] Sending DDoS UDP fragmentation flood (20 mismatched fragments) to {esp32_ip}:8080...")
                    fragment1 = b'F' + bytes([4, 0, 0])
                    fragment2 = b'F' + bytes([4, 3, 0])
                    for i in range(20):
                        payload = fragment1 if i % 2 == 0 else fragment2
                        sock.sendto(payload, (esp32_ip, 8080))
                        time.sleep(0.01)
                    print("[NETWORK] DDoS Flood finished. Check serial logs - LED (GPIO 2 / 5) should light up.")
            
            elif choice == '9':
                print("[SYSTEM] Resetting ESP32...")
                ser.setDTR(False)
                ser.setRTS(True)
                time.sleep(0.1)
                ser.setRTS(False)
                time.sleep(0.5)
                ser.reset_input_buffer()
                time.sleep(1.0)
            else:
                print("Invalid choice, please try again.")
        except KeyboardInterrupt:
            print("\nExiting...")
            break

    ser.close()
    sock.close()
    print("Goodbye!")

if __name__ == "__main__":
    main()
