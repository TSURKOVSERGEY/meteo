#include "main.h"
#include "ethernet.h"
#include "spi_f207.h"
#include "at24c512.h"

extern tcp_message_struct  tx_tcp_msg;
extern tcp_message_struct  rx_tcp_msg;
extern int rew_status;


struct pbuf    *pOut;
struct tcp_pcb *ptcp_pcb;
struct tcp_pcb *pout_pcb;

struct udp_pcb *pudp_pcb;

struct ip_addr ip_addr_tx;

void *repbuffer;

ethernet_initial_struct eth_ini_dat;

int re_len;

uint8_t  udp_buffer[1024];
uint32_t udp_len;

void SendTcpMessage(unsigned int msg_id, void* data, unsigned int len)
{
  if(data)memcpy(&tx_tcp_msg.data[0],data,len);
  
  tx_tcp_msg.msg_id  = msg_id;
  tx_tcp_msg.msg_len = len;
  tx_tcp_msg.msg_crc = crc32((uint8_t*)&tx_tcp_msg+4,(TCP_HEADER_SIZE-4) + tx_tcp_msg.msg_len,TCP_MESSAGE_SIZE); 
  SendTcpData(&tx_tcp_msg,TCP_HEADER_SIZE+tx_tcp_msg.msg_len);
}

void SendUdpData(uint8_t* pbuffer, int len)
{
 
  pbuf_free(pOut);
  pOut = pbuf_alloc(PBUF_TRANSPORT,len,PBUF_RAM);
  
  pOut->tot_len = len;
  
  udp_len = len;

  memcpy(udp_buffer,pbuffer,len);
     
  if(pbuf_take(pOut,udp_buffer,len) == ERR_OK)
  {
     if(udp_sendto(pudp_pcb,pOut,IP_ADDR_BROADCAST,30785) != ERR_OK)  rew_status = 2;
  }
  else rew_status = 1;

}

void ReSendUdpData(void)
{
  if(rew_status == 1)
  {  
    pOut->tot_len = udp_len;
    
    if(pbuf_take(pOut,udp_buffer,udp_len) == ERR_OK)
    {
      if(udp_sendto(pudp_pcb,pOut,IP_ADDR_BROADCAST,30785) != ERR_OK)  rew_status = 2;
    }
  }
  else if(rew_status == 2)
  {
    if(udp_sendto(pudp_pcb,pOut,IP_ADDR_BROADCAST,30785) == ERR_OK) rew_status = 0;
  
  }
}


void ReSendTcpData(int mode)
{
  if(rew_status == 1)
  {
    if(tcp_write(pout_pcb,&tx_tcp_msg,tx_tcp_msg.msg_len + TCP_HEADER_SIZE+2,1) == ERR_OK) 
    {
      if(tcp_output(pout_pcb) == ERR_OK)
      { tcp_sent(pout_pcb,tcp_callback_sent);  
        rew_status = 0;
      }
     else rew_status = 2; 
    }
  }
  else if(rew_status == 2)
  {
    if(tcp_output(pout_pcb) == ERR_OK) rew_status = 0;
  }  
}

void SendTcpData(void* pbuffer, int len)
{
  if(tcp_write(pout_pcb,pbuffer,len+2,1) == ERR_OK) 
  {
    if(tcp_output(pout_pcb) != ERR_OK) rew_status = 2; 
    else tcp_sent(pout_pcb,tcp_callback_sent);  
  }
  else rew_status = 1;
}


int LWIP_Config(void)
{
  
  //AT45_Read(0,(uint8_t*)&eth_ini_dat,sizeof(ethernet_initial_struct));
  
  if(crc32(&eth_ini_dat,sizeof(eth_ini_dat)-4,sizeof(eth_ini_dat)-4) != eth_ini_dat.crc)
  {
    eth_ini_dat.IP_ADR[0] = IP_ADDR0;
    eth_ini_dat.IP_ADR[1] = IP_ADDR1;
    eth_ini_dat.IP_ADR[2] = IP_ADDR2;
    eth_ini_dat.IP_ADR[3] = IP_ADDR3;
    
    eth_ini_dat.GW_ADR[0] = GW_ADDR0;
    eth_ini_dat.GW_ADR[1] = GW_ADDR1;
    eth_ini_dat.GW_ADR[2] = GW_ADDR2;
    eth_ini_dat.GW_ADR[3] = GW_ADDR3;
      
    eth_ini_dat.MASK[0] = NETMASK_ADDR0;
    eth_ini_dat.MASK[1] = NETMASK_ADDR1;
    eth_ini_dat.MASK[2] = NETMASK_ADDR2;
    eth_ini_dat.MASK[3] = NETMASK_ADDR3;

    eth_ini_dat.MAC_ADR[0] = MAC_ADDR0;
    eth_ini_dat.MAC_ADR[1] = MAC_ADDR1;
    eth_ini_dat.MAC_ADR[2] = MAC_ADDR2;
    eth_ini_dat.MAC_ADR[3] = MAC_ADDR3;
    eth_ini_dat.MAC_ADR[4] = MAC_ADDR4;
    eth_ini_dat.MAC_ADR[5] = MAC_ADDR5;
    
    eth_ini_dat.TCP_RX_PORT = STM_PORT_RX_TCP;
    eth_ini_dat.UDP_RX_PORT = STM_PORT_TX_UDP;


  }
  else
  {
  }
    
  if(ETH_BSP_Config() == ETH_ERROR) return 0;
   
  LwIP_Init();
    
  return 1;
}


void UDP_Config(void)
{
  if((pudp_pcb = udp_new()) == NULL)  return;

  if((udp_bind(pudp_pcb,IP_ADDR_ANY,eth_ini_dat.UDP_RX_PORT) != ERR_OK))  return;
 // if((udp_bind(pudp_pcb,IP_ADDR_BROADCAST,eth_ini_dat.UDP_RX_PORT) != ERR_OK))  return;
  //udp_recv(pudp_pcb,&udp_rx_handler,NULL);

  pOut = pbuf_alloc(PBUF_TRANSPORT,1024,PBUF_RAM);
}


int TCP_SERVER_Config(void)
{
  if((ptcp_pcb = tcp_new()) != NULL)
  {
     if ((tcp_bind(ptcp_pcb, IP_ADDR_ANY,eth_ini_dat.TCP_RX_PORT)) == ERR_OK)
     { ptcp_pcb = tcp_listen(ptcp_pcb);
       tcp_accept(ptcp_pcb,tcp_callback_accept);
     }
  }
  return 0;
}

err_t tcp_callback_accept(void *arg, struct tcp_pcb *tpcb, err_t err)
{
  err_t ret_err;
  
  struct tcp_struct *es;

  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(err);

  tcp_setprio(tpcb, TCP_PRIO_MIN);

  es = (struct tcp_struct *)mem_malloc(sizeof(struct tcp_struct));
  
  if (es != NULL)
  {
    es->state = ES_ACCEPTED;
    es->pcb = tpcb;
    es->p = NULL;
    
    tcp_arg(tpcb, es);
    tcp_recv(tpcb, tcp_callback_recv);     
    tcp_sent(tpcb, tcp_callback_sent);     
    tcp_poll(tpcb, tcp_callback_poll,1);   
    ret_err = ERR_OK;
  }
  else
  {
    ret_err = ERR_MEM;
  }
  return ret_err;  
}


err_t tcp_callback_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
  struct tcp_struct *es = NULL;
 
  if(err == ERR_OK)   
  {
    es = (struct tcp_struct *)mem_malloc(sizeof(struct tcp_struct));
  
    if(es != NULL)
    {
      es->state = ES_CONNECTED;
    
      es->pcb = tpcb;
      
      if(es->p)
      {       
       
        tcp_arg(tpcb, es);
        tcp_recv(tpcb, tcp_callback_recv);     // initialize LwIP tcp_recv callback function 
        tcp_sent(tpcb, tcp_callback_sent);     // initialize LwIP tcp_sent callback function 
        tcp_poll(tpcb, tcp_callback_poll,2);   // initialize LwIP tcp_poll callback function 
         
        char buffer[] = "set client connection";      
        es->p = pbuf_alloc(PBUF_TRANSPORT, strlen(buffer) , PBUF_POOL);
        pbuf_take(es->p, buffer, strlen(buffer));
        tcp_send(tpcb,es);   
        
        return ERR_OK;
      }
    }
    else
    {
      tcp_connection_close(tpcb, es);
      return ERR_MEM;  
    }
  }
  else
  {
    tcp_connection_close(tpcb, es);
  }
  return err;
}


err_t tcp_callback_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
  struct tcp_struct *es;

  es = (struct tcp_struct *)arg;
    
  if(es->p != NULL)
  {
    tcp_sent(tpcb,tcp_callback_sent); 
    tcp_send(tpcb,es);
  }
  else
  {
    if(es->state == ES_CLOSING)
    {
      tcp_connection_close(tpcb, es);
    }
  }

  return ERR_OK;
}


err_t tcp_callback_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{ 
  struct tcp_struct *es;
  static int i = 0, j = 0, m = 0, n = 0;
  struct pbuf *pbuffer = p;
  static uint8_t buffer[4096];
  uint8_t *prx_tcp_msg;\
    
  
  es = (struct tcp_struct*)arg;
  

  if(err != ERR_OK)
  {
    if (p != NULL) 
    {
      pbuf_free(p);
    }
    return err;
  }
  
  if(es->pcb->state == CLOSE_WAIT)
  {
    if((tcp_send_ctrl(tpcb, TCP_FIN)) == ERR_OK)
    {
      es->pcb->state = LAST_ACK;  
      tcp_output(tpcb);
      tcp_connection_close(tpcb,es);
    }
    return ERR_OK;
  }
  
  while(1)
  {
    for(m = 0; m < pbuffer->len; m++)
    {
      buffer[n++&0xfff] = *(((uint8_t*)pbuffer->payload)+m);
    }
      
    if(pbuffer->next == NULL) break;
    else pbuffer = pbuffer->next;
  }

  prx_tcp_msg = (uint8_t*)&rx_tcp_msg;
     
  for(m  = 0; m < p->tot_len; m++)  
  {
    prx_tcp_msg[i++] = buffer[j++&0xfff];
    
    if(i >= 12)
    {
      if(i == rx_tcp_msg.msg_len + 12)
      { i = 0;
        pout_pcb = tpcb;
        eth_cmd_handler();
      }
    }
  }
   
  tcp_recved(tpcb,p->tot_len);
  pbuf_free(p);

  return ERR_OK;

}

void tcp_send(struct tcp_pcb *tpcb, struct tcp_struct * es)
{
  struct pbuf *ptr;
  
  err_t wr_err = ERR_OK;
 
  while ((wr_err == ERR_OK) &&
         (es->p != NULL) && 
         (es->p->len <= tcp_sndbuf(tpcb)))
  {
    
    ptr = es->p;

    wr_err = tcp_write(tpcb, ptr->payload, ptr->len, 1);
    
    if (wr_err == ERR_OK)
    { 
      es->p = ptr->next;
      
      if(es->p != NULL)
      {
        pbuf_ref(es->p);
      }
      
      pbuf_free(ptr);
   }
   else if(wr_err == ERR_MEM)
   {
     es->p = ptr;
   }
   else
   {
     delay(0);
   }
  }
}


void tcp_connection_close(struct tcp_pcb *tpcb, struct tcp_struct *es)
{
  tcp_recv(tpcb, NULL);
  tcp_sent(tpcb, NULL);
  tcp_poll(tpcb, NULL,0);

  if (es != NULL)
  {
    mem_free(es);
  }

  tcp_close(tpcb);
}



err_t tcp_callback_poll(void *arg, struct tcp_pcb *tpcb)
{
  err_t ret_err;
  struct tcp_struct *es;

  es = (struct tcp_struct*)arg;
  if (es != NULL)
  {
    if (es->p != NULL)
    {
      tcp_send(tpcb, es);
    }
    else
    {
      if(es->state == ES_CLOSING)
      {
        tcp_connection_close(tpcb,es);
      }
    }
    ret_err = ERR_OK;
  }
  else
  {
    tcp_abort(tpcb);
    ret_err = ERR_ABRT;
  }
  return ret_err;
}

