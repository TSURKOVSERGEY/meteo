#include "main.h"
#include "ethernet.h"
#include "spi_f207.h"


extern tcp_message_struct  tx_tcp_msg;
extern tcp_message_struct  rx_tcp_msg;
extern int rew_status;


struct pbuf    *pOut;
struct tcp_pcb *ptcp_pcb;
struct tcp_pcb *pout_pcb;
struct udp_pcb *pudp_pcb;

struct ip_addr ip_addr_tx;

void *repbuffer;

int   re_len;

void ReSendData(int mode)
{
  if(mode == 1)
  {
    if(pbuf_take(pOut,(uint8_t*)repbuffer,re_len) == ERR_OK)
    {
      if(udp_sendto(pudp_pcb,pOut,&ip_addr_tx,STM_PORT_A) != ERR_OK)  rew_status = 2;
    }
  }
  else if(mode == 2)
  {
    if(udp_sendto(pudp_pcb,pOut,&ip_addr_tx,STM_PORT_A) == ERR_OK) rew_status = 0;
  
  }
}

void SendData(void* pbuffer, int len)
{
 
  
  repbuffer = pbuffer;
  re_len = len;
           
  if(pbuf_take(pOut,(uint8_t*)pbuffer,len) == ERR_OK)
  {
     pOut->tot_len = len;
     
     if(udp_sendto(pudp_pcb,pOut,&ip_addr_tx,STM_PORT_A) != ERR_OK)  rew_status = 2;
  }
  else rew_status = 1;

}


void UDP_Config(void)
{
  
  if(ETH_BSP_Config() == ETH_ERROR) return;
  
  LwIP_Init();
  
  delay(500);
  
  IP4_ADDR(&ip_addr_tx,192,9,206,251);
  
  if((pudp_pcb = udp_new()) == NULL)  return;
 
  if((udp_bind(pudp_pcb,IP_ADDR_ANY,STM_PORT_A) != ERR_OK))  return;
 
  udp_recv(pudp_pcb,&udp_rx_handler,NULL);

  if(!(pOut = pbuf_alloc(PBUF_TRANSPORT,sizeof(tx_tcp_msg),PBUF_RAM)))  return;
}

#ifdef TCP

int TCP_CLIENT_Config(void)
{ 
  if(ETH_BSP_Config() == ETH_ERROR) return 0;
   
  LwIP_Init();
  
  delay(500);
  
  if((ptcp_pcb = tcp_new()) == NULL) return 0;
   
  IP4_ADDR(&ip_addr_tx,192,9,206,251);
  
  if(tcp_connect(ptcp_pcb,&ip_addr_tx,STM_PORT_A,tcp_callback_connected) == ERR_OK) 
  {
    pbuf_free(ptcp_pcb->refused_data);
    return 0;
  }
  else return 1;
}

int TCP_SERVER_Config(void)
{
  if(ETH_BSP_Config() == ETH_ERROR) return 0;
   
  LwIP_Init();
  
  delay(500);
  
  if((ptcp_pcb = tcp_new()) != NULL)
  {
     if ((tcp_bind(ptcp_pcb, IP_ADDR_ANY,STM_PORT_A)) == ERR_OK)
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
        tcp_rx_handler();
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

#endif