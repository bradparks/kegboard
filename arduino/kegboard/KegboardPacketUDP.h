#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <IPAddress.h>
#include <inttypes.h>
#include "KegboardPacket.h"

const byte KB_ETHERNET_MAC[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xAB };

// 'K' * 'B' == 11526
#define KB_UDP_PORT 11526

class KegboardPacketUDP : public KegboardPacket {
  public:
   KegboardPacketUDP(IPAddress dest_ip = 0, int dest_port = KB_UDP_PORT);
   void Print();

  protected:
   int dhcp_connected();

  private:
   EthernetUDP _udp;
   IPAddress _remote_ip;
   unsigned int _remote_port;
   int _dhcp_status;
};

