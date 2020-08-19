#!/usr/bin/env python3


"""This is a half-assed attempt at a LoRaHam GUI that was written to
learn the Urwid library.  Transmitting packets does not yet work, but
it does allow a user to conveniently bounce between viewing packets
by different filters."""

#slightly modified version of LoRaHam-Console by Travis Goodspeed, modified by LeRoy Miller Aug 17, 2020, all credit should go to
# Travis Goodspeed's and the loraham project https://github.com/travisgoodspeed/loraham
# it now sends packets and will send a packet to another LoRaMaDor node. 
# To use this you will need a LoRaMaDor board hooked to your computer. 

# I am still learning python, and I used this program as a good example, all credit should go to Travis Goodspeed, my changes were small.

import urwid, serial;


MYCALL = "N0CALL" #Change to your callsign

try:
    MYCALL
except:
    raise Exception('set your callsign')
ENCODING = 'utf-8'  # don't change me
#ser = serial.Serial('/dev/ttyS0');
ser = serial.Serial('/dev/ttyUSB0', 115200); #You may need to change your serial port

SHOWALL=1;
SHOWMINE=2;
SHOWVERBOSE=3;
toshow=SHOWALL;
def on_showall_clicked(button):
    global toshow;
    toshow=SHOWALL;
    show_packets();

def on_showmine_clicked(button):
    global toshow;
    toshow=SHOWMINE;
    show_packets();

def on_showverbose_clicked(button):
    global toshow;
    toshow=SHOWVERBOSE;
    show_packets();

def on_send(button):
    global packet, dest, ser
    dest.set_edit_text(dest.get_edit_text().upper())
    packet_to_send = '{} {} {}'.format(dest.get_edit_text(),
                                       MYCALL,
                                       packet.get_edit_text()).encode(ENCODING)
    ser.write(packet_to_send)
    ser.write(b'\n\r')
    packet.set_edit_text('')
    


def show_packets():
    global toshow, allpackets, mypackets, verbloselog;
    if toshow==SHOWALL:
        received.set_text(allpackets);
    elif toshow==SHOWMINE:
        received.set_text(mypackets);
    elif toshow==SHOWVERBOSE:
        received.set_text(verboselog);

def on_packet_change(edit, new_edit_text):
    pass;

#Is my callsign mentioned in the line?
def ismentioned(line):
    for word in line.split():
        if word==MYCALL:
            return True;
    return False;
    
linestate=0;
lastline="";
def handle_line(line):
    global allpackets, mypackets, verboselog, linestate, lastline;

    #Verbose log has everything.
    verboselog=verboselog+line+'\n';

    #Empty lines delimit transmissions.
    if len(line)==0:
        linestate=0;
        return;

    #First line is the real message.
    if linestate==0 and line!=lastline:
        lastline=line;
        allpackets=allpackets+line+"\n";
        if ismentioned(line):
            mypackets=mypackets+line+"\n";
    linestate=linestate+1;
    
def ser_available():
    global ser, allpackets;
    try:
        newline=ser.readline().decode(ENCODING).strip();
        handle_line(newline);
        show_packets();
    except:
        handle_line("Data lost to UTF-8 corruption.");
        show_packets();


#Some globals for our state.
allpackets="";
mypackets="";
verboselog="";

palette = [('entry', 'bold,yellow', 'black'),]

dest=urwid.Edit(('entry', u"Destination:\n"));
packet=urwid.Edit(('entry', u"Message to send:\n"));
received=urwid.Text(('body_text', u"Packets will appear here when they arrive."));
div=urwid.Divider();
#button=urwid.Button(u'Exit');

showall=urwid.Button(('button', u'All'));
showmine=urwid.Button(('button', u'Mine'));
showverbose=urwid.Button(('button', u'Verbose'));
sendpacket=urwid.Button(('button', u'Send'));
buttons=urwid.Columns([showall,showmine,showverbose,sendpacket]);

pile=urwid.Pile([received,div,buttons,dest,packet]);
top=urwid.Filler(pile,valign='bottom');

urwid.connect_signal(packet, 'change', on_packet_change);
urwid.connect_signal(showall, 'click', on_showall_clicked);
urwid.connect_signal(showmine, 'click', on_showmine_clicked);
urwid.connect_signal(showverbose, 'click', on_showverbose_clicked);
urwid.connect_signal(sendpacket, 'click', on_send);

loop=urwid.MainLoop(top, palette)
loop.watch_file(ser, ser_available);
loop.run();

