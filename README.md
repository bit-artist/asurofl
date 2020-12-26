## asurofl
asurofl(ash) is a command line tool to flash application binaries into ASURO's program memory. ASURO is a small programmable robot that was developed by Deutsches Zentrum für Luft- und Raumfahrt (DLR).
See <https://de.wikipedia.org/wiki/ASURO> to find out more about ASURO.

## Building
Just invoke `make`.

## Usage
`asurofl -i <binary file> -d <serial device>`

asurofl can also read from stdin

`cat <binary file> | asurofl -d <serial device>`

and write to stdout (just omit the -d option).
Error and Status messages are printed on stderr.

Example invocation:

`asurofl -i application.bin -d /dev/ttyS0`

## Workflow
1. Switch off ASURO
2. Execute asurofl
3. When it prints *Connecting..* switch on ASURO within 10 seconds to start the flash process.

## Supported devices
This program was tested on a Linux host (x86_64) with ASURO's IR-transceiver connected to RS-232 interface.

## How to convert from hex to binary?
`objcopy -I ihex -O binary application.hex application.bin`

## Why are hex files not supported?
There are better tools like objcopy for converting hex files to binary.

## License
asurofl is licensed under the MIT license. See license file.
