VEHEX


Overview
Library to control Victron ve.direct charging devices using the HEX serial protocol.

The normal ve.direct protocol only sends messages and therefore can't be used to control the device.

The HEX protocol (there is a copy under docs) is a bit weird. It is command/response protocol
but if you don't send it a command every second or so it reverts to the normal ve.direct protocol which
sends all the data from the device has every second. So you need to send it a commandscontinuously to keep
it in HEX mode (this ability is provided by the class function 'get'). Just to make it more difficult
it also sends asyncronous messages which you can ignore.

I have only implemented a small number of the hundreds of possibled command IDs but it is very easy to add new commands
using add_paser or add_parser_raw.

Hardware

I have only tested this on Hat Labs SH-ESP32 under platformio but it should work on any Arduino framework
device under platformio or the Arduino IDE. One thing to watch is if you are using a software serial you could
drop characters (the baud is set at 19200, so not too high), the messages do have a crude checksum so this
should be detected. Some of the software serial drivers detect overflow this could be useful.

Connections for the SH-ESP32 are:
VE.Direct   Name    GPIO pin
1           GND     GND
2           Rx      12      Data from ve.direct device
3           Tx      13      Data to ve.direct device
3           +5V     NC

The Victron MPPTs output 0 to 5V so you need a potential divered to reduce the voltage. I use a 10K resistor
from the MPPT to GPIO 12 and a 10K from there to ground. The Tx from GPIO13 works fine straight into the MPPT.

Software

Other than the normal Arduino library the only dependency is https://github.com/davidcliddell/libDCL.git
which has some simple functions in it. The libDCL path is in the platformio.ini and library.json.
This was developed as part of a much larger Lithium Battery monitoring project sending to data a signal K
server. I intend to provide an example which will interface to Signal K via sensESP.

Setup
Setup a project folder on platformio (you can use the wizard).Get the code from github onto your local machine, copy platformioi.ini and examples/vehex_example.cpp to your project compile.

Good luck