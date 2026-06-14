#!/usr/bin/env python3
import argparse
import sys

import serial
from serial.tools import list_ports

# Raspberry Pi Pico default USB vendor ID
PICO_USB_VID = 0x2E8A


def find_port():
    """Return the device path of the first attached Pico, or exit with an error."""
    for port in list_ports.comports():
        if port.vid == PICO_USB_VID:
            return port.device
    sys.exit("error: no Pico found (set --port to override)")


def transfer(ser, args):
    """Send one transfer request and print the response, returning an exit code."""
    # Forward the request verbatim: "transfer <response_len> <command> [args...]"
    request = "transfer " + " ".join([args.response_len] + args.data)
    ser.write((request + "\n").encode())

    # Read exactly one reply line
    reply = ser.readline().decode(errors="replace").strip()
    if not reply:
        print("Timeout error", file=sys.stderr)
        return 1

    # Split on the status token: "RX <bytes...>" or "!! <reason>"
    status, _, rest = reply.partition(" ")
    if status == "RX":
        print(rest)
        return 0

    print(rest or "Unknown error", file=sys.stderr)
    return 1


def main():
    parser = argparse.ArgumentParser(description="Joybus host CLI")
    parser.add_argument("--port", help="serial port (default: auto-detect Pico)")
    parser.add_argument(
        "--timeout", type=float, default=1.0, help="read timeout in seconds"
    )

    sub = parser.add_subparsers(dest="command", required=True)
    p_transfer = sub.add_parser("transfer", help="perform a raw Joybus transfer")
    p_transfer.add_argument("response_len", help="number of response bytes expected")
    p_transfer.add_argument(
        "data", nargs="+", help="command byte followed by data bytes"
    )

    args = parser.parse_args()

    port = args.port or find_port()
    with serial.Serial(port, timeout=args.timeout) as ser:
        if args.command == "transfer":
            sys.exit(transfer(ser, args))


if __name__ == "__main__":
    main()
