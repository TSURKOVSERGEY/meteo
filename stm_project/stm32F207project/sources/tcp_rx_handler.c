#include "main.h"
#include "spi_f207.h"


extern tcp_message_struct    tx_tcp_msg;
extern tcp_message_struct    rx_tcp_msg;

extern struct                tcp_pcb *pout_pcb;

char buffer[300][256];

void tcp_rx_handler(void)
{
  if(crc32((uint8_t*)&rx_tcp_msg+4,rx_tcp_msg.msg_len + TCP_HEADER_SIZE - 4) == rx_tcp_msg.msg_crc)
  {
    switch(rx_tcp_msg.msg_id)
    {
      case CHECK_CONNECT: 
           tcp_write(pout_pcb," соединение установлено",23,1);
           tcp_output(pout_pcb);
           tcp_sent(pout_pcb,NULL); 
      break;
      
      case SEND_FILE: 
        {
           int adr  = rx_tcp_msg.data[0];
           int size = rx_tcp_msg.msg_len - 1;
           memcpy(&buffer[adr][0],&rx_tcp_msg.data[1],size);
        }
      break;
      
      case SET_DATA_DAC: 
        {
           unsigned short data_temp = *((unsigned short*)&rx_tcp_msg.data[0]);
           for (int i = 0; i < 8192; i++) *(uint16_t *) (sram_bank4 + 0) =  data_temp;
           GPIO_ToggleBits(GPIOI, GPIO_Pin_0); 
        }
      break;
      

      case SEND_FILE_DONE: 
        {
          int adr,size;
          unsigned short *pbuffer = (unsigned short*)&buffer[0][0];
 
          adr  = rx_tcp_msg.data[0];
          size = rx_tcp_msg.msg_len - 1;
          memcpy(&buffer[adr][0],&rx_tcp_msg.data[1],size);
           
          for (int i = 0; i < 8192; i++) *(uint16_t *) (sram_bank4 + 0) = *(pbuffer++);
             
          GPIO_ToggleBits(GPIOI, GPIO_Pin_0); 
          
        } 
     
      break;
    }
  }
}
