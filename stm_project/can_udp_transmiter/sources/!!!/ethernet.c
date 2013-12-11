#include "main.h"
#include "ethernet.h"
#include "spi_f207.h"

struct pbuf               *pOut;
udp_message_struct         tx_udp_msg;
udp_message_struct         rx_udp_msg;
tcp_message_struct         tx_tcp_msg;
tcp_message_struct         rx_tcp_msg;


extern struct udp_pcb *pudp_pcb;
extern struct tcp_pcb *ptcp_pcb;
 
struct ip_addr ip_addr_tx;


void UDP_Config(void)
{
  
  if(ETH_BSP_Config() == ETH_ERROR) CriticalError(ERR_ETH_CONFIG);
  
  LwIP_Init();
  
  delay(500);
  
  IP4_ADDR(&ip_addr_tx,192,9,206,251);
  
  if((pudp_pcb = udp_new()) == NULL) CriticalError(ERR_UDP_NEW);
 
  if((udp_bind(pudp_pcb,IP_ADDR_ANY,RX_PORT) != ERR_OK)) CriticalError(ERR_UDP_BIND);
 
  udp_recv(pudp_pcb,&udp_rx_handler,NULL);

  if(!(pOut = pbuf_alloc(PBUF_TRANSPORT,sizeof(tx_udp_msg),PBUF_RAM))) CriticalError(ERR_POUT_NEW);
}

int TCP_CLIENT_Config(void)
{ 
  if(ETH_BSP_Config() == ETH_ERROR) CriticalError(ERR_ETH_CONFIG);
  
  LwIP_Init();
  
  delay(500);
  
  if((ptcp_pcb = tcp_new()) == NULL) CriticalError(ERR_UDP_NEW);
   
  IP4_ADDR(&ip_addr_tx,192,9,206,251);
  
  if(tcp_connect(ptcp_pcb,&ip_addr_tx,TX_PORT,tcp_echoclient_connected) == ERR_OK) 
  {
    pbuf_free(ptcp_pcb->refused_data);
    return 0;
  }
  else return 1;
}

static err_t tcp_echoclient_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
  struct tcp_echoclient_struct *es = NULL;
  
  if (err == ERR_OK)   
  {
    es = (struct tcp_echoclient_struct *)mem_malloc(sizeof(struct tcp_echoclient_struct));
  
    if (es != NULL)
    {
      es->state = ES_CONNECTED;
      es->pcb = tpcb;
      
      if (es->p_tx)
      {       

        tcp_arg(tpcb, es);
         
        tcp_recv(tpcb, http_recv);     // initialize LwIP tcp_recv callback function 
  
        tcp_sent(tpcb, http_sent);     // initialize LwIP tcp_sent callback function 
       
        tcp_poll(tpcb, tcp_echoclient_poll, 1);  // initialize LwIP tcp_poll callback function 
    
        return ERR_OK;
      }
    }
    else
    {
      tcp_echoclient_connection_close(tpcb, es);
      return ERR_MEM;  
    }
  }
  else
  {
    tcp_echoclient_connection_close(tpcb, es);
  }
  return err;
}

/////////////////////////////////////////////////////////////////////////////////


static err_t  (void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
  
  struct pbuf *pbuf = p;
  u16_t tot_len = pbuf->tot_len;
  uint8_t* pmsg; 
  
  
  if(tot_len > sizeof(rx_tcp_msg)) 
  { pbuf_free(p);
    return ERR_OK;
  }
  
  while(tot_len > 0)
  {   
    if(tot_len == p->tot_len)
    { pmsg = (uint8_t*)&rx_tcp_msg;
      memcpy(pmsg,(uint8_t*)pbuf->payload,pbuf->len);
      pmsg += pbuf->len;
    }
    else
    { memcpy(pmsg,(uint8_t*)pbuf->payload,pbuf->len);
      pmsg += pbuf->len;
    }
     tot_len -= pbuf->len;
     pbuf = pbuf->next;
   }
  
  pbuf_free(p);
  
  tcp_rx_handler();
  
  return ERR_OK;
}



static err_t http_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
  struct tcp_echoserver_struct *es;

  LWIP_UNUSED_ARG(len);

  es = (struct tcp_echoserver_struct *)arg;
  
  if(es->p != NULL)
  {
     tcp_echoserver_send(tpcb, es);
  }
  else
  {
    if(es->state == ES_CLOSING)
     tcp_echoserver_connection_close(tpcb, es);
  }
  return ERR_OK;
}


static void tcp_echoserver_send(struct tcp_pcb *tpcb, struct tcp_echoserver_struct *es)
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
      u16_t plen;

      plen = ptr->len;
     
      es->p = ptr->next;
      
      if(es->p != NULL)
      {
        pbuf_ref(es->p);
      }
      
      pbuf_free(ptr);

      /* Update tcp window size to be advertized : should be called when received
      data (with the amount plen) has been processed by the application layer */
      tcp_recved(tpcb, plen);
   }
   else if(wr_err == ERR_MEM)
   {
      /* we are low on memory, try later / harder, defer to poll */
     es->p = ptr;
   }
   else
   {
     /* other problem ?? */
   }
  }
}

static void tcp_echoclient_send(struct tcp_pcb *tpcb, struct tcp_echoclient_struct *es)
{
  struct pbuf *ptr;
  err_t wr_err = ERR_OK;
 
  while ((wr_err == ERR_OK) &&
         (es->p_tx != NULL) && 
         (es->p_tx->len <= tcp_sndbuf(tpcb)))
  {
    
    /* get pointer on pbuf from es structure */
    ptr = es->p_tx;

    /* enqueue data for transmission */
    wr_err = tcp_write(tpcb, ptr->payload, ptr->len, 1);
    
    if (wr_err == ERR_OK)
    { 
      /* continue with next pbuf in chain (if any) */
      es->p_tx = ptr->next;
      
      if(es->p_tx != NULL)
      {
        /* increment reference count for es->p */
        pbuf_ref(es->p_tx);
      }
      
      /* free pbuf: will free pbufs up to es->p (because es->p has a reference count > 0) */
      pbuf_free(ptr);
   }
   else if(wr_err == ERR_MEM)
   {
      /* we are low on memory, try later, defer to poll */
     es->p_tx = ptr;
   }
   else
   {
     /* other problem ?? */
   }
  }
}

static void tcp_echoserver_connection_close(struct tcp_pcb *tpcb, struct tcp_echoserver_struct *es)
{
  
  tcp_arg(tpcb, NULL);
  tcp_sent(tpcb, NULL);
  tcp_recv(tpcb, NULL);
  tcp_err(tpcb, NULL);
  tcp_poll(tpcb, NULL, 0);
  
  if (es != NULL)
  {
    mem_free(es);
  }  
  
  tcp_close(tpcb);
}

static void tcp_echoclient_connection_close(struct tcp_pcb *tpcb, struct tcp_echoclient_struct *es)
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


static err_t tcp_echoclient_poll(void *arg, struct tcp_pcb *tpcb)
{
  err_t ret_err;
  struct tcp_echoclient_struct *es;

  es = (struct tcp_echoclient_struct*)arg;
  if (es != NULL)
  {
    if (es->p_tx != NULL)
    {
      /* there is a remaining pbuf (chain) , try to send data */
      tcp_echoclient_send(tpcb, es);
    }
    else
    {
      /* no remaining pbuf (chain)  */
      if(es->state == ES_CLOSING)
      {
        /* close tcp connection */
        tcp_echoclient_connection_close(tpcb,es);
      }
    }
    ret_err = ERR_OK;
  }
  else
  {
    /* nothing to be done */
    tcp_abort(tpcb);
    ret_err = ERR_ABRT;
  }
  return ret_err;
}

