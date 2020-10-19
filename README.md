# LoRaMaDoR - ham radio based on LoRa 

This project is inspired by LoRaHam. The major enhancement is a packet format
which is more powerful and more extensible, allowing for true network formation.
The implementation is a work in
progress, and more documentation will be added as time permits.

# Console and command-line interface (CLI)

Use the terminal software of your choice (screen, minicom, etc.)
to connect to Arduino serial port. Set the speed to 115200bps
and disable RTS/CTS handshaking if necessary.

Alternatively, you can operate the console wirelessly using any
Telnet client e.g. PuTTY. Just fill in your Wi-Fi configuration
(details further down this document).

Users can use the protocol almost directly, with a "hands on" sensation,
the implementation fills in the red tape to generate a valid packet.
For example, a user types:

```
QC Chat tonight 22:00 at repeater 147.000
```

"QC" is the pseudo-destination for broadcast messages. The
actual transmitted packet (minus the FEC suffix) is something like:

```
QC<PU5EPX-11:33 Chat tonight 22:00 at repeater 147.000
```

There are some commands, all start with ! (exclamation point):

```
!callsign ABCDEF-1
!debug
!help
```

To show the currently configured callsign, call !callsign without a parameter.
By default is FIXMEE-1. Type !help to get a list of all available commands.

# More examples

Example of beacon packet, sent automatically every 10 minutes by
every station:

```
QB<PU5EPX-11:2 bat=7.93V temp=25.4C wind=25.4kmh
```

How to ping another station:

```
PP5CRE-11:PING test123
```

Actual traffic:
```
PP5CRE-11<PU5EPX-11:21,PING test123
PU5EPX-11<PP5CRE-11:54,PONG test123
```

Route request:

```
PP5CRE-11:RREQ test123
```

Actual traffic:
```
PP5CRE-11<PU5EPX-11:RREQ
```

Possible response:
```
PU5EPX-11<PP5CRE-11:RRSP >PP5ABC>PU5XYZ|PP5CRE-11>PU5ABC
```

## Packet format

A LoRaMaDoR packet is composed by three parts: header, payload and FEC code.
The header contains source callsign, destination callsign, and some 
parameters:

```
Destination<Source:Parameters Payload FEC
```

There shall not be spaces within the header. If there is a payload, it is separated
from the header by a single space. The FEC code is mandatory, and it is specified
in its own section later on.

Stations shall have 4 to 7 octets, shall not start with "Q", and may be suffixed
by an SSID. Special or pseudo-callsigns are: QB (beacon) for automatic broadcast
sent every 10 minutes, QC (think "CQ") for human broadcasting, and QL ("loopback")
for debugging/testing.

Parameters are a set of comma-separated list of items, for example:

```
123,A,B=C,D,E=FGH
```

The parameters can be in three formats: naked number, naked key, and key=value.

There shall be one and only one naked number in the parameter list: it is the
packet ID. Together with the source callsign, it uniquely identifies the packet
within the network in a 20-minute time window. This is used to e.g. avoid 
forwarding the same packet twice.

Keys (naked or not) must be composed of capital letters and numbers only, and must start
with a letter. Values may be composed of any characters except those used as delimiters
in the header (space, comma, equal, etc.)

A value can be empty and this should be handled different from a naked key e.g.
in `A=,B`, B is naked while A has a value, which is an empty string. Implementations
should allow for this distinction.

Reserved parameters, added or handled by the core stack:

`PING` asks for an automated `PONG` response.

`RREQ` (route request) asks for an automated `RRSP` response. Intermediate routers are
annotated in the message payload.

`C` (confirmation) asks for a confirmation message (with `CO` flag) to be sent back by
the destination.

`R` signals the packet was forwarded. This parameter is automatically added
and processed, and the user should not use it explicitly.

Predefined parameters available for any application:

`T=number` is an optional timestamp, as the UNIX timestamp (seconds since 1/1/1970
0:00 UTC) subtracted by 1552265462. If sub-second precision is required, the number
may have decimal places.

`S=chars` is an optional digital signature of the payload.

## Routing and forwarding

Currently, diffusion routing is the only implemented strategy.

By default, forwarding is OFF. To activate it, use the command `!repeater 1`
and restart the node. To deactivate, `!repeater 0`. Likewise the callsign, 
this setting is saved on NVRAM.

## FEC code

Every packet is augmented by a 20-octet FEC (Forward Error Code) suffix.
The FEC is a Reed-Solomon code.

Since Reed-Solomon codes demand a fixed-size message, it is calculated as if
the network packet was padded with nulls (binary zeros).

In order to contemplate low-memory microcontrollers, which cannot handle RS codes
above a certain size, two base sizes (and therefore two different RS codes) are
used: 100/80 and 200/180.

The reference FEC RS implementation is https://github.com/simonyipeter/Arduino-FEC .

## LoRa mode

Packets shall be transmitted using LoRa explicit mode, so the packet size is known
when it reaches the network layer to be parsed. CRC shall be disabled.

In our experiments, LoRa packets often arrive with errors even under the
best circunstances. Using maximum CR is not enough to prevent this, and
LoRa CRC protection would discard lots of packets. Software-level FEC code
seems to be the best solution.

## Console via Telnet

The console can be operated via Telnet. This is more convenient e.g. when
the Arduino is far away, perhaps positioned in a good antenna spot. This
is possible because ESP32 TTGO has built-in support for Wi-Fi and TCP/IP.

You just need to enter your network details in LoRaMaDoR via serial console,
using the commands !ssid and !password. Then !reset and check the console
for connection messages to confirm the network configuration is ok. Use the
!wifi command to get the detailed status.

The console and the !wifi command shows the IP address. But LoRaMaDoR also
supports Bonjour (mDNS). For example, if callsign is configured to ABCDEF-1,
the network name will be ABCDEF-1.local or abcdef-1.local (it is case-insensitive).

While a Telnet client is connected, the serial console is silenced (save
for Wi-Fi disconnection messages) and does not accept input. Once the Telnet
connection is closed, the serial console is reenabled.
