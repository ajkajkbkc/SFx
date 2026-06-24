#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
IAP Firmware Download Tool
Supports firmware update via USART1 (PA9/PA10) or USART3 (PC10/PC11)

Usage:
    python iap_download.py COM1 firmware.bin
    python iap_download.py COM1 firmware.bin --trigger
    python iap_download.py COM3 firmware.bin --baud 115200
"""

import sys
import time
import struct
import argparse

try:
    import serial
except ImportError:
    print("Error: pyserial not installed. Run: pip install pyserial")
    sys.exit(1)

# Protocol constants
IAP_SYNC_BYTE1 = 0xAA
IAP_SYNC_BYTE2 = 0x55
IAP_ACK = 0xCC
IAP_NAK = 0xEE
IAP_EOT = 0x04

IAP_DATA_PACKET_SIZE = 512
IAP_RETRY_MAX = 3

# Command for app to trigger reset back to bootloader
CMD_RESET_TO_BL = b'R'


def calc_checksum(data, packet_num):
    """Calculate XOR checksum matching bootloader logic."""
    checksum = 0
    for b in data:
        checksum ^= b
    checksum ^= (packet_num & 0xFF)
    checksum ^= ((packet_num >> 8) & 0xFF)
    return checksum


def send_and_wait(ser, data, expected_len=1, timeout=5.0):
    """Send data and wait for response."""
    ser.timeout = timeout
    ser.write(data)
    time.sleep(0.01)
    return ser.read(expected_len)


def iap_download(port, baudrate, firmware_path, trigger_reset=False):
    """Upload firmware to device via IAP bootloader."""

    print(f"[INFO] Opening {port} @ {baudrate} baud...")
    with serial.Serial(port, baudrate, timeout=1.0) as ser:
        ser.flushOutput()

        # Read firmware file
        with open(firmware_path, 'rb') as f:
            firmware = f.read()

        firmware_size = len(firmware)
        print(f"[INFO] Firmware: {firmware_path}")
        print(f"[INFO] Firmware size: {firmware_size} bytes ({firmware_size/1024:.1f} KB)")

        if firmware_size == 0:
            print("[ERROR] Firmware file is empty!")
            return False

        # Optional: trigger reset into bootloader
        if trigger_reset:
            print("[INFO] Triggering MCU reset...")
            # Method 1: Toggle DTR (many boards connect DTR to RESET pin)
            ser.setDTR(True)
            time.sleep(0.05)
            ser.setDTR(False)
            time.sleep(0.3)
            # Method 2: Also send 'R' command via UART (for app-based reset)
            for _ in range(10):
                ser.write(CMD_RESET_TO_BL)
                time.sleep(0.02)
            print("[INFO] Waiting for bootloader to start...")
            time.sleep(2.0)

        # Read any available data (welcome message)
        ser.timeout = 0.5
        _ = ser.read(256)

        # Send sync with retry (bootloader now ignores incorrect bytes)
        for sync_attempt in range(3):
            ser.reset_input_buffer()
            print(f"[INFO] Sending sync (attempt {sync_attempt + 1})...")
            ser.timeout = 3.0
            ser.write(bytes([IAP_SYNC_BYTE1, IAP_SYNC_BYTE2]))
            ack = ser.read(1)
            if ack == bytes([IAP_ACK]):
                print("[OK] Sync OK")
                break
            else:
                print(f"      Got: {ack.hex() if ack else 'nothing'}, retrying...")
                time.sleep(0.5)
        else:
            print("[ERROR] Sync failed after 3 attempts")
            return False

        # Step 3: Send firmware size (4 bytes, LE)
        print(f"[INFO] Sending firmware size: {firmware_size} bytes...")
        ser.write(struct.pack('<I', firmware_size))
        ack = ser.read(1)
        if ack != bytes([IAP_ACK]):
            print("[ERROR] Size ACK failed")
            return False
        print("[OK] Size OK")

        # Wait for erase to complete
        ack = ser.read(1)
        if ack != bytes([IAP_ACK]):
            print("[ERROR] Erase ACK failed")
            return False
        print("[OK] Flash erase OK")

        # Step 4: Send data packets
        total_packets = (firmware_size + IAP_DATA_PACKET_SIZE - 1) // IAP_DATA_PACKET_SIZE
        print(f"[INFO] Sending {total_packets} packets...")

        for packet_num in range(total_packets):
            offset = packet_num * IAP_DATA_PACKET_SIZE
            chunk = firmware[offset:offset + IAP_DATA_PACKET_SIZE]

            # Pad last packet with 0xFF if needed
            if len(chunk) < IAP_DATA_PACKET_SIZE:
                chunk = chunk + b'\xFF' * (IAP_DATA_PACKET_SIZE - len(chunk))

            # Calculate checksum
            checksum = calc_checksum(chunk, packet_num)

            # Build packet: [0x01][packet_num 2B LE][data 512B][checksum 1B]
            packet = bytes([0x01])
            packet += struct.pack('<H', packet_num)
            packet += chunk
            packet += bytes([checksum])

            # Send with retry
            success = False
            for retry in range(IAP_RETRY_MAX):
                ser.write(packet)
                ser.timeout = 5.0
                ack = ser.read(1)

                if ack == bytes([IAP_ACK]):
                    success = True
                    break
                else:
                    print(f"  [WARN] Packet {packet_num} retry {retry + 1}...")

            if not success:
                print(f"[ERROR] Packet {packet_num} send failed after {IAP_RETRY_MAX} retries")
                return False

            # Progress
            if (packet_num % 8 == 0) or (packet_num == total_packets - 1):
                pct = (packet_num + 1) * 100 // total_packets
                print(f"  Progress: {pct}% ({packet_num + 1}/{total_packets} packets)")

        # Step 5: Send EOT
        print("[INFO] Sending EOT...")
        ser.write(bytes([IAP_EOT]))
        ser.timeout = 5.0
        ack = ser.read(1)
        if ack == bytes([IAP_ACK]):
            print("[OK] Update complete! Device is jumping to application...")
        else:
            print(f"[WARN] EOT ACK not received: {ack.hex() if ack else 'nothing'}")

        # Try to read final message
        ser.timeout = 2.0
        final = ser.read(64)
        if final:
            print(f"[INFO] Final message: {final.decode('utf-8', errors='replace').strip()}")

        return True


def main():
    parser = argparse.ArgumentParser(description='STM32 IAP Firmware Download Tool')
    parser.add_argument('port', help='Serial port (e.g., COM1, COM3, /dev/ttyUSB0)')
    parser.add_argument('firmware', help='Path to firmware binary file (.bin)')
    parser.add_argument('--baud', type=int, default=9600,
                        help='Baud rate (default: 9600, match bootloader setting)')
    parser.add_argument('--trigger', action='store_true',
                        help='Send reset command to running app before flashing')

    args = parser.parse_args()

    print("=" * 50)
    print("STM32 IAP Firmware Download Tool")
    print("=" * 50)

    success = iap_download(args.port, args.baud, args.firmware, args.trigger)

    if success:
        print("\n[SUCCESS] Firmware update completed successfully!")
        return 0
    else:
        print("\n[FAILED] Firmware update failed!")
        return 1


if __name__ == '__main__':
    sys.exit(main())
