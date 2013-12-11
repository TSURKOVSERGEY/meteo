  


//  if(buffer_len < 12) return 0;
 
 // else if(buffer_len < (prx_tcp_msg->msg_len + 12)) return 0;
  

   
  // return prx_tcp_msg->msg_len + 12;

  


  /*
  // if we receive an empty tcp frame from server => close connection 
  if (p == NULL)
  {
    // remote host closed connection 
    es->state = ES_CLOSING;
    if(es->p_tx == NULL)
    {
       // we're done sending, close connection  
       tcp_echoclient_connection_close(tpcb, es);
    }
    else
    {    
      // send remaining data
      tcp_echoclient_send(tpcb, es);
    }
    ret_err = ERR_OK;
  }   
  else if(err != ERR_OK)
  {
    if (p != NULL) // free received pbuf
    {
      pbuf_free(p);
    }
    ret_err = err;
  }
  else if(es->state == ES_CONNECTED)
  {
    tcp_recved(tpcb, p->tot_len);  // Acknowledge data reception 
    pbuf_free(p);
  //  tcp_echoclient_connection_close(tpcb, es);
    SetPageReadAdr(1,1);
    ret_err = ERR_OK;
  }
  else // data received when connection already closed 
  {
    tcp_recved(tpcb, p->tot_len);  
    pbuf_free(p);
    SetPageReadAdr(1,1);
    ret_err = ERR_OK;
  }
  
  return ret_err;
  */




err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{ 

  static uint8_t header[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
      
  tcp_message_struct *prx_tcp_msg = (tcp_message_struct*)header;
      
  struct pbuf *pbuf = p;
  u16_t tot_len = p->tot_len;
  uint8_t *pdata = (uint8_t*)p->payload; 
  err_t ret_err = ERR_OK;
  int i = 0, brr = 0;
  
  if(err != ERR_OK)
  {
    if (p != NULL) 
    {
      pbuf_free(p);
    }
    ret_err = err;
  }
 
  if(tot_len > TCP_MESSAGE_SIZE+12)
  { pbuf_free(p);
    return ret_err;
  }
  
  while(tot_len > 0)
  { 
    
    if(brr == 0)
    {
      for(i = 0; i < 11; i++) 
      {
        header[i] = header[i+1];
      }
    
      header[11] = *pdata++; 
      
      tot_len--;
      
      if(pbuf->len-- == 0) 
      {
        pbuf = pbuf->next;
        pdata = (uint8_t*)pbuf->payload; 
      }
      
      if(prx_tcp_msg->msg_id == 0xabcd)
      {
        if(prx_tcp_msg->msg_len <= TCP_MESSAGE_SIZE) 
        {
          brr = 1;
          memcpy(&rx_tcp_msg,header,12);
        }
      }
    }
    else if(brr < prx_tcp_msg->msg_len)
    {
      rx_tcp_msg.data[brr-1] = *pdata;
      brr++;
      pdata++;
      tot_len--;
      if(pbuf->len-- == 0) 
      {
        pbuf = pbuf->next;
        pdata = (uint8_t*)pbuf->payload; 
      }
    }
    else
    {
      rx_tcp_msg.data[brr-1] = *pdata;
      pdata++;
      tot_len--;
      if(pbuf->len-- == 0) 
      {
        pbuf = pbuf->next;
        pdata = (uint8_t*)pbuf->payload; 
      }
      tcp_msg_handler();
      prx_tcp_msg->msg_id = 0;
      brr = 0;
    }
  }
  
  tcp_recved(tpcb, p->tot_len);
  pbuf_free(p);
  GPIO_ToggleBits(GPIOI, GPIO_Pin_0); 
    
  return ret_err;

}
