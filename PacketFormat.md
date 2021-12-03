# LoRaMaDoR protocol

This page describes the more technical details of the protocol.

## LoRaMaDoR packet format

A LoRaMaDoR packet is composed by three parts: header, payload and FEC code.
The header contains destination and source callsigns, as well as some 
parameters:

```
Destination<Source:Parameters Payload FEC
```

There shall not be spaces within the header. If there is a payload, it is separated
from the header by a single space. The FEC code is mandatory.

Stations shall have 4 to 7 octets, shall not start with "Q", and may be suffixed
by an SSID. Special or pseudo-callsigns are: QB (beacon) for automatic broadcast
sent every 10 minutes, QR (beacon of repeater), QC (think "CQ") for human
broadcasting, and QL ("loopback") for debugging/testing.

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

`H=chars` is an optional digital signature (HMAC) of the payload.

## Routing and forwarding

Currently, diffusion routing is the only implemented strategy.

By default, forwarding is OFF. To activate it, use the command `!repeater 1`
and restart the station. To deactivate, `!repeater 0`. Likewise the callsign, 
this setting is saved on NVRAM.

## Beacon interval

By default, the average interval of beacon packets is 10 minutes/600 seconds.
The fudge factor is 0.5, meaning the interval is randomly chosen between 5 and
15 minutes. The randomization avoids gratitous collisions when e.g. all stations
in the area are rebooted simultaneously after a blackout.

The first beacon packet is sent, by default and in average, 5 seconds after
startup. The fudge factor is 0.5 as well.

The beacon interval can be queried or set via command `!beacon`. Note the
value is expressed in seconds.

## FEC code and LoRa mode

This project uses LoRa-trans project for layer-2 packet encapsulation.
Refer to https://github.com/elvis-epx/LoRa-trans to learn the reasoning and
technical details about FEC code and chosen LoRa mode.

In our experiments, we found that LoRa packets arrive with errors even under
the best of circunstances. Using maximum CR is not enough to prevent this, and
LoRa CRC protection discards many almost-perfect packets. Software-level FEC
can employ stronger algorithms.

## Encryption

Currently, LoRaMaDoR does not support criptography, but the LoRa-trans library
(that handles layer-2 encapsulation) does support encryption. It is trivial to
change the source code to pass the encryption key to LoRaL2 constructor. This
could be useful for non-amateur uses.
