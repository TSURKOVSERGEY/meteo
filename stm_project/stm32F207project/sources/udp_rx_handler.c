#include "main.h"
#include "nand_hw_driver.h"

extern uint8_t rls_msg_buffer[1024];
extern initial_data_struct initial_data;
extern int page_per_az;

uint32_t page_adr_old = -1;
  
uint16_t az_old = 0;
uint16_t el_old = 0;


uint16_t AZEL[2][400*11];
int azel_index = 0;


uint8_t data_buffer[MAX_PAGE][2048];
  
void udp_rx_handler(void *arg, struct udp_pcb *upcb,struct pbuf *p, struct ip_addr *addr, u16_t port)
{
  //char debuf[50];
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
  
  pbuf_free(p);
  
  if((strcmp((char*)&rls_msg_buffer[4],"RCV02")) == 0)
  {
    if(rls_msg_buffer[16] == 0x80) 
    { 
         
     // GPIO_ToggleBits(GPIOA, GPIO_Pin_4); 
         
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
      
      if(page_adr == page_adr_old) return;
      else
      {
        page_adr_old = page_adr;
        az_old = AZ;
        el_old = EL;
      }
	  
      AZEL[0][azel_index] = AZ;
      AZEL[1][azel_index] = EL;
      azel_index++;  
      if(EL > 3)
       {
         delay(0);
       }
      
     // TIM7_Config(16000);
      
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



}


void SendCanMessage(void)
{
  CanTxMsg TxMessage;
  uint8_t mailbox;
  static uint8_t temp = 0;
  
  TxMessage.StdId = 0x0001;
  TxMessage.ExtId = 0x0001;
  TxMessage.RTR=CAN_RTR_DATA;
  TxMessage.IDE=CAN_ID_STD;
  TxMessage.DLC = 8;
    
  for(int i = 0; i < 8; i++) TxMessage.Data[i] = temp + i;
  temp++;
  mailbox = CAN_Transmit(CAN1,&TxMessage);
  while (CAN_TransmitStatus(CAN1, mailbox) != CAN_TxStatus_Ok){;};
   
}

void TIM7_IRQHandler(void)
{
  TIM_ClearITPendingBit(TIM7,TIM7_IRQn);
  //TIM_ClearITPendingBit(TIM7,TIM_IT_CC7);
  
 // if(TIM7->CNT > 2000)
  {
    GPIO_ToggleBits(GPIOI, GPIO_Pin_1); 
  }

   
  
}

/*
void TIM7_IRQHandler(void)
{

  int i;
  
  uint16_t IAZ;
  uint16_t IEL;
    
  float AZ_f;
  float EL_f; 
  
  uint32_t page_adr;
  uint32_t adr;
  
  uint16_t *pdata; 
  
  TIM_ClearITPendingBit(TIM7,TIM7_IRQn);
  GPIO_ToggleBits(GPIOA, GPIO_Pin_4); 
    
  if(az_old++ > initial_data.NAZ)
  {
    az_old = 0;
    
    if(el_old++ > initial_data.NEL)
    {
      el_old = 0;
    }
  }
  
  
  AZ_f  = (float)az_old * 360.0 / 16384.0;
  EL_f  = (float)el_old * 360.0 / 16384.0;
      
  IAZ = (uint16_t)(AZ_f / initial_data.DAZ);
  IEL = (uint16_t)(EL_f / initial_data.DEL);
      
  page_adr = (IEL * initial_data.NAZ * page_per_az) + (IAZ * page_per_az);
  
  if(page_adr == page_adr_old) return;
  else
  {
    page_adr_old = page_adr;
  }
   
      
  AZEL[0][azel_index] = az_old;
  AZEL[1][azel_index] = el_old;
  
  azel_index++;  
  
  if(el_old > 3)
  {
    delay(0);
  }
      
//  for(i = 0; i < page_per_az; i++)
//  {
//    adr = page_adr + i;
//    nand_8bit_read_page(1,&data_buffer[i][0],adr);
//  }
//               
//   pdata = (uint16_t*)&data_buffer[0][0];
//     
//   for(i = 0; i < initial_data.NS / 2; i++) *(uint16_t*) (sram_bank4 + 0) = *(pdata++);
//      
//   while(i++ < 8192) *(uint16_t*) (sram_bank4 + 0) = 0;
//      


}
*/