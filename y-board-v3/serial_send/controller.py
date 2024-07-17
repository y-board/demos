import argparse
import time

import serial
import psutil


def send_cpu_usage(serial_port):
    cpu_usage_list = []

    with serial.Serial(serial_port, 115200, timeout=1) as ser:
        while True:
            # Read the current CPU usage
            cpu_usage = psutil.cpu_percent(interval=1)

            # Map the CPU usage from 0-100 to 0-20
            mapped_value = int((cpu_usage / 100) * 20)

            # Convert the mapped value to bytes and send it over serial
            payload = f"{mapped_value}\n".encode()
            ser.write(payload)

            # print(payload, ser.read_all())


if __name__ == "__main__":
    # Parse the arguments to get the serial device
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "serial_port", help="The serial port where the device is connected."
    )
    args = parser.parse_args()

    try:
        send_cpu_usage(args.serial_port)
    except serial.serialutil.SerialException:
        print("Serial port not found.")
    except KeyboardInterrupt:
        pass
