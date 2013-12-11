#include "main.h" 

#pragma pack(push,1)   
 typedef struct 
 { int16_t  prevsample;
   int8_t   previndex;
   int8_t   channel_id;
   int8_t   adpcm_data[MAX_SAMPLE / 2];
   uint32_t time[2];
   uint32_t crc;
 } adpcm_struct;
#pragma pack(pop)