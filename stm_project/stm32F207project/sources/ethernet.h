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
#define IP_ADDR3        202

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


enum states
{
  ES_NONE = 0,
  ES_NOT_CONNECTED, 
  ES_CONNECTED,
  ES_ACCEPTED,
  ES_RECEIVED,
  ES_CLOSING
};


struct tcp_struct
{ 
  enum   states state;          /* connection status */
  struct tcp_pcb *pcb;          /* pointer on the current tcp_pcb */
  struct pbuf *p;               /* pointer on pbuf to be transmitted */    
};


void UDP_Config(void);
int TCP_CLIENT_Config(void);
int TCP_SERVER_Config(void);


void udp_rx_handler(void *arg, struct udp_pcb *upcb,struct pbuf *p, struct ip_addr *addr, u16_t port);
void tcp_msg_recovery(uint8_t bin[], uint8_t bout[], int len);

err_t tcp_callback_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
err_t tcp_callback_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
err_t tcp_callback_poll(void *arg, struct tcp_pcb *tpcb);
err_t tcp_callback_connected(void *arg, struct tcp_pcb *pcb, err_t err);
err_t tcp_callback_accept(void *arg, struct tcp_pcb *newpcb, err_t err);

void tcp_send(struct tcp_pcb *tpcb, struct tcp_struct *es);
void tcp_connection_close(struct tcp_pcb *tpcb, struct tcp_struct *es);


void SendTcpMessage(unsigned int msg_id, void* data, unsigned int len);