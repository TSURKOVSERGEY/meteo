////////////////////////////////////////////////////////////////////////////////
// ETHERNET * ETHERNET * ETHERNET * ETHERNET * ETHERNET * ETHERNET * ETHERNET * 
////////////////////////////////////////////////////////////////////////////////

#define MAC_ADDR0       0x00 
#define MAC_ADDR1       0x40
#define MAC_ADDR2       0x45
#define MAC_ADDR3       0x31
#define MAC_ADDR4       0x32
#define MAC_ADDR5       IP_ADDR3

#define IP_ADDR0        192
#define IP_ADDR1        9
#define IP_ADDR2        206
#define IP_ADDR3        205

#define NETMASK_ADDR0   255
#define NETMASK_ADDR1   255
#define NETMASK_ADDR2   255
#define NETMASK_ADDR3   0

#define GW_ADDR0        IP_ADDR0
#define GW_ADDR1        IP_ADDR1
#define GW_ADDR2        IP_ADDR2
#define GW_ADDR3        IP_ADDR3

#define STM_PORT_TX_TCP   30000
#define STM_PORT_RX_TCP   30000
#define STM_PORT_TX_UDP   30785

void UDP_Config(void);
void SendMessage(uint8_t* pbuffer, int len);
void ReSendUdpData(void);
