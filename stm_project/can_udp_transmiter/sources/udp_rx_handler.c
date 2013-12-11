#include "main.h"
#include "debug.h"

#include "nand_hw_driver.h"

extern uint8_t rls_msg_buffer[1024];
extern initial_data_struct initial_data;
extern int page_per_az;

   
#define MAX_PAGE  5 

uint8_t data_buffer[MAX_PAGE][2048];
  
void udp_rx_handler(void *arg, struct udp_pcb *upcb,struct pbuf *p, struct ip_addr *addr, u16_t port)
{
  static int i = 0, j = 0, m = 0, n = 0;
  static uint8_t buffer[2048];
  struct pbuf *pbuffer = p;

  uint16_t AZ;
  uint16_t EL;
  
  uint16_t IAZ;
  uint16_t IEL;
    
  float AZ_f;
  float EL_f; 
  
  uint32_t page_adr;
  uint32_t adr;
  
  uint16_t *pdata; 
    
  
  while(1)
  {
    for(i = 0; i < pbuffer->len; i++)
    {
      buffer[n++&0x7ff] = *(((uint8_t*)pbuffer->payload)+i);
    }
      
    if(pbuffer->next == NULL) break;
    else pbuffer = pbuffer->next;
  }
   
       
  for(j  = 0; j < p->tot_len; j++) 
  {
    rls_msg_buffer[j] = buffer[m++&0x7ff];
  }
  

  if((strcmp((char*)&rls_msg_buffer[4],"RCV02")) == 0)
  {
    if(rls_msg_buffer[16] == 0x80) 
    {
      AZ = 0;
      AZ = rls_msg_buffer[18] << 7;
      AZ = AZ | rls_msg_buffer[17];

      EL = 0;
      EL = rls_msg_buffer[20] << 7;
      EL = EL | rls_msg_buffer[19];
      
      AZ_f  = (float)AZ * 360.0 / 16384.0;
      EL_f  = (float)EL * 360.0 / 16384.0;
      
      IAZ = (uint16_t)(AZ_f / initial_data.DAZ);
      IEL = (uint16_t)(EL_f / initial_data.DEL);
      
      page_adr = (IEL * initial_data.NAZ * page_per_az) + (IAZ * page_per_az);
	
      for(i = 0; i < page_per_az; i++)
      {
        adr = page_adr + i;
        nand_8bit_read_page(1,&data_buffer[i][0],adr);
      }
               
      pdata = (uint16_t*)&data_buffer[0][0];
      
      
      for(i = 0; i < initial_data.NS / 2; i++) *(uint16_t*) (sram_bank4 + 0) = *(pdata++);
      
      while(i++ < 8192) *(uint16_t*) (sram_bank4 + 0) = 0;
      

    }
  }

  
  GPIO_ToggleBits(GPIOI, GPIO_Pin_0);  
  
  pbuf_free(p);
}