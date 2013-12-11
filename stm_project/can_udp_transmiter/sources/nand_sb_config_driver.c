#include "main.h"
#include "at24c512.h"


extern bad_block_map_struct  *pmap_bb;
extern super_block_struct    *prsb;
extern super_block_struct    *pwsb;
extern adpcm_page_struct     *padpcm[2][MAX_CHANNEL];
extern alarm_common_struct    common_data;
extern alarm_struct           alarm_data[2];

//#define DEBUG_ALARM_DATA

uint32_t SuperBlock_Config(void)
{
  int i;
  uint32_t crc, status = 0;
  
  prsb = (super_block_struct*)sram_bank3;
  pwsb = (super_block_struct*)((uint8_t*)prsb + (65*2048));
  pmap_bb = (bad_block_map_struct*) ((uint8_t*)pwsb + (65*2048));
 
  padpcm[0][0] = (adpcm_page_struct*)((uint8_t*)pmap_bb+sizeof(bad_block_map_struct));
  for(i = 1; i < MAX_CHANNEL; i ++) padpcm[0][i] = (adpcm_page_struct*)((uint8_t*)padpcm[0][i-1]+sizeof(adpcm_page_struct));
  
  padpcm[1][0] = (adpcm_page_struct*)((uint8_t*)padpcm[0][15]+sizeof(adpcm_page_struct));
  for(i = 1; i < MAX_CHANNEL; i ++) padpcm[1][i] = (adpcm_page_struct*)((uint8_t*)padpcm[1][i-1]+sizeof(adpcm_page_struct)); 
   
  
  memset(prsb,0,sizeof(super_block_struct));
  memset(pwsb,0,sizeof(super_block_struct));
  memset(pmap_bb,0,sizeof(bad_block_map_struct));
  memset(padpcm[0][0],0,sizeof(adpcm_page_struct)*MAX_CHANNEL*2);


#ifdef DEBUG_ALARM_DATA
  
  if(alarm_data[0].pcom == NULL)
  {
    alarm_data[0].pcom = &common_data;
    alarm_data[0].pcom->index = 0;
    GetAlarmScanSuperBlock(0);
  }

  if(alarm_data[1].pcom == NULL)
  {
    alarm_data[1].pcom = &common_data;
    alarm_data[1].pcom->index = 1;
    GetAlarmScanSuperBlock(1);
  }


#else 

  memset(alarm_data,0,sizeof(alarm_data));
  alarm_data[0].pcom = &common_data;    
  alarm_data[1].pcom = &common_data;  
  alarm_data[0].pcom->index = 0;
    
#endif
  
  
  alarm_data[0].pcom->index = 0;
  
 // page_real_read  = 0;
 // page_real_write = 0;

  AT45_Read(AT45ADR_MAP_BB,(uint8_t*)pmap_bb,sizeof(bad_block_map_struct));
  CRC_ResetDR();
  crc = CRC_CalcBlockCRC((uint32_t*)pmap_bb,(sizeof(bad_block_map_struct)-4)/4); 
  
  if(crc != pmap_bb-> crc) status = 1;
  
  return status;
} 