#include"main.h"
#include"at24c512.h"

uint32_t AT24_Config(void)
{
  I2C_InitTypeDef I2C_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;
  
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE); 
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6 | GPIO_Pin_9; 
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOB, &GPIO_InitStructure);     
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_I2C1);  
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource9, GPIO_AF_I2C1);  
    
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
  I2C_DeInit(I2C1);
  I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
  I2C_InitStructure.I2C_OwnAddress1 = 0x33;
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStructure.I2C_ClockSpeed = I2C_SPEED;
  I2C_Init(I2C1, &I2C_InitStructure);
  
  I2C_Cmd(I2C1, ENABLE);  
  
  return 0;
}

uint32_t AT45_Write(uint16_t Addr, uint8_t *pdata, int size)
{
  int adr = 0;
  
  while(size >= 0)
  {
    AT45_PageWrite(adr,pdata);
    delay(100);
    adr+=128;
    pdata+=128;
    size-=128;
  }

  return 1;
}
 
uint32_t AT45_PageWrite(uint16_t Addr, uint8_t *pdata)
{
  uint32_t Timeout;
  
  I2C_AcknowledgeConfig(I2C1, ENABLE);
  
  /* While the bus is busy */
  Timeout = AT45_TIMEOUT;
  while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY)) { if((Timeout--) == 0) return 0; }
 
  /* Send START condition */
  I2C_GenerateSTART(I2C1, ENABLE);
  
  /* Test on EV5 and clear it */
  Timeout = AT45_TIMEOUT; 
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) { if((Timeout--) == 0) return 0; }
 
  /* Send EEPROM address for write */
  I2C_Send7bitAddress(I2C1, 0xA0, I2C_Direction_Transmitter);
 
  /* Test on EV6 and clear it */
  Timeout = AT45_TIMEOUT; 
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) { if((Timeout--) == 0) return 0; }
 
  /* Send the EEPROM's internal address to read from: MSB of the address first */
  I2C_SendData(I2C1, (uint8_t)((Addr & 0xFF00) >> 8));
 
  /* Test on EV8 and clear it */
  Timeout = AT45_TIMEOUT;   
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) { if((Timeout--) == 0) return 0; }
 
  /* Send the EEPROM's internal address to read from: LSB of the address */
  I2C_SendData(I2C1, (uint8_t)(Addr & 0x00FF));
 
  /* Test on EV8 and clear it */
  Timeout = AT45_TIMEOUT;   
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))  { if((Timeout--) == 0) return 0; }
 
  for(int i = 0; i < 128; i++)
  {
    I2C_SendData(I2C1,*(pdata++));

    Timeout = AT45_TIMEOUT;   
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)); { if((Timeout--) == 0) return 0; }
  }
  
  I2C_AcknowledgeConfig(I2C1, DISABLE);
  
  /* Send STOP Condition */
  I2C_GenerateSTOP(I2C1, ENABLE);

  return 1;
}


uint32_t AT45_Read(uint16_t Addr, uint8_t *pdata, int size)
{
  uint32_t Timeout;
   
  I2C_AcknowledgeConfig(I2C1, ENABLE);
    
  /* While the bus is busy */
  Timeout = AT45_TIMEOUT;   
  while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY)) { if((Timeout--) == 0) return 0; }
 
  /* Send START condition */
  I2C_GenerateSTART(I2C1, ENABLE);
 
  /* Test on EV5 and clear it */
  Timeout = AT45_TIMEOUT;   
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) { if((Timeout--) == 0) return 0; }
 
  /* Send EEPROM address for write */
  I2C_Send7bitAddress(I2C1, 0xA0, I2C_Direction_Transmitter);
 
  /* Test on EV6 and clear it */
  Timeout = AT45_TIMEOUT;   
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) { if((Timeout--) == 0) return 0; }
 
  /* Send the EEPROM's internal address to read from: MSB of the address first */
  I2C_SendData(I2C1, (uint8_t)((Addr & 0xFF00) >> 8));
 
  /* Test on EV8 and clear it */
  Timeout = AT45_TIMEOUT;   
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) { if((Timeout--) == 0) return 0; }
 
  /* Send the EEPROM's internal address to read from: LSB of the address */
  I2C_SendData(I2C1, (uint8_t)(Addr & 0x00FF));
 
  /* Test on EV8 and clear it */
  Timeout = AT45_TIMEOUT;   
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) { if((Timeout--) == 0) return 0; }
 
 
  /* Send STRAT condition a second time */
  I2C_GenerateSTART(I2C1, ENABLE);
 
  /* Test on EV5 and clear it */
  Timeout = AT45_TIMEOUT;   
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) { if((Timeout--) == 0) return 0; }
 
  /* Send EEPROM address for read */
  I2C_Send7bitAddress(I2C1, 0xA1, I2C_Direction_Receiver);
 
  while(size-- > 0)
  {
    /* Test on EV6 and clear it */
    Timeout = AT45_TIMEOUT;   
    while(!I2C_CheckEvent(I2C1,I2C_EVENT_MASTER_BYTE_RECEIVED)); { if((Timeout--) == 0) return 0; }
 
    *(pdata++) = I2C_ReceiveData(I2C1);

  }
  
   I2C_AcknowledgeConfig(I2C1, DISABLE);
 
  /* Send STOP Condition */
  I2C_GenerateSTOP(I2C1, ENABLE);
  
  return 1;
}

