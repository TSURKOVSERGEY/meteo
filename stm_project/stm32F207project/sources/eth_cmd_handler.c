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
extern int nand_erase_flag;
extern int dma_mode;
char buffer[64][256];



void EnableIrq(void)
{
   TIM_Cmd(TIM5,ENABLE);
   CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);
}

void DisableIrq(void)
{
   TIM_Cmd(TIM5,DISABLE);
   CAN_ITConfig(CAN1, CAN_IT_FMP0, DISABLE);
   
   *(uint16_t*)(sram_bank4 + 2) =  0;
   for(int i = 0; i < 2048; i++) *(uint16_t*)(sram_bank4 + 0) =  0; 
}

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
      
      case ENABLE_EXT: 
        TIM_Cmd(TIM5,ENABLE);
      break;

      case DISABLE_EXT: 
        TIM_Cmd(TIM5,DISABLE);
      break;

      case ENABLE_DMA: 
        dma_mode = 1;
      break;

      case DISABLE_DMA: 
        dma_mode = 0;
      break;

      case SET_DATA_DAC: 
   
           for (i = 0; i < 8192; i++) *(uint16_t *) (sram_bank4 + 0) =  *((unsigned short*)&rx_tcp_msg.data[0]);
        
      break;

      case SET_FREQ_DAC: 
        
          freq = *((uint16_t *)&rx_tcp_msg.data[0]);
          *(uint16_t *)(sram_bank4 + 4) = 50000 / freq;
          
      break;      

      case SET_MODE_1: 

          for(i = 0; i < 8192; i++) 
          {
            *(uint16_t *) (sram_bank4 + 0) = *(pbuffer++);
          }

          mode = SET_MODE_1;
          
      break;
      
      case SET_MODE_2: 
      
        mode = SET_MODE_2;
        
      break;

      case SET_INITIAL_DATA:
        
        nand_erase_flag = 1;
        
      break;
      
      case GET_ETH_PARAM:
        
           AT45_Read(0,(uint8_t*)&eth_ini_dat,sizeof(eth_ini_dat));
           memcpy(&tx_tcp_msg.data[0],&eth_ini_dat,sizeof(eth_ini_dat));
           tx_tcp_msg.msg_id  = GET_ETH_PARAM + 100;
           tx_tcp_msg.msg_len = sizeof(eth_ini_dat);
           tx_tcp_msg.msg_crc = crc32((uint8_t*)&tx_tcp_msg+4,(TCP_HEADER_SIZE-4) + tx_tcp_msg.msg_len,sizeof(rx_tcp_msg)); 
           SendTcpData(&tx_tcp_msg,TCP_HEADER_SIZE+tx_tcp_msg.msg_len);
           
           
      break;

      case START:
        
        EnableIrq();

      break;

      
      case STOP:
        
        DisableIrq();

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
        
      break;    
      
    }
  }
}
