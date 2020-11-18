# Roadmap

Create prototype of "sensor" protocol, analogous to the Switch
protocol that models an actuator.

Abstract the challenge-response protection against replay attacks,
found in Switch, so it is reusable in other protocols.

Improve the Python stack quality, not just for testing.

Experiment with 433MHz LoRa, 500mW modules, amplifiers, Si4463
modules, so the project can work over a wider range of physical
media, and be more versatile. Supporting the Si4463 could be an
interesting challenge. Estimate the range of every option and
add to README.

Test with discrete LoRa modules (currently, the reference hardware
is TTGO that integrates ESP32 and LoRa in the same board).

Multiple devices for 1 callsign.
Case in view: LoRa module with amplifier, if the amplifier hinders RX,
the cheapest solution is to use a second LoRa module just for RX, but
both modules belong to the same station.

Telnet, serial x security.
Cases: Telnet login/sniffing, stolen device. Add Telnet password?

HMAC - multiple keys?

Possibility of remote, NAT-piercing access

Gateway via Internet, IP router

