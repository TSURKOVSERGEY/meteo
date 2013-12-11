#include "main.h"
#include "nand_hw_driver.h"

extern initial_data_struct initial_data;
extern int page_per_az;
extern int32_t us_timer_az[3],us_timer_el[3];

CanRxMsg RxMessage;

struct info_msg_struct
{
  uint32_t can_AZ;
  uint32_t can_DAZ;
  float    AZ_f;
  float    VAZ;
  uint32_t can_EL;
  uint32_t can_DEL;
  float EL_f;
  float VEL;
}ims;

uint8_t can_data_buffer[MAX_PAGE][2048];

uint16_t can_AZ  = 0;
uint16_t can_EL  = 0;
int16_t  can_DAZ = 0;
int16_t  can_DEL = 0; 

void CAN1_RX0_IRQHandler(void)
{
  static uint16_t can_AZ_old = 0;
  static uint16_t can_EL_old = 0;

  CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);
 
  if(RxMessage.StdId == 0x21)
  {
    
    can_AZ = *((uint16_t*)&RxMessage.Data[0]) & 0x3fff;
    
    if((can_DAZ = can_AZ - can_AZ_old) < 0)
    {
      can_DAZ = 0x3fff - can_AZ_old + can_AZ + 1;
    }

    can_AZ_old = can_AZ;
    
    us_timer_az[2] = us_timer_az[0] / can_DAZ;
    us_timer_az[1] = us_timer_az[2];
    us_timer_az[0] = 0;
   
  }
  
  else if(RxMessage.StdId == 0x41)
  {
    can_EL = *((int16_t*)&RxMessage.Data[0]) & 0x3fff;
    
    if((can_DEL = can_EL - can_EL_old) < 0)
    {
      can_DEL = 0x3fff - can_EL_old + can_EL + 1;
    }

    can_EL_old = can_EL;
    
    us_timer_el[2] = us_timer_el[0] / can_DEL;
    us_timer_el[1] = us_timer_el[2];
    us_timer_el[0] = 0;
   
    NextAntenaPos();
  }

}

void PrintToDebug(void)
{
  ims.can_AZ  = can_AZ;
  ims.can_DAZ = can_DAZ;
  ims.AZ_f = ;

}


void NextAntenaPos(void)
{
  GPIO_ToggleBits(GPIOI, GPIO_Pin_1); 
}


/*


    ims.can_AZ = can_AZ;
    ims.DAZ = DAZ;
    ims.AZ_f = can_AZ_f;
    ims.us_timer_az = (float)us_timer_az / 10.0;
    ims.VAZ = (float)us_timer_az / (float)DAZ / 10.0;
    ims.tp = us_timer_az / DAZ * 100;
    us_timer_az = 0;
    SendUdpData((uint8_t*)&ims,sizeof(ims));


float can_AZ_f;
  float can_EL_f;

    if(can_EL != IEL_old)
    {
      if((DEL = can_EL - IEL_old) < 0)
      {
        DEL = 0x3fff - IEL_old + can_EL + 1;
      }
    
      IEL_old = can_EL;
    
      can_EL_f = (float)can_EL * 0.021972656; 
      can_EL_f = can_EL_f - 53.24;
     
      ims.can_EL = can_EL;
      ims.DEL = DEL;
      ims.EL_f = can_EL_f;
      
      ims.us_timer_el = us_timer_el;
      ims.VEL = us_timer_el / DEL;
      us_timer_el = 0;
    
    }

  
void GetNextPos(void)
{
  int i;
  uint16_t IAZ;
  uint16_t IEL;  
  uint32_t page_adr;
  uint32_t adr;
  uint16_t *pdata; 
  
  IAZ = (uint16_t)(can_AZ_f / initial_data.DAZ);
  IEL = (uint16_t)(can_EL_f / initial_data.DEL);
  
  page_adr = (IEL * initial_data.NAZ * page_per_az) + (IAZ * page_per_az);

  for(i = 0; i < page_per_az; i++)
  {
    adr = page_adr + i;
    nand_8bit_read_page(1,&can_data_buffer[i][0],adr);
  }
              
   pdata = (uint16_t*)&can_data_buffer[0][0];
   for(i = 0; i < initial_data.NS / 2; i++) *(uint16_t*) (sram_bank4 + 0) = *(pdata++);
   while(i++ < 8192) *(uint16_t*) (sram_bank4 + 0) = 0;
 
} 




*/