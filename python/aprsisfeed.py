# An attempt to watch the APRS-IS stream and look for messages to
# a callsign, the messages will be forwarded to the LoRaMaDor board
# The forwarded message will also contain the callsign of the sending ham
# you will need a LoRa board setup with the LoRaMaDor sketch to act as a gateway
# You will need to add your call/settings below
#
# copyright 2020, Aug 17, 2020 LeRoy Miller KD8BXP v0.0.1

import aprslib, json, serial #not sure json is really needed

def callback(packet):
    pto = packet['to']
    pfrom = packet['from']
    pformat = packet['format']
    if pto == 'N0CALL' and pformat == 'message': #put the callsign from APRS you want the messages forward from in pto ==
       print(pto + " " + pfrom + " " + packet['message_text'])
       msg = '{} {} {}'.format('N0CALL-2',pfrom,packet['message_text']).encode('utf-8') #put the callsign of the LoRaMaDor board you wish to recieve your message at
       print("Sending message...")
       print(msg)
       ser = serial.Serial('/dev/ttyUSB0', 115200) #You may need to change the serial port here, the speed should be the same 
       ser.write(msg)
       ser.write(b'\n\r')
       ser.close()

AIS = aprslib.IS("N0CALL",passwd='999999', port=14580) #at this point a passwd probably isn't required. 
AIS.connect()
AIS.set_filter("t/m") #message filter
#AIS.consumer(callback, raw=True)
AIS.consumer(callback, raw=False)

#{'raw': 'HS0QKD-9>APGJW6-1,WIDE1-1,qAS,E27HCD-1:!1305.41N/10055.29Ev080/007/A=000085', 'from': 'HS0QKD-9', 'to': 'APGJW6-1', 'path': ['WIDE1-1', 'qAS', 'E27HCD-1'], 'via': 'E27HCD-1', 'messagecapable': False, 'format': 'uncompressed', 'posambiguity': 0, 'symbol': 'v', 'symbol_table': '/', 'latitude': 13.090166666666667, 'longitude': 100.9215, 'course': 80, 'speed': 12.964, 'altitude': 25.908, 'comment': ''}

# I am pretty new to python, so there is still a lot I need to learn, please feel free to update this with improvements
# but please share so I can see and learn something. Thanks, LeRoy KD8BXP

# Idea for next version - make it a 2 way system, so that it can also send a message back into the APRS system passwd would be required for that.
