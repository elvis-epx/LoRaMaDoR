# TNC mode

The default mode of command-line interface (CLI) is targeted at humans
interacting directly with a LoRaMaDoR station via serial or Telnet.
It has some amenities that are great for interactive sessions but
inadequate for a parser e.g. when the station is to be controlled by
a computer.

Also, the interactive mode is not 8-bit clean due to special handling
of control chars and Telnet char sequences.

To remedy this, the CLI can be put in "TNC" mode using the !tnc
command. In this mode, all RX data follows a definite format that is
easier to parse.

For now, only some test routines (Python scripts in test/ folder) use
the TNC mode, and are practical examples. In time we will develop 
modules that will use TNC in real-world scenarios e.g. collector of
sensor data, Internet repeater, Internet gateway, etc.

In TNC mode, every line sent from station starts with a label, and
ends with CR+LF. The prefix labels are:

"debug: " for debug messages. In TNC mode, debug mode is always active,
so there is no need to call !debug. In real-world usage, the debug
messages would not be parsed, but should be logged or stored to aid
any troubleshooting.

"cli: " for CLI messages e.g. the return of interactive commands like
!wifi, !neigh, etc. Their handling should be probably the same as
debug messages.

"net: " for out-of-band messages related to network e.g. report that
a packet was sent with ID #, or the packet was not sent due to an error.
Depending on the case, the payload of such messages will need to be
parsed.

"callsign: " for the first CLI message when the station is started up.
Followed by the configured callsign plus some additional info. 

"pkrx: " when a packet is received. This prefix is exclusive to TNC
mode. The packet contents are encoded in human-readable hexadecimal
digits, no spaces between bytes.

Any other message lines with different prefixes can be safely be ignored
by the host.

The host sends commands to the controller as lines terminated by CR
(following the Telnet convention). All commands work as usual.

Packets can be sent in two ways:

a) without a prefix, in plain text, starting with the destination station
as if a human was typing it; and

b) using the !pktx command followed by the packet encoded as human-readable
hexadecimal digits.

## 8-bit clean packet exchange

Together, the !pktx command and the pkrx: prefix establish an 8-bit clean
method of exchanging packets over a potentially ASCII-only channel.

Note that layer-2 (FEC and encryption, if activated in LoRa-trans) is still handled within the
microcontroller, even in TNC mode. The host only sees valid, layer-3
packets. It does not get "raw" packets.

Likewise, packets sent over !pktx are not blindly transmitted. Invalid
packets are rejected, and ID stamping, HMAC calculation, etc. are
still carried out by the microcontroller.
