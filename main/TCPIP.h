#ifndef __TCPIP_H
#define __TCPIP_H

class Network;

void wifi_setup(Ptr<Network>);
void wifi_handle();
void telnet_print(const char *);
Buffer get_wifi_status();

#endif
