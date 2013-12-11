#include "stm32f2xx.h"

void SPI_GPIO_Config(void);
void SPI_Config(void);
void SPI_TxDma_Config(void);
void SPI_RxDma_Config(FunctionalState mode);
void SPI_RxInt_Config(FunctionalState mode);
int Write16BitData(unsigned short data);
int Write16BitMessage(unsigned short *pbuffer, int size);
void SendL151Message(uint16_t *pbuffer, int size);