import usb.core
dev = usb.core.find()
cfg = dev.get_active_configuration()
intf = cfg[(3,0)] # vendor interface
out_ep, in_ep = intf.endpoints()

out_ep.write(b"\x06\x8F\xFF\xFF")
result = in_ep.read(1024)
print("Got:", result.tobytes())
