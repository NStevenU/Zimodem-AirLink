#if INCLUDE_PPP
#include "lwip/tcpip.h"
#include "lwip/sys.h"

extern int qosBps;
extern float qosTokens;
extern unsigned long qosLastTime;

static const uint16_t fcstab[256] = {
  0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
  0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
  0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
  0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
  0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
  0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
  0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
  0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
  0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
  0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
  0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
  0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
  0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
  0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
  0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
  0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
  0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
  0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
  0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
  0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
  0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
  0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
  0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
  0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
  0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
  0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
  0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
  0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
  0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
  0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
  0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
  0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

static err_t ppp_wifi_output_hook(struct netif *netif, struct pbuf *p, const ip4_addr_t *ipaddr)
{
  if(pppMode.original_wifi_output != NULL)
    return pppMode.original_wifi_output(netif, p, ipaddr);
  return ERR_OK;
}

static err_t ppp_wifi_input_hook(struct pbuf *p, struct netif *inp)
{
  if((p != NULL) && (p->len >= 34))
  {
    uint8_t *payload = (uint8_t *)p->payload;
    uint8_t *data = payload + 14;
    uint8_t ip_version = (data[0] >> 4) & 0x0F;

    if(ip_version == 4)
    {
      uint32_t dest_ip = (data[16] << 24) | (data[17] << 16) | (data[18] << 8) | data[19];

      IPAddress wifiIP = WiFi.localIP();
      uint32_t our_ip = ((uint32_t)wifiIP[0] << 24) | ((uint32_t)wifiIP[1] << 16) |
                        ((uint32_t)wifiIP[2] << 8) | (uint32_t)wifiIP[3];
      if(dest_ip == our_ip)
      {
        pbuf_remove_header(p, 14);
        if(baudRate >= 57600)
          pppMode.sendPacketToSerial(p);
        else
        if((dest_ip & 0xF0000000) != 0xE0000000 && dest_ip != 0xFFFFFFFF)
          pppMode.sendPacketToSerial(p);
        pbuf_free(p);
        return ERR_OK;
      }
    }
  }

  if(pppMode.original_wifi_input != NULL)
    return pppMode.original_wifi_input(p, inp);

  return ERR_OK;
}

static err_t ppp_netif_init(struct netif *netif)
{
  netif->name[0] = 'p';
  netif->name[1] = 'p';
  netif->mtu = 1500;
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_LINK_UP;
  return ERR_OK;
}

uint16_t ZPPPMode::calculateFCS(uint8_t *data, int len)
{
  uint16_t fcs = 0xFFFF;
  for(int i = 0; i < len; i++)
    fcs = (fcs >> 8) ^ fcstab[(fcs ^ data[i]) & 0xFF];
  return fcs ^ 0xFFFF;
}

void ZPPPMode::sendPPPFrame(uint16_t protocol, uint8_t *data, int len)
{
  uint8_t header[4];
  header[0] = 0xFF;
  header[1] = 0x03;
  header[2] = (protocol >> 8) & 0xFF;
  header[3] = protocol & 0xFF;

  uint16_t fcs = 0xFFFF;
  for(int i = 0; i < 4; i++)
    fcs = (fcs >> 8) ^ fcstab[(fcs ^ header[i]) & 0xFF];
  for(int i = 0; i < len; i++)
    fcs = (fcs >> 8) ^ fcstab[(fcs ^ data[i]) & 0xFF];
  fcs ^= 0xFFFF;

  sserial.printb(PPP_FLAG);
  if(sserial.isSerialOut())
    serialOutDeque();

  for(int i = 0; i < 4; i++)
  {
    uint8_t c = header[i];
    if(c == PPP_FLAG || c == PPP_ESCAPE || c < 0x20)
    {
      sserial.printb(PPP_ESCAPE);
      sserial.printb(c ^ PPP_TRANS);
    }
    else
      sserial.printb(c);
  }

  for(int i = 0; i < len; i++)
  {
    uint8_t c = data[i];
    if(c == PPP_FLAG || c == PPP_ESCAPE || c < 0x20)
    {
      sserial.printb(PPP_ESCAPE);
      sserial.printb(c ^ PPP_TRANS);
    }
    else
      sserial.printb(c);

    if((i % 64) == 0)
    {
      yield();
      if(sserial.isSerialOut())
        serialOutDeque();
    }
  }

  uint8_t fcs_bytes[2];
  fcs_bytes[0] = fcs & 0xFF;
  fcs_bytes[1] = (fcs >> 8) & 0xFF;
  for(int i = 0; i < 2; i++)
  {
    uint8_t c = fcs_bytes[i];
    if(c == PPP_FLAG || c == PPP_ESCAPE || c < 0x20)
    {
      sserial.printb(PPP_ESCAPE);
      sserial.printb(c ^ PPP_TRANS);
    }
    else
      sserial.printb(c);
  }

  sserial.printb(PPP_FLAG);
  if(sserial.isSerialOut())
    serialOutDeque();
}


void ZPPPMode::sendLCPPacket(uint8_t code, uint8_t id, uint8_t *data, int len)
{
  uint8_t packet[1500];
  packet[0] = code;
  packet[1] = id;
  packet[2] = ((len + 4) >> 8) & 0xFF;
  packet[3] = (len + 4) & 0xFF;
  if(len > 0)
    memcpy(packet + 4, data, len);

  sendPPPFrame(PPP_PROTOCOL_LCP, packet, len + 4);
}

void ZPPPMode::sendIPCPPacket(uint8_t code, uint8_t id, uint8_t *data, int len)
{
  uint8_t packet[1500];
  packet[0] = code;
  packet[1] = id;
  packet[2] = ((len + 4) >> 8) & 0xFF;
  packet[3] = (len + 4) & 0xFF;
  if(len > 0)
    memcpy(packet + 4, data, len);

  sendPPPFrame(PPP_PROTOCOL_IPCP, packet, len + 4);
}

void ZPPPMode::sendPAPPacket(uint8_t code, uint8_t id, const char *msg)
{
  uint8_t packet[256];
  int msgLen = msg ? strlen(msg) : 0;
  packet[0] = code;
  packet[1] = id;
  packet[2] = ((msgLen + 5) >> 8) & 0xFF;
  packet[3] = (msgLen + 5) & 0xFF;
  packet[4] = msgLen; // Message Length
  if(msgLen > 0)
    memcpy(packet + 5, msg, msgLen);

  sendPPPFrame(PPP_PROTOCOL_PAP, packet, msgLen + 5);
}

void ZPPPMode::handleLCP(uint8_t *data, int len)
{
  if(len < 4)
    return;

  uint8_t code = data[0];
  uint8_t id = data[1];

  switch(code)
  {
    case LCP_CONF_REQ:
    {
      uint8_t response[256];
      int respLen = 0;
      int i = 4;
      bool reject = false;
      
      while(i < len)
      {
        uint8_t optType = data[i];
        uint8_t optLen = data[i + 1];
        if(optLen < 2) break; // Invalid option length

        // STRICT WHITELIST: Only accept MRU (1), ACCM (2), and Magic Number (5)
        // Anything else (like Callback 13, PFC 7, ACFC 8) MUST be rejected.
        if(optType != 1 && optType != 2 && optType != 5)
        {
          memcpy(response + respLen, data + i, optLen);
          respLen += optLen;
          reject = true;
        }
        i += optLen;
      }

      if(reject)
      {
        sendLCPPacket(LCP_CONF_REJ, id, response, respLen);
      }
      else
      {
        sendLCPPacket(LCP_CONF_ACK, id, data + 4, len - 4);
        if(!lcpOpened)
        {
          lcpOpened = true;
          lcpId++;
          
          // Require PAP Authentication (Type 3, Length 4, 0xC023)
          uint8_t lcpReqData[] = { 3, 4, 0xC0, 0x23 };
          sendLCPPacket(LCP_CONF_REQ, lcpId, lcpReqData, sizeof(lcpReqData));
        }
      }
      break;
    }
    case LCP_CONF_ACK:
      if(lcpOpened)
        state = PPP_OPENED;
      break;
    case LCP_ECHO_REQ:
      sendLCPPacket(LCP_ECHO_REPLY, id, data + 4, len - 4);
      break;
    case LCP_TERM_REQ:
      sendLCPPacket(LCP_TERM_ACK, id, NULL, 0);
      switchBackToCommandMode();
      break;
  }
}

void ZPPPMode::handleIPCP(uint8_t *data, int len)
{
  if(len < 4)
    return;

  uint8_t code = data[0];
  uint8_t id = data[1];

  switch(code)
  {
    case LCP_CONF_REQ:
    {
      // If Win95 initiates IPCP, we know it's ready. Send our Req if not sent yet.
      if(!ipcpReqSent)
      {
        ipcpReqSent = true;
        ipcpId++;
        IPAddress gwIP = WiFi.gatewayIP();
        uint8_t ipReq[6] = { 3, 6, gwIP[0], gwIP[1], gwIP[2], gwIP[3] };
        sendIPCPPacket(LCP_CONF_REQ, ipcpId, ipReq, sizeof(ipReq));
      }

      uint8_t ackResp[256];
      uint8_t nakResp[256];
      uint8_t rejResp[256];
      int ackLen = 0, nakLen = 0, rejLen = 0;
      int i = 4;

      while(i < len)
      {
        uint8_t optType = data[i];
        uint8_t optLen = data[i + 1];
        if(optLen < 2) break;

        if(optType == 3) // IP Address
        {
          IPAddress wifiIP = WiFi.localIP();
          if(optLen == 6 && data[i+2] == wifiIP[0] && data[i+3] == wifiIP[1] && data[i+4] == wifiIP[2] && data[i+5] == wifiIP[3])
          {
            memcpy(ackResp + ackLen, data + i, optLen);
            ackLen += optLen;
          }
          else
          {
            nakResp[nakLen++] = 3;
            nakResp[nakLen++] = 6;
            nakResp[nakLen++] = wifiIP[0];
            nakResp[nakLen++] = wifiIP[1];
            nakResp[nakLen++] = wifiIP[2];
            nakResp[nakLen++] = wifiIP[3];
          }
        }
        else if(optType == 129) // Primary DNS
        {
          IPAddress dnsIP = WiFi.dnsIP(0);
          if(optLen == 6 && data[i+2] == dnsIP[0] && data[i+3] == dnsIP[1] && data[i+4] == dnsIP[2] && data[i+5] == dnsIP[3])
          {
            memcpy(ackResp + ackLen, data + i, optLen);
            ackLen += optLen;
          }
          else
          {
            nakResp[nakLen++] = 129;
            nakResp[nakLen++] = 6;
            nakResp[nakLen++] = dnsIP[0];
            nakResp[nakLen++] = dnsIP[1];
            nakResp[nakLen++] = dnsIP[2];
            nakResp[nakLen++] = dnsIP[3];
          }
        }
        else if(optType == 131) // Secondary DNS
        {
          IPAddress dnsIP = WiFi.dnsIP(1);
          if(optLen == 6 && data[i+2] == dnsIP[0] && data[i+3] == dnsIP[1] && data[i+4] == dnsIP[2] && data[i+5] == dnsIP[3])
          {
            memcpy(ackResp + ackLen, data + i, optLen);
            ackLen += optLen;
          }
          else
          {
            nakResp[nakLen++] = 131;
            nakResp[nakLen++] = 6;
            nakResp[nakLen++] = dnsIP[0];
            nakResp[nakLen++] = dnsIP[1];
            nakResp[nakLen++] = dnsIP[2];
            nakResp[nakLen++] = dnsIP[3];
          }
        }
        else // Unsupported option (WINS, etc)
        {
          memcpy(rejResp + rejLen, data + i, optLen);
          rejLen += optLen;
        }
        i += optLen;
      }

      if(rejLen > 0)
      {
        sendIPCPPacket(LCP_CONF_REJ, id, rejResp, rejLen);
      }
      else if(nakLen > 0)
      {
        sendIPCPPacket(LCP_CONF_NAK, id, nakResp, nakLen);
      }
      else
      {
        sendIPCPPacket(LCP_CONF_ACK, id, ackResp, ackLen);

        // Fallback: if we somehow haven't sent our Conf-Req yet, send it now.
        // (Normally handlePAP sends it first; this covers edge cases.)
        if(papPassed && !ipcpReqSent)
        {
          ipcpReqSent = true;
          ipcpId++;
          IPAddress gwIP = WiFi.gatewayIP();
          uint8_t fbReq[6] = { 3, 6, gwIP[0], gwIP[1], gwIP[2], gwIP[3] };
          sendIPCPPacket(LCP_CONF_REQ, ipcpId, fbReq, sizeof(fbReq));
        }
      }
      break;
    }
    case LCP_CONF_ACK:
      ipcpOpened = true;  // Mark IPCP complete when Win95 ACKs our Conf-Req
      break;
    case LCP_CONF_NAK:
    {
      // Resend Conf-Req
      ipcpReqSent = false;
      ipcpId++;
      IPAddress gwIP = WiFi.gatewayIP();
      uint8_t nakRetry[6] = { 3, 6, gwIP[0], gwIP[1], gwIP[2], gwIP[3] };
      sendIPCPPacket(LCP_CONF_REQ, ipcpId, nakRetry, sizeof(nakRetry));
      break;
    }
    case LCP_CONF_REJ:
    {
      // We should technically remove rejected options, but we just resend for now
      ipcpReqSent = false;
      ipcpId++;
      sendIPCPPacket(LCP_CONF_REQ, ipcpId, NULL, 0);
      break;
    }
  }
}

void ZPPPMode::handlePAP(uint8_t *data, int len)
{
  if(len < 4)
    return;

  uint8_t code = data[0];
  uint8_t id = data[1];

  // Removed debugPrintf for PPP-RX PAP

  if(code == PAP_AUTH_REQ)
  {
    int i = 4;
    
    // Extract Peer-ID
    if (i >= len) return;
    uint8_t peerIdLen = data[i++];
    char peerId[256];
    memset(peerId, 0, sizeof(peerId));
    if (i + peerIdLen > len) return;
    if (peerIdLen > 0)
      memcpy(peerId, data + i, peerIdLen);
    i += peerIdLen;

    // Extract Password
    if (i >= len) return;
    uint8_t passwdLen = data[i++];
    char passwd[256];
    memset(passwd, 0, sizeof(passwd));
    if (i + passwdLen > len) return;
    if (passwdLen > 0)
      memcpy(passwd, data + i, passwdLen);

    debugPrintf("PPP PAP: User='%s' Pass='%s'\n", peerId, passwd);

    if(strcmp(peerId, "airlink") == 0 && strcmp(passwd, "1111") == 0)
    {
      papPassed = true;
      sendPAPPacket(PAP_AUTH_ACK, id, "Login OK");
    }
    else
    {
      papPassed = false;
      sendPAPPacket(PAP_AUTH_NAK, id, "Login incorrect");
    }
  }
}

void ZPPPMode::handleIPPacket(uint8_t *data, int len)
{
  if(state != PPP_OPENED || !ipcpOpened)
    return;

  injectPacketToNetwork(data, len);
}

void ZPPPMode::processPPPFrame(uint8_t *data, int len)
{
  if(len < 6)
    return;

  uint16_t fcs = calculateFCS(data, len - 2);
  uint16_t recv_fcs = data[len - 2] | (data[len - 1] << 8);

  if(fcs != recv_fcs)
  {
    debugPrintf("PPP: FCS error\n");
    return;
  }

  if(data[0] != 0xFF || data[1] != 0x03)
    return;

  uint16_t protocol = (data[2] << 8) | data[3];
  uint8_t *payload = data + 4;
  int payloadLen = len - 6;

  switch(protocol)
  {
    case PPP_PROTOCOL_LCP:
      handleLCP(payload, payloadLen);
      break;
    case PPP_PROTOCOL_PAP:
      handlePAP(payload, payloadLen);
      break;
    case PPP_PROTOCOL_IPCP:
      handleIPCP(payload, payloadLen);
      break;
    case PPP_PROTOCOL_IP:
      handleIPPacket(payload, payloadLen);
      break;
    default:
      debugPrintf("PPP-RX: Unknown protocol 0x%04X\n", protocol);
      if(state >= PPP_ESTABLISH)
      {
        uint8_t rejPacket[128];
        rejPacket[0] = (protocol >> 8) & 0xFF;
        rejPacket[1] = protocol & 0xFF;
        int copyLen = len;
        if(copyLen > 100) copyLen = 100; // Limit payload size
        memcpy(rejPacket + 2, data, copyLen);
        lcpId++;
        sendLCPPacket(8, lcpId, rejPacket, 2 + copyLen); // 8 = LCP_PROT_REJ
      }
      break;
  }
}

void ZPPPMode::sendPacketToSerial(struct pbuf *p)
{
  if(p == NULL || state != PPP_OPENED || !ipcpOpened)
    return;

  if (qosBps > 0) {
    unsigned long now = millis();
    unsigned long elapsed = now - qosLastTime;
    if (elapsed > 0) {
      qosTokens += (float)elapsed * ((float)qosBps / 8000.0);
      if (qosTokens > (float)(qosBps / 8)) qosTokens = (float)(qosBps / 8);
      qosLastTime = now;
    }
    if (qosTokens < p->tot_len) {
      return; // DROP PACKET (Tail Drop)
    }
    qosTokens -= p->tot_len;
  }

  struct pbuf *q = p;
  int total = 0;
  uint8_t tempBuf[1600];

  while(q != NULL)
  {
    uint8_t *payload = (uint8_t *)q->payload;
    int len = q->len;
    memcpy(tempBuf + total, payload, len);
    total += len;
    q = q->next;
  }

  if(total >= 20)
  {
    // Removed debugPrintf for PPP-RX IP packet
  }

  sendPPPFrame(PPP_PROTOCOL_IP, tempBuf, total);
}

void ZPPPMode::injectPacketToNetwork(uint8_t *data, int len)
{
  if(len < 20)
    return;

  if (qosBps > 0) {
    unsigned long now = millis();
    unsigned long elapsed = now - qosLastTime;
    if (elapsed > 0) {
      qosTokens += (float)elapsed * ((float)qosBps / 8000.0);
      if (qosTokens > (float)(qosBps / 8)) qosTokens = (float)(qosBps / 8);
      qosLastTime = now;
    }
    if (qosTokens < len) {
      return; // DROP PACKET (Tail Drop)
    }
    qosTokens -= len;
  }

  // Removed debugPrintf for PPP-TX IP packet

  if(wifi_netif == NULL || original_wifi_output == NULL)
    return;

  struct pbuf *p = pbuf_alloc(PBUF_IP, len, PBUF_RAM);
  if(p == NULL)
    return;

  memcpy(p->payload, data, len);

  ip4_addr_t dest;
  IP4_ADDR(&dest, data[16], data[17], data[18], data[19]);

  LOCK_TCPIP_CORE();
  err_t err = original_wifi_output(wifi_netif, p, &dest);
  UNLOCK_TCPIP_CORE();

  if(err != ERR_OK)
    pbuf_free(p);
}

void ZPPPMode::switchBackToCommandMode()
{
  debugPrintf("\r\nMode:Command\r\n");

  if(wifi_netif != NULL && original_wifi_output != NULL)
    wifi_netif->output = original_wifi_output;
  if(wifi_netif != NULL && original_wifi_input != NULL)
    wifi_netif->input = original_wifi_input;

  LOCK_TCPIP_CORE();
  netif_remove(&ppp_netif);
  UNLOCK_TCPIP_CORE();
  currMode = &commandMode;

  if(this->buf != 0)
  {
    free(this->buf);
    this->buf = 0;
  }
}

void ZPPPMode::switchTo()
{
  debugPrintf("\r\nMode:PPP\r\n");
  sserial.setFlowControlType(FCT_DISABLED);
  if(commandMode.getFlowControlType()==FCT_RTSCTS)
    sserial.setFlowControlType(FCT_RTSCTS);
  sserial.setPetsciiMode(false);
  sserial.setXON(true);

  this->curBufLen = 0;
  this->escaped = false;
  this->state = PPP_ESTABLISH;
  this->lcpOpened = false;
  this->ipcpOpened = false;
  this->ipcpReqSent = false;
  this->papPassed = false;
  this->lcpId = 0;
  this->ipcpId = 0;

  yield();
  delay(999);

  while(HWSerial.available() > 0)
    HWSerial.read();

  wifi_netif = netif_list;
  while(wifi_netif != NULL)
  {
    if((wifi_netif->name[0] == 's' && wifi_netif->name[1] == 't')
    || (wifi_netif->name[0] == 'e' && wifi_netif->name[1] == 'n'))
      break;
    wifi_netif = wifi_netif->next;
  }

  if(wifi_netif == NULL)
    wifi_netif = netif_default;

  if(wifi_netif != NULL)
  {
    debugPrintf("PPP: Using interface %c%c%d\n",
                wifi_netif->name[0], wifi_netif->name[1], wifi_netif->num);

    original_wifi_output = wifi_netif->output;
    wifi_netif->output = ppp_wifi_output_hook;

    original_wifi_input = wifi_netif->input;
    wifi_netif->input = ppp_wifi_input_hook;
  }

  ip4_addr_t ipaddr, netmask, gw;
  IPAddress wifiIP = WiFi.localIP();
  IPAddress wifiGW = WiFi.gatewayIP();
  IPAddress wifiMask = WiFi.subnetMask();

  if(wifiIP[0] != 0)
  {
    IP4_ADDR(&ipaddr, wifiIP[0], wifiIP[1], wifiIP[2], wifiIP[3]);
    IP4_ADDR(&gw, wifiGW[0], wifiGW[1], wifiGW[2], wifiGW[3]);
    IP4_ADDR(&netmask, wifiMask[0], wifiMask[1], wifiMask[2], wifiMask[3]);
  }
  else
  {
    IP4_ADDR(&ipaddr, 192, 168, 1, 1);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gw, 192, 168, 1, 1);
  }

  debugPrintf("PPP: Using WiFi IP: %s\n", ip4addr_ntoa(&ipaddr));
  debugPrintf("PPP: Using WiFi Gateway: %s\n", ip4addr_ntoa(&gw));
  debugPrintf("PPP: Using WiFi Netmask: %s\n", ip4addr_ntoa(&netmask));

  LOCK_TCPIP_CORE();
  netif_add(&ppp_netif, &ipaddr, &netmask, &gw, NULL, ppp_netif_init, ip_input);
  netif_set_up(&ppp_netif);
  netif_set_link_up(&ppp_netif);
  UNLOCK_TCPIP_CORE();

  currMode = &pppMode;
  debugPrintf("PPP mode active\n\r");
}

void ZPPPMode::serialIncoming()
{
  while(HWSerial.available() > 0)
  {
    uint8_t c = HWSerial.read();

    if(logFileOpen)
      logSerialIn(c);

    if(this->buf == 0)
    {
      this->buf = (uint8_t *)malloc(4096);
      if(this->buf == 0)
        return;
      this->maxBufSize = 4096;
      this->curBufLen = 0;
    }

    if(c == PPP_FLAG)
    {
      if(this->curBufLen > 0)
      {
        processPPPFrame(this->buf, this->curBufLen);
        this->curBufLen = 0;
        this->escaped = false;
      }
    }
    else
    if(c == PPP_ESCAPE)
    {
      this->escaped = true;
    }
    else
    {
      if(this->escaped)
      {
        c ^= PPP_TRANS;
        this->escaped = false;
      }

      if(this->curBufLen < this->maxBufSize)
        this->buf[this->curBufLen++] = c;
    }

    if(this->curBufLen >= this->maxBufSize)
    {
      int newSize = this->maxBufSize * 2;
      if(newSize > 65536)
      {
        debugPrintf("PPP: packet too large, discarding\n");
        this->curBufLen = 0;
        this->escaped = false;
      }
      else
      {
        uint8_t *newBuf = (uint8_t *)malloc(newSize);
        if(newBuf != NULL)
        {
          memcpy(newBuf, this->buf, this->curBufLen);
          maxBufSize = newSize;
          free(this->buf);
          this->buf = newBuf;
        }
        else
          this->curBufLen = 0;
      }
    }
  }
}

void ZPPPMode::loop()
{
  if(checkPlusPlusPlusEscape())
  {
    switchBackToCommandMode();
  }
  else if(sserial.isSerialOut())
    serialOutDeque();
  logFileLoop();
}

#endif
