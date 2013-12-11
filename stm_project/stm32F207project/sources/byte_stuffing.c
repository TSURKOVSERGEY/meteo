#include "byte_stuffing.h"
#include "spi_f207.h"
#include "stdio.h"
#include "stdlib.h"

extern unsigned short msg_buffer[256];

int StuffingWriteCmd(unsigned short cmd)
{
  unsigned short msg[HEADER_SIZE] = {DLE,STX,cmd,DLE,ETX};
  Write16BitMessage(msg,HEADER_SIZE);
  return HEADER_SIZE;
}

int StuffingWriteData(unsigned short cmd, unsigned short data)
{
  int wrw = 0;
  unsigned short msg[HEADER_SIZE+2];
  msg[wrw++] = DLE;
  msg[wrw++] = STX;
  msg[wrw++] = cmd;
  if(data == DLE) msg[wrw++] = DLE;
  msg[wrw++] = data;  
  msg[wrw++] = DLE;
  msg[wrw++] = ETX;
  Write16BitMessage(msg,wrw);
  return wrw;
}


int StuffingWriteMessage(unsigned short cmd, unsigned short *pbin, int size)
{
  int wrw = HEADER_SIZE+1;
  unsigned short *pmsg = malloc((size*2)+HEADER_SIZE); 
    
  unsigned short *pbout = pmsg; 
 
  *(pbout++) = DLE;
  *(pbout++) = STX; 
  *(pbout++) = cmd;
  *(pbout++) = size;
  
  if(size == DLE) *(pbout++) = DLE;

  while(size-- > 0)
  { *(pbout++) = *(pbin++);
    if(*(pbout-1) == DLE) *(pbout++) = DLE;
  }
  
  *(pbout++) = DLE;
  *(pbout++) = ETX; 
  
  wrw += pbout - pmsg;
    
  wrw = Write16BitMessage(pmsg,wrw);;
  
  free(pmsg);
  
  return wrw;
}

int StuffingReadMessage(unsigned short data)
{
  static unsigned char recive_flag = 0;
  static unsigned char adr = 0;
  static unsigned short data_temp[2] = {0,0};
  int *pdata = (int*)&data_temp[0];
  
  data_temp[0] = data;
  
  if(*pdata == DLE_STX)
  {
    if(recive_flag)
    { msg_buffer[adr] = data;
    }
    else 
    { recive_flag = 1;
      adr = 0;
    }
  }
  else if(recive_flag & *pdata == DLE_ETX)
  {
    *pdata = 0;
     recive_flag = 0;
     return adr;
  }
  else if(*pdata == DLE_DLE) 
  {
    data_temp[0] = 0;
  }
  else if(recive_flag)
  {
    msg_buffer[adr++] = data_temp[0];
  }
  
  data_temp[1] = data_temp[0];
          
  return 0;
}


