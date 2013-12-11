#include "spi_f207.h"
#include "stm32f2xx_i2c.h"
#include "stm32f2xx_dma.h"
#include "string.h"
#include "main.h"

#define DMA_Stream_IT_MASK     (uint32_t)(((DMA_LISR_FEIF0 | DMA_LISR_DMEIF0 | \
                                           DMA_LISR_TEIF0 | DMA_LISR_HTIF0 | \
                                           DMA_LISR_TCIF0) << 22) | (uint32_t)0x20000000)

extern uint16_t spi_dma_buffer[2][SPI_RX_DMA];

byte_stuffing_message_struct bs_tx_msg;

void SPI_Config(void)
{
  SPI_GPIO_Config();
  SPI_InitTypeDef  SPI_InitStructure;
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE); 
  SPI_I2S_DeInit(SPI3);
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Slave;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_16b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 0;
  SPI_Init(SPI3, &SPI_InitStructure);
  SPI_SSOutputCmd(SPI3,DISABLE);
  SPI_NSSInternalSoftwareConfig(SPI1, SPI_NSSInternalSoft_Set);
  SPI_Cmd(SPI3, ENABLE); 
  SPI3->DR = DLE;
}

void SPI_RxInt_Config(FunctionalState mode)
{
  NVIC_InitTypeDef NVIC_InitStructure;
  SPI_ITConfig(SPI3,SPI_IT_RXNE,mode);
  NVIC_InitStructure.NVIC_IRQChannel = SPI3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

void SPI_GPIO_Config(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12; 
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_SPI3);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_SPI3);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_SPI3);
}

void SPI_RxDma_Config(FunctionalState mode)
{
  DMA_InitTypeDef   DMA_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);
  SPI_DMACmd(SPI3,SPI_DMAReq_Rx,DISABLE);
  DMA_Cmd(DMA1_Stream0,DISABLE);
  DMA_DeInit(DMA1_Stream0);
  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream0_IRQn; 
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = mode;
  NVIC_Init(&NVIC_InitStructure);
  DMA_InitStructure.DMA_Channel = DMA_Channel_0;  
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &(SPI3->DR);
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t) &spi_dma_buffer[1][0];
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_BufferSize = SPI_RX_DMA;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;         
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_Init(DMA1_Stream0,&DMA_InitStructure);
  DMA_DoubleBufferModeConfig(DMA1_Stream0, (uint32_t)&spi_dma_buffer[0][0], DMA_Memory_0);
  DMA_DoubleBufferModeCmd(DMA1_Stream0, mode);
  DMA_ITConfig(DMA1_Stream0,DMA_IT_TC,mode); 
  DMA_Cmd(DMA1_Stream0,mode);
  SPI_DMACmd(SPI3,SPI_DMAReq_Rx,mode);
}



void SendL151Message(uint16_t *pbuffer, int size)
{  
  bs_tx_msg.msg_len = 0;
  
  bs_tx_msg.data[bs_tx_msg.msg_len++] = DLE;
  bs_tx_msg.data[bs_tx_msg.msg_len++] = STX;
 
  while(size-- >0)
  {
    bs_tx_msg.data[bs_tx_msg.msg_len++] = *(pbuffer++);  
    if(*(pbuffer-1) == DLE) bs_tx_msg.data[bs_tx_msg.msg_len++] = DLE;
  } 
 
  bs_tx_msg.data[bs_tx_msg.msg_len++] = DLE;
  bs_tx_msg.data[bs_tx_msg.msg_len++] = ETX;
  bs_tx_msg.data[bs_tx_msg.msg_len++] = 0;
  
  DMA1->HIFCR = DMA_Stream_IT_MASK;
  DMA1_Stream7->M0AR = (uint32_t)&bs_tx_msg;
  DMA1_Stream7->NDTR = bs_tx_msg.msg_len;
  DMA1_Stream7->CR |= (uint32_t)DMA_SxCR_EN;

}

void SPI_TxDma_Config(void)
{
  DMA_InitTypeDef   DMA_InitStructure;
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);
  DMA_Cmd(DMA1_Stream7, DISABLE);
  DMA_DeInit(DMA1_Stream7);     
  DMA_InitStructure.DMA_Channel = DMA_Channel_0;  
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &(SPI3->DR);
  DMA_InitStructure.DMA_Memory0BaseAddr = 0;
  DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
  DMA_InitStructure.DMA_BufferSize = 0;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;         
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_Init(DMA1_Stream7, &DMA_InitStructure);
  DMA_Cmd(DMA1_Stream7, ENABLE);
  SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Tx, ENABLE); 
}