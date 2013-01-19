#include <Ethernet.h>
#include <EthernetUdp.h>
#include <IPAddress.h>
#include <inttypes.h>
#include "KegboardPacket.h"

#define KB_ETHERNET_MAC { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xAB }

// 'K' * 'B' == 11526
#define KB_UDP_PORT 11526

class KegboardUDP {
  public:
   static KegboardUDP* get(IPAddress dest_ip = (uint32_t)0, int dest_port = KB_UDP_PORT);
   void sendPacket(const KegboardPacket &p);

   bool receivePacket(KegboardPacket& p);

  protected:
   KegboardUDP(IPAddress dest_ip, int dest_port);
   int dhcp_connected();

   EthernetUDP _udp;
   IPAddress _remote_ip;
   unsigned int _remote_port;
   int _dhcp_status;
};

