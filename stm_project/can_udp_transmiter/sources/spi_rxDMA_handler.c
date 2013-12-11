#include "main.h"
#include "ethernet.h"

extern adpcm_message_struct   adpcm_msg[2]; 
extern adpcm_page_ctrl_struct adpcm_ctrl[MAX_CHANNEL];
extern adpcm_page_struct     *padpcm[2][MAX_CHANNEL];
extern uint8_t                adpcm_ready;
extern system_info_struct     sys_info;
extern uint16_t spi_dma_buffer[2][SPI_RX_DMA];
extern int msg_8bit_size;
extern int msg_16bit_size;
extern int msg_32bit_size;

void DMA1_Stream0_IRQHandler(void)
{
  static uint16_t msg_id[2] = {0,0};
  static int wrr = 0;
  static int msg_index = 0;
  static uint16_t data_temp[2] = {0,0};
  static uint32_t mode = DLE_STX;
  uint32_t *pdata = (uint32_t*)&data_temp[0];
  uint16_t *padpcm_msg;
  uint32_t crc;
  
  DMA_ClearFlag(DMA1_Stream0, DMA_IT_TC | DMA_IT_TE);
  DMA_ClearITPendingBit(DMA1_Stream0, DMA_IT_TCIF0 | DMA_IT_TEIF0);
 
  int id = DMA_GetCurrentMemoryTarget(DMA1_Stream0);

  for(int i = 0;  i < SPI_RX_DMA; i++)
  {
     data_temp[1] = data_temp[0];
     data_temp[0] = spi_dma_buffer[id][i];
    
     if(mode == DLE_STX)
     {
       if(*pdata == DLE_STX) 
       {
         mode = DLE_ETX;
         padpcm_msg = (uint16_t*)&adpcm_msg[msg_index];
         wrr = 0;
       }
     }
     else if(mode == DLE_ETX)
     {       
       if(*pdata == DLE_ETX) 
       {  
      
         CRC_ResetDR();
         crc = CRC_CalcBlockCRC(((uint32_t*)&adpcm_msg[msg_index])+1,msg_32bit_size-1); 
      
         if(crc == adpcm_msg[msg_index].crc)
         {
           sys_info.L151_mode = adpcm_msg[msg_index].msg_id;
           
////////////////////////////////////////////////////////////////////////////////   
             
           int id,adr;
           
           for(int i = 0; i < MAX_CHANNEL; i++)
           {   
             id  = adpcm_ctrl[i].id;
             adr = adpcm_ctrl[i].adr++;
         
             memcpy((uint8_t*)&padpcm[id][i]->adpcm_data[adr][0],&adpcm_msg[msg_index].adpcm_block[i].adpcm_data[0],112);
     
             if(adr == 0)
             {
               padpcm[id][i]->prevsample = adpcm_msg[msg_index].adpcm_block[i].prevsample;
               padpcm[id][i]->previndex =  adpcm_msg[msg_index].adpcm_block[i].previndex;
               padpcm[id][i]->id = i;
               padpcm[id][i]->time = GetTime();
               padpcm[id][i]->crc = 0xaabbccdd;
             }
             else if(adr == 17) 
             {
               adpcm_ctrl[i].done = 1;
               adpcm_ctrl[i].id  ^= 1;
               adpcm_ctrl[i].adr  = 0;
             }
           }
           
           if(adr == 17)
           {
             for(int i = 0; i < MAX_CHANNEL; i++) adpcm_ready |= adpcm_ctrl[i].done;
           }
           
////////////////////////////////////////////////////////////////////////////////   

         }
         
          msg_id[0] = adpcm_msg[msg_index].msg_counter;
          
          if(msg_id[0] != (msg_id[1]+1))
          {
            sys_info.L151_stream_error++;
          }
          
          msg_id[1] = msg_id[0];
              
          *pdata = 0;
          mode = DLE_STX;         
          msg_index ^= 1; 
          
       }
       else if(*pdata == DLE_DLE) 
       {
         data_temp[0] = 0;
       }
       else 
       {
         if(wrr++ < msg_16bit_size) *(padpcm_msg++) = data_temp[0];
       }
     
      }
  }
}
