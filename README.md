VEHEX

Overview
Library to control Victron ve.direct charging devices using the HEX serial protocol.

The normal ve.direct protocol only sends messages and therefore can't be used to contol the device.

The HEX protocol (there is a copy under docs) is a bit weird. It is command response protocol
but if you don't send it a command every second or so it reverts to the normal ve.direct protocol which
sends all the data the device has every second. So you need to send it a command continuously to keep
it in HEX mode (this ability is provided by the class function 'get'). Just to make it more difficult
it also sends asyncronous messages which you can ignore.

Hardware
I have only tested this on Hat Labs SH-ESP32 under platformio but it should work on any Arduino framework
device under platformio or the Arduino IDE. One thing to watch is if you are using a software serial you could
drop characters (the baud is set at 19200, so not too high), the messages do have a crude checksum so this
should be detected. Some of the software serial drivers detect overflow this could be useful.

Connections for the SH-ESP32 are:
VE.Direct   Name    GPIO pin
1           GND     GND
2           Rx      12
3           Tx      13
3           +5V     NC

Software
Other than the normal Arduino library there no other dependencies for vehex_example.cpp.
This was developed as part of a much larger Lithium Battery monitoring project sending to data a signal K
server. I intend to provide an example which will interface to Signal K via sensESP.

Setup
Clone or fork the code onto youe local machine