# Some Python to enteract with the LoRaMaDor project

Aprsisfeed.py - An attempt to watch the APRS-IS stream and look for messages to a callsign, the messages will be forwarded to the LoRaMaDor board The forwarded message will also contain the callsign of the sending ham you will need a LoRa board setup with the LoRaMaDor sketch to act as a gateway You will need to add your call/settings.   
install aprslib using pip, this script works with python3.

Chat.py - This is a modified version of the LoraHam-Console from Travis Goodspeed's loraham project https://github.com/travisgoodspeed/loraham it now sends packets and will send a packet to another LoRaMaDor node. this modified version removes most of the buttons, and moves the send button to below the message, making it a little easier to use this as a "chat". It will also show the verbose output of your board. To use this you will need a LoRaMaDor board hooked to your computer. Aug 18, 2020 LeRoy Miller I am still learning python, and I used this program as a good example, all credit should go to Travis Goodspeed, my changes were small.  

Loraham-console.py - slightly modified version of LoRaHam-Console by Travis Goodspeed, modified by LeRoy Miller Aug 17, 2020, all credit should go to Travis Goodspeed's and the loraham project https://github.com/travisgoodspeed/loraham it now sends packets and will send a packet to another LoRaMaDor node. To use this you will need a LoRaMaDor board hooked to your computer.  

This two scripts use urwid.  

lorasun.py - this is an attempt to make something useful for the LoRaMaDor project this program will check the times (UTC) of the Sun rise, and Sun set for a given latitude and longitude. After getting the information it will send to a LoRaMaDor node of your choice. You need a LoRaMaDor board hooked to the computer. Problem, this sends too much for the node to display correctly, but you can read it from a serial port copyright 2020, Aug 17, 2020 LeRoy Miller KD8BXP v0.0.1  
this uses urllib.request, and json  

I am pretty new to python, so there is still a lot I need to learn, please feel free to update this with improvements but please share so I can see and learn something. Thanks, LeRoy KD8BXP  

