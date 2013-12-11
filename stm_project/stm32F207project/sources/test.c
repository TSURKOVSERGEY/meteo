 #include "main.h"

uint32_t SRAM_Test(void)
{
  uint32_t  adr   = 0;
  uint32_t  error = 0;
  uint16_t *prnd;
  uint16_t *pmem;
  uint16_t  temp;
  
  prnd = malloc(512); 
 
  RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG, ENABLE);
  RNG_Cmd(ENABLE);
  
  for(adr = 0; adr < 512; adr++)
  {
    while(RNG_GetFlagStatus(RNG_FLAG_DRDY) == RESET); 
    *(prnd++) = (uint16_t)RNG_GetRandomNumber();   
  } 
   
  RNG_Cmd(DISABLE);
  RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG,DISABLE);     
  
  prnd -= 512;
  pmem = (uint16_t*)sram_bank3;
  temp = *prnd;
  
  for(adr = 0; adr < 524288; adr++)
  { 
    if((adr & 0x3ff) == 0x3ff) temp = *(prnd++);
    temp = (temp + 1024) * 5;
    *(pmem++) = temp;
  }
  
  for(int test_num = 0; test_num < 3; test_num++)
  {
    prnd -= 512;
    pmem = (uint16_t*)sram_bank3;
    temp = *prnd;
  
    for(adr = 0; adr < 524288; adr++)
    {
      if((adr & 0x3ff) == 0x3ff) temp = *(prnd++);
      
      temp = (temp + 1024) * 5;
      
      if(temp != *(pmem++)) error++;
    }
  }
  
  free(prnd); 
  return error;
}
