#include "main.h"
#include "at24c512.h"
#include "spi_f207.h"
#include "nand_hw_driver.h"


extern initial_data_struct      initial_data;
extern tcp_message_struct       tx_tcp_msg;
extern tcp_message_struct       rx_tcp_msg;
extern ethernet_initial_struct  eth_ini_dat;
extern struct tcp_pcb*          pout_pcb;
extern int page_per_az;
extern int mode;
extern int get_crc_flag;
int total_page;

char buffer[64][256];


void eth_cmd_handler(void)
{
  int i,j;
  unsigned short  freq;
  unsigned short *pbuffer = (unsigned short*)&buffer[0][0];
       
  if(crc32((uint8_t*)&rx_tcp_msg+4,rx_tcp_msg.msg_len + TCP_HEADER_SIZE - 4,sizeof(rx_tcp_msg)) == rx_tcp_msg.msg_crc)
  {
    switch(rx_tcp_msg.msg_id)
    {
      case CHECK_CONNECT: 
        
           tx_tcp_msg.msg_id  = CHECK_CONNECT + 100;
           tx_tcp_msg.msg_len = 0;
           tx_tcp_msg.msg_crc = crc32((uint8_t*)&tx_tcp_msg+4,(TCP_HEADER_SIZE-4) + tx_tcp_msg.msg_len,sizeof(rx_tcp_msg)); 
           SendTcpData(&tx_tcp_msg,TCP_HEADER_SIZE+tx_tcp_msg.msg_len);
           memset(buffer,0,sizeof(buffer));
           GPIO_ToggleBits(GPIOI, GPIO_Pin_0);             
           
      break;
      
      case GET_CRC: 
        
        get_crc_flag = 1;
        
      break;
      
      
      case SEND_DATA_MODE_1: 
        
           i  = rx_tcp_msg.data[0];
           j = rx_tcp_msg.msg_len - 1;
           memcpy(&buffer[i][0],&rx_tcp_msg.data[1],j);
  
           
      break;
      
      case SEND_DATA_MODE_2: 
        
           WriteNandPage((mode2_msg_data_struct*)&rx_tcp_msg.data[0]);
        
      break;
      
      
      case SET_DATA_DAC: 
   
           for (i = 0; i < 8192; i++) *(uint16_t *) (sram_bank4 + 0) =  *((unsigned short*)&rx_tcp_msg.data[0]);
           GPIO_ToggleBits(GPIOI, GPIO_Pin_0);  
        
      break;

      case SET_FREQ_DAC: 
        
          freq = *((uint16_t *)&rx_tcp_msg.data[0]);
          *(uint16_t *)(sram_bank4 + 4) = 50000 / freq;
          GPIO_ToggleBits(GPIOI, GPIO_Pin_0); 
        
      break;      

      case SET_MODE_1: 

          for(i = 0; i < 8192; i++) 
          {
            *(uint16_t *) (sram_bank4 + 0) = *(pbuffer++);
          }

          mode = SET_MODE_1;
          GPIO_ToggleBits(GPIOI, GPIO_Pin_0);  
      break;
      
      case SET_MODE_2: 
      
        mode = SET_MODE_2;
        GPIO_ToggleBits(GPIOI, GPIO_Pin_0); 
      break;

      case SET_INITIAL_DATA:
        
        memcpy(&initial_data,&rx_tcp_msg.data[0],sizeof(initial_data));
       
       	page_per_az = initial_data.NS / 1024;

	if((page_per_az * 1024) - initial_data.NS < 0) page_per_az += 1;
	
        total_page = (initial_data.NAZ * page_per_az) * initial_data.NEL;
       
        for(i = 0; i < total_page; i+=64) 
        {
          if((nand_erase_block(1,i) & 0x1) == 0x1)
          {
            delay(10);
          }
        }
      
        GPIO_ToggleBits(GPIOI, GPIO_Pin_0); 
             
      break;
      
      case GET_ETH_PARAM:
        
           AT45_Read(0,(uint8_t*)&eth_ini_dat,sizeof(eth_ini_dat));
           memcpy(&tx_tcp_msg.data[0],&eth_ini_dat,sizeof(eth_ini_dat));
           tx_tcp_msg.msg_id  = GET_ETH_PARAM + 100;
           tx_tcp_msg.msg_len = sizeof(eth_ini_dat);
           tx_tcp_msg.msg_crc = crc32((uint8_t*)&tx_tcp_msg+4,(TCP_HEADER_SIZE-4) + tx_tcp_msg.msg_len,sizeof(rx_tcp_msg)); 
           SendTcpData(&tx_tcp_msg,TCP_HEADER_SIZE+tx_tcp_msg.msg_len);
           GPIO_ToggleBits(GPIOI, GPIO_Pin_0); 
           
      break;

      case SET_ETH_PARAM:
        
        switch(rx_tcp_msg.data[0])
        {
          case SET_MAC: memcpy(&eth_ini_dat.MAC_ADR,&rx_tcp_msg.data[1],6);  break;
          case SET_GW: memcpy(&eth_ini_dat.GW_ADR,&rx_tcp_msg.data[1],4);    break;
          case SET_IP: memcpy(&eth_ini_dat.IP_ADR,&rx_tcp_msg.data[1],4);    break;
          case SET_MASK: memcpy(&eth_ini_dat.MASK,&rx_tcp_msg.data[1],4);    break;          
          case SET_TCP_RX_PORT: eth_ini_dat.TCP_RX_PORT = *((uint32_t*)&rx_tcp_msg.data[1]); break;   
          case SET_UDP_RX_PORT: eth_ini_dat.UDP_RX_PORT = *((uint32_t*)&rx_tcp_msg.data[1]); break;  
        }
        
        eth_ini_dat.crc = crc32(&eth_ini_dat,sizeof(eth_ini_dat)-4,sizeof(eth_ini_dat)-4);
        AT45_Write(0,(uint8_t*)&eth_ini_dat,sizeof(eth_ini_dat));
        GPIO_ToggleBits(GPIOI, GPIO_Pin_0); 
      break;    
      
    }
  }
}
