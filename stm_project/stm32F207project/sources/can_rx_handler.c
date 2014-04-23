#include "main.h"
#include "nand_hw_driver.h"

void PrintToDebug(void);

extern initial_data_struct initial_data;
extern int page_per_az;
extern int el_ext_ena;
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

float start_pos_az = -10.1; 
float start_pos_el = 53.24;


void CAN1_RX0_IRQHandler(void)
{
  static uint16_t can_AZ_old = 0;
  
  CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);
 
  if(RxMessage.StdId == 0x21)
  {
    can_AZ = *((uint16_t*)&RxMessage.Data[0]) & 0x3fff;
    
    if((can_DAZ = can_AZ - can_AZ_old) < 0)
    {
      can_DAZ = 0x3fff - can_AZ_old + can_AZ + 1;
    }

    can_AZ_old = can_AZ;

    #ifdef PTD1

    PrintToDebug();

    #endif
    
    us_timer_az[2] = us_timer_az[0] / can_DAZ;
    us_timer_az[1] = us_timer_az[2];
    us_timer_az[0] = 0;


    GetNextAZ(CAN_MODE);

  }
  
  else if(RxMessage.StdId == 0x41)
  {
    can_EL = *((int16_t*)&RxMessage.Data[0]) & 0x3fff;
  }
}



#define PTD3

#ifdef PTD1

void PrintToDebug(void)
{
  ims.can_AZ = can_AZ;
  ims.can_DAZ = can_DAZ;
  ims.AZ_f = (float)can_AZ * 0.021972656 - start_pos_az; 
  ims.can_EL = us_timer_az[0];
  ims.VAZ = (float)us_timer_az[0] / (float)can_DAZ / 10;

  SendUdpData((uint8_t*)&ims,sizeof(ims));
}

#endif
#ifdef PTD2

void PrintToDebug(void)
{
  ims.can_EL = can_EL;
  ims.can_DEL = can_DEL;
  ims.EL_f = (float)can_EL * 0.021972656 + start_pos_el; 
  ims.VAZ = (float)us_timer_el[0] / (float)can_DEL / 10.0;
  SendUdpData((uint8_t*)&ims,sizeof(ims));

}

#endif
#ifdef PTD3

void PrintToDebug(void)
{

}

#endif


  
void GetNextAZ(int mode)
{ 
  int i;
  static uint16_t AZ_old = 0;  
uint32_t err = 0;
uint32_t page_adr; 
  uint16_t IAZ;
  uint16_t IEL; 
  float AZ_f,EL_f;
  DMA_InitTypeDef            DMA_InitStructure;

  if(mode == TIM_MODE) 
  {
    can_AZ = (can_AZ + 1) & 0x3fff;
  }
  else if(mode == CAN_MODE)
  {
    if(AZ_old == can_AZ) return;
  }
 
  GPIO_ToggleBits(GPIOI, GPIO_Pin_1); 
  
  if(can_AZ != ((AZ_old + 1) & 0x3fff))
  {
    GPIO_ToggleBits(GPIOI, GPIO_Pin_0); 
    err++;
  }
  
  AZ_old = can_AZ;
  
  AZ_f = (float)can_AZ * 0.021972656 - start_pos_az; 
  EL_f = (float)can_EL * 0.021972656 - start_pos_el; 
  
  if(AZ_f < 0) AZ_f = AZ_f + 360.0;
  else if(AZ_f >= 360.0) AZ_f = AZ_f - 360.0;
  
  if(EL_f < -180.0) EL_f = EL_f + 360.0;
  else if(EL_f >= 180.0) EL_f = EL_f - 360.0;


  
  IAZ = (uint16_t)(AZ_f / initial_data.DAZ);
  IEL = (uint16_t)(EL_f / initial_data.DEL);
  
  page_adr = (IEL * initial_data.NAZ) + IAZ;
  
 #ifdef PTD3
  
  //PrintToDebug();
  
  ims.can_AZ = can_AZ;
  ims.can_EL = can_EL;
  ims.can_DAZ = page_adr;//can_DAZ;

  SendUdpData((uint8_t*)&ims,sizeof(ims));

#endif  
  
  nand_rdy(1);
  nand_command_wr(1,0x0000);					
  nand_adr_wr(1,0x0000);
  nand_adr_wr(1,0x0000);
  nand_page_adr_wr(1,page_adr);
  nand_command_wr(1,0x3030); 
  for (i=0;i<16;i++);
  nand_rdy(1);

  *(uint16_t*)(sram_bank4 + 2) =  0;
  
  // for(i = 0; i < 2048; i++) *(uint16_t*)(sram_bank4 + 0) =  nand_data_rd(1); 


    DMA_DeInit(DMA2_Stream6);
  
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE); 
    DMA_InitStructure.DMA_Channel = DMA_Channel_0;   
    DMA_InitStructure.DMA_PeripheralBaseAddr = sram_bank1;
    DMA_InitStructure.DMA_Memory0BaseAddr = sram_bank4;
    DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToMemory;
    DMA_InitStructure.DMA_BufferSize = 2048; 
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable;  
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_FIFOMode_Disable;
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable; 
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
    DMA_Init(DMA2_Stream6,&DMA_InitStructure);  
  
    DMA_Cmd(DMA2_Stream6, ENABLE);
   
  
   GPIO_ToggleBits(GPIOI, GPIO_Pin_1); 
}


