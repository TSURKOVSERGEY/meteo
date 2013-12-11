#include "main.h" 

CanRxMsg RxMessage;

void CAN1_RX0_IRQHandler(void)
{
 // GPIO_ToggleBits(GPIOB, GPIO_Pin_9); 
  CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);
}