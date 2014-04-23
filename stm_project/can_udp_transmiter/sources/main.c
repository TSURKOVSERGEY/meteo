#include "main.h"
#include "debug.h"
#include "ethernet.h"
#include "at24c512.h"
#include "nand_hw_driver.h"

__IO uint32_t LocalTime = 0;
int rew_status = 0;

#define PERIOD           1000 // микросекунды 
#define PERIOD_MODULE    5

void main()
{
  
  RCC_ClocksTypeDef RCC_Clocks;
  
  RCC_GetClocksFreq(&RCC_Clocks);
  
  SysTick_Config(RCC_Clocks.HCLK_Frequency / 100);  

  LED_Config();
  LWIP_Config();
  UDP_Config();  
  CAN_Config();
  TIM7_Config(PERIOD);

  while(1)
  {
    if(ETH_CheckFrameReceived())
    {
      LwIP_Pkt_Handle();
    }
      
    LwIP_Periodic_Handle(LocalTime);
    
    if(rew_status != 0) ReSendUdpData();
  }
 
}

void SendCanMessage(uint32_t id, uint16_t data)
{
  CanTxMsg TxMessage;
  uint8_t mailbox;

  TxMessage.StdId = id;
  TxMessage.ExtId = id;
  TxMessage.RTR=CAN_RTR_DATA;
  TxMessage.IDE=CAN_ID_STD;
  TxMessage.DLC = 4;
    
  *((uint16_t*)(&TxMessage.Data[0])) = data;

  mailbox = CAN_Transmit(CAN1,&TxMessage);
  while (CAN_TransmitStatus(CAN1, mailbox) != CAN_TxStatus_Ok){;};
   
}



void TIM7_IRQHandler(void)
{
  
  TIM_ClearITPendingBit(TIM7,TIM7_IRQn);
    
  static uint16_t AZ = 0x3fff - 500, EL = 2423, flag = 0;
    
  GPIO_ToggleBits(GPIOB, GPIO_Pin_9); 
  //GPIO_ToggleBits(GPIOB, GPIO_Pin_9); 
  
  if(flag++ == (PERIOD_MODULE - 1))
  {
    SendCanMessage(0x21,AZ);
    SendCanMessage(0x41,EL);
    flag = 0;
  }
    
  AZ = (AZ + 1) & 0x3fff;
  if(AZ == 0) EL = (EL + 1) & 0x3fff;

}
 

void SysTick_Handler(void)
{
  LocalTime += SYSTEMTICK_PERIOD_MS;
}

void delay(uint32_t nCount)
{
  uint32_t timingdelay = LocalTime + nCount;  
  while(timingdelay > LocalTime);
}

