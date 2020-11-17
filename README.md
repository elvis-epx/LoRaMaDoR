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

Users can play with the protocol with (seemingly) bare hands.
The implementation fills in the red tape to generate a valid packet.
For example, a user types:

```
QC Chat tonight 22:00 at repeater 147.000
```

"QC" is the pseudo-destination for broadcast messages. The
actual transmitted packet (minus the FEC suffix) is something like

```
QC<PU5EPX-11:33 Chat tonight 22:00 at repeater 147.000
```

There are some commands to interact with the local station.
Every command starts with ! (exclamation point):

```
!callsign ABCDEF-1
!debug
!help
```

To show the currently configured callsign, issue !callsign without a parameter.
By default is FIXMEE-1. Type !help to get a list of all available commands.

# More examples

Example of beacon packet, by default sent every 10 minutes:

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

## The protocol

See [this page](PacketFormat.md) for technical details about the
wire protocol.

## Console via Telnet

The console can be operated via Telnet. This is more convenient when
the Arduino is far away, perhaps positioned in a good antenna spot. This
is possible because ESP32 TTGO has built-in support for Wi-Fi and TCP/IP.

You just need to enter your network details in LoRaMaDoR via serial console,
using the commands !ssid and !password. Then !reset and check the console
messages to confirm the network configuration is ok. Use the !wifi command
to get the detailed connection status.

The IP address is shown upon connection on console, and can be queried via
!wifi command. Generally you won't need to find out the IP, since
LoRaMaDoR also supports Bonjour (mDNS). For example, if the station callsign is
ABCDEF-1, the network name will be ABCDEF-1.local or abcdef-1.local
(network names are case-insensitive).

While a Telnet client is connected, the serial console is silenced (save
for Wi-Fi disconnection messages) and does not accept input. Once the Telnet
connection is closed, the serial console is reenabled.
