#include <Arduino.h>
#include "kegboard.h"

#include "KegboardUDP.h"

/**
 * Get singleton instance
 */
KegboardUDP* KegboardUDP::get(IPAddress dest_ip, int dest_port) {
  static KegboardUDP* instance = 0;
  if (!instance) {
    instance = new KegboardUDP(dest_ip, dest_port);
  }

  return instance;
}

/**
 * Construct new KegboardUDP object with specified destination
 *
 * The default destination is *.113 on the current subnet, port 11526.
 */
KegboardUDP::KegboardUDP(IPAddress dest_ip, int dest_port) {
  if (dhcp_connected() && dest_ip == IPAddress((uint32_t) 0)) {
    _remote_ip = Ethernet.localIP();
    _remote_ip[3] = 113;
  }
  else {
    _remote_ip = dest_ip;
  }

  _remote_port = dest_port;

  dhcp_connected();
}

/**
 * Get current connection status
 *
 * @return true if packets can be sent; false otherwise.
 */
int KegboardUDP::dhcp_connected() {
  if (!_dhcp_status) {
    uint8_t mac[] = KB_ETHERNET_MAC;
    _dhcp_status = Ethernet.begin(mac);
    _dhcp_status = _dhcp_status && _udp.begin(KB_UDP_PORT);
    // TODO: find out what happens to sockets allocated with udp.begin() when 
    // the dhcp status changes.
  }
  else {
    // Make sure our lease stays current.
    // Ethernet.maintain() has its own time limits & caching, so this does
    // not cost too much.
    _dhcp_status = Ethernet.maintain();

    // 0 => no-op, 2 => renew success, 4 => rebind success
    if (_dhcp_status == 0 || _dhcp_status == 2 || _dhcp_status == 4)
      _dhcp_status = 1;
    else
      _dhcp_status = 0;
  }

  return _dhcp_status;
}

/**
 * Send a KegboardPacket over the UDP socket
 *
 * Assumes that the packet's CRC is valid, to save a few cycles
 * in the current implementation.
 *
 * TODO: verify that byte/bit ordering is little-endian
 */
void KegboardUDP::sendPacket(const KegboardPacket &p) {
  int i;

  if (dhcp_connected()) {
    _udp.beginPacket(_remote_ip, _remote_port);

    _udp.write((byte*) &p.m_type, 2);
    i = p.m_len;
    _udp.write((byte*) &i, 2);

    _udp.write(p.m_payload, i);

    // we're always called from KegboardPacket::Print after GenCrc, so 
    // the CRC is already up to date.
    _udp.write((byte*) &p.m_crc, 2);
    _udp.write("\r\n");

    _udp.endPacket();
  }
}

/**
 * If available, receive a KegboardPacket over the UDP socket
 *
 * TODO: verify that byte ordering is little-endian
 *
 * @return true if a valid packet was received; false otherwise.
 */
bool KegboardUDP::receivePacket(KegboardPacket& p) {
  uint16_t packet_size, crc;
  uint8_t buffer[KBSP_PAYLOAD_MAXLEN];
  KegboardPacket temp;

  // Fetch UDP packet from ethernet land
  packet_size = _udp.parsePacket();

  // Only bother with valid length packets
  if (packet_size > 0 && packet_size < KBSP_HEADER_LEN + KBSP_PAYLOAD_MAXLEN + KBSP_FOOTER_LEN) {

    // prefix "KBSP v1:"
    _udp.read((byte*)buffer, 8);
    buffer[8] = 0;
    if (String((char*)buffer) != KBSP_PREFIX)
       return false;

    // type value (2 bytes)
    _udp.read((byte*) &temp.m_type, 2);
    // length value (2 bytes)
    _udp.read((byte*) &packet_size, 2);
    if (packet_size < 0 || packet_size > 112)
      return false;

    // payload
    _udp.read((byte*) &temp.m_payload, packet_size);

    // crc: specified and calculated
    _udp.read((byte*) &crc, 2);
    temp.GenCrc();

    if (temp.m_crc == crc) {
      p.m_type    = temp.m_type;
      p.m_len     = temp.m_len;
      memcpy(p.m_payload, temp.m_payload, p.m_len);
      p.m_crc     = temp.m_crc;
      return true;
    }
  }

  return false;
}
