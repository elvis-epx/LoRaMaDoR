# Non-amateur usage

Even though this project sees amateur radio community as the primary
user, we feel LoRaMaDoR provides a good foundation for non-amateur
uses of LoRa radios. Encryption was specifically implemented for
these use cases.

(Amateurs must communicate using cleartext; on the
other hand, amateurs are not bound to ISM band limitations of
transmitter power, antenna gain and duty cycle.)

The Proto\_Switch protocol is the initial PoC of something that
I intend to use in a real-world scenario: control lights and
actuators remotely over a reasonably high distance (~1km). 
Other home automation technologies I have checked (X10, Bluetooth,
Wi-Fi) have 1/10 of the needed range.

The Switch protocol has to implement protection against relay
attacks. It uses a challenge-response scheme, and packets must
be HMAC-signed to make sure the response can be trusted.

This project does not use encryption, but LoRaL2
encapsulation does support encryption, so you can change the
code. Specifically, pass the encryption key and its length
to LoRaL2 constructor, called in Network.cpp.
