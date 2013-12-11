#include "main.h"
#include "ethernet.h"
#include "nand_sb_driver.h"
#include "nand_hw_driver.h"
#include "at24c512.h"


extern struct              udp_pcb *pudp_pcb;
extern struct              tcp_pcb *ptcp_pcb;
extern struct              tcp_pcb *pout_pcb;
extern struct              pbuf *pOut;
extern udp_message_struct  tx_udp_msg;
extern tcp_message_struct  tx_tcp_msg;
extern struct              ip_addr ip_addr_tx;  
extern void *xarg;

bad_block_map_struct         *pmap_bb;
super_block_struct           *prsb;
super_block_struct           *pwsb;
extern adpcm_page_struct     *padpcm[2][MAX_CHANNEL]; 
extern adpcm_page_ctrl_struct adpcm_ctrl[MAX_CHANNEL];

alarm_struct               alarm_data[2];
alarm_common_struct        common_data;

int32_t     page_real_read;
int32_t     page_real_write;
int32_t     read_MT29Unit_id;

#define ONE_PAGE  1
#define ALL_PAGE  0

#define FILE_BEGIN 0
#define FILE_END   1

void NandWriteDataHandler(void)
{
  for(int i = 0; i < MAX_CHANNEL; i++)
  {
    if(adpcm_ctrl[i].done == 1)
    {
      WriteNandPage(padpcm[adpcm_ctrl[i].id^0x1][i]);
      adpcm_ctrl[i].done = 0;
      GPIO_ToggleBits(GPIOI, GPIO_Pin_1); 
    }
  }
}

int GetSuperBlockAdr(uint32_t *padr, uint32_t sb_num, int mode)
{
  alarm_struct *palarm = &alarm_data[alarm_data[0].pcom->index];
    
  uint32_t adr = palarm->super_block_begin;
  
  while(1)
  {
    ReadSuperBlockHeader(adr,ONE_PAGE);
  
    if(prsb->sb_num != sb_num)
    {
      if(prsb->id != SUPER_BLOCK_ID) return 0;
      else if(prsb->super_block_next == NULL) return 0;
           else
           {
             adr = prsb->super_block_next;
           }
    }
    else break;
  }
  
  if(mode == FILE_END)
  {
    ReadSuperBlockHeader(adr,ALL_PAGE);
    *padr = prsb->ips[prsb->page_real_write].page_address; 
  }
  else *padr = adr;
  
  return 1;
}

int ReadFile(uint32_t file_begin, uint32_t file_end)
{
  uint32_t page_begin, page_end;
  
  if(!GetSuperBlockAdr(&page_begin,file_begin,FILE_BEGIN)) return 0;
  else if(!GetSuperBlockAdr(&page_end,file_end,FILE_END)) return 0;  
       else
       {
         SetPageReadAdr(alarm_data[0].pcom->index,page_begin,page_end);
       }
       
 return 1;      
}

int ReadSuperBlock(uint32_t sb_begin, uint32_t sb_end)
{  uint32_t page_adr;

   if(!GetSuperBlockAdr(&page_adr,sb_begin,FILE_BEGIN)) return 0;
}

void GetSuperBlockInfo(uint8_t id, void *p)
{
}

void RecoverAddresList(uint32_t adr1, uint32_t adr2)
{
  delay(0);
}

int CheckSuperBlockCRC(int adr)
{ 

  ReadSuperBlockHeader(adr,ALL_PAGE);
  
  return 1;
}

int CheckAddresList(int adr, int mode)
{
  static uint32_t prev_adr = 0;

  alarm_struct *palarm = &alarm_data[alarm_data[0].pcom->index];
    
  if(mode)
  { prev_adr = 0;
    palarm->super_block_time[0] = prsb->time_open;
    palarm->super_block_begin = adr;
  }
  
  if(prsb->status != SUPER_BLOCK_READ)
  {
    palarm->super_block_real_read++;  
  }
  
  palarm->super_block_time[1] = prsb->time_close;
  palarm->super_block_real_write++; 
  palarm->PageRealWrite = prsb->page_real_write;
  palarm->super_block_current = adr;
  palarm->super_block_prev = prev_adr;
  prev_adr = adr;
  palarm->PageAddress = prsb->ips[prsb->page_real_write].page_address + 1; 

  if(prsb->super_block_next != 0)
  {  
    adr = prsb->super_block_next;
  
    ReadSuperBlockHeader(adr,ONE_PAGE);
  
    if(prsb->id == SUPER_BLOCK_ID)
    {
      if(CheckSuperBlockCRC(adr))
      {
        adr = CheckAddresList(adr,ALL_PAGE);
      }
      else
      {
        adr = palarm->PageAddress;
      }
    }
  }
  else adr = palarm->PageAddress;
  
  return adr;
}

void GetAlarmScanSuperBlock(int id)
{
  #define MAX_PAGES 64 * 50 //2048

  int page_adr = 0;
  uint32_t super_block_prev = 0;

  
  while(page_adr < MAX_PAGES)
  {
    ReadSuperBlockHeader(page_adr,ONE_PAGE);
    
    if(prsb->id == SUPER_BLOCK_ID)
    {
      if(CheckSuperBlockCRC(page_adr))
      {
        if(super_block_prev != prsb->super_block_prev)
        {
          RecoverAddresList(super_block_prev,prsb->super_block_prev);     
        }
      
        page_adr = CheckAddresList(page_adr,ONE_PAGE);
        
        super_block_prev = alarm_data[id].super_block_current;
      }
    }
    
    page_adr++;
    printf("%d",page_adr);
  }
}

void SetPageReadAdr(uint32_t id, uint32_t page_begin, uint32_t page_end)
{
  if(page_real_write > 0) 
  {
    return;
  }
  else if(page_end >= page_begin) 
  {
    read_MT29Unit_id = id;
    page_real_write  = (page_end - page_begin + 1) * K_BIT;
    page_real_read   = page_begin << SHIFT_MASK;
    tx_tcp_msg.msg_len = 0;
  }
} 

void SendSuperBlockHandler(void)
{

}

void SendDataHandler(void)
{
  static int flag_busy = 1;
 
  void *ptr = NULL;
  
  if(page_real_write > 1)
  {
    tx_tcp_msg.msg_id  = READ_PAGE;
  }
  else if(page_real_write == 1)
  {
    tx_tcp_msg.msg_id  = READ_PAGE_DONE;
  }
  else return;
    
  tx_tcp_msg.msg_len = TCP_MESSAGE_SIZE;
  
  if(tcp_sndbuf(pout_pcb) < TCP_MESSAGE_SIZE+TCP_HEADER_SIZE) 
  {
    return;
  }
  else if(flag_busy)
  {
    flag_busy = 0;
       
     switch(page_real_read & AND_MASK)
     {
       case 0:
                nand_8bit_read_page(read_MT29Unit_id,&tx_tcp_msg.data[0],page_real_read >> SHIFT_MASK);
                ptr = &tx_tcp_msg;
       break;
       case 1:
                ptr = &tx_tcp_msg.data[1024 - TCP_HEADER_SIZE];
                memcpy(ptr,&tx_tcp_msg,TCP_HEADER_SIZE);
       break;
       case 2:
                ptr = &tx_tcp_msg.data[2048 - TCP_HEADER_SIZE];
                memcpy(ptr,&tx_tcp_msg,TCP_HEADER_SIZE);
       break;
       case 3:
                ptr = &tx_tcp_msg.data[3072 - TCP_HEADER_SIZE];
                memcpy(ptr,&tx_tcp_msg,TCP_HEADER_SIZE);
       break;
     }
     
     CRC_ResetDR();
     *((uint32_t*)ptr) = CRC_CalcBlockCRC((uint32_t*)ptr+1,TCP_MESSAGE_SIZE+2); 
     page_real_read++;
  }
       
   if(tcp_write(pout_pcb,ptr,TCP_MESSAGE_SIZE+TCP_HEADER_SIZE,1) == ERR_OK) 
   {
       flag_busy = 1;
       page_real_write--;
       tcp_output(pout_pcb);
       tcp_sent(pout_pcb,tcp_callback_sent);  
   }
}
 

//#define DEBUG_ALARM_DATA

uint32_t SuperBlock_Config(void)
{
  uint32_t crc, status = 0;
  
  prsb = (super_block_struct*)sram_bank3;
  pwsb = (super_block_struct*)((uint8_t*)prsb+sizeof(super_block_struct));
  pmap_bb = (bad_block_map_struct*) ((uint8_t*)pwsb+sizeof(super_block_struct));
 
  padpcm[0][0] = (adpcm_page_struct*)((uint8_t*)pmap_bb+sizeof(bad_block_map_struct));
  for(int i = 1; i < MAX_CHANNEL; i ++) padpcm[0][i] = (adpcm_page_struct*)((uint8_t*)padpcm[0][i-1]+sizeof(adpcm_page_struct));
  
  padpcm[1][0] = (adpcm_page_struct*)((uint8_t*)padpcm[0][15]+sizeof(adpcm_page_struct));
  for(int j = 1; j < MAX_CHANNEL; j ++) padpcm[1][j] = (adpcm_page_struct*)((uint8_t*)padpcm[1][j-1]+sizeof(adpcm_page_struct)); 
   
  memset(prsb,0,sizeof(super_block_struct));
  memset(pwsb,0,sizeof(super_block_struct));
  memset(pmap_bb,0,sizeof(bad_block_map_struct));
  memset(padpcm[0][0],0,sizeof(adpcm_page_struct));
  memset(padpcm[1][0],0,sizeof(adpcm_page_struct));


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
  
  page_real_read  = 0;
  page_real_write = 0;

  AT45_Read(AT45ADR_MAP_BB,(uint8_t*)pmap_bb,sizeof(bad_block_map_struct));
  CRC_ResetDR();
  crc = CRC_CalcBlockCRC((uint32_t*)pmap_bb,(sizeof(bad_block_map_struct)-4)/4); 
  
  if(crc != pmap_bb-> crc) status = 1;
  
  
  
  return status;
}

void reset_bad_block_map(void)
{
  if(pmap_bb)
  {
    memset(pmap_bb,0,sizeof(bad_block_map_struct));
    for(int i = 0; i < MAX_BLOCK; i++) pmap_bb->block_address[i] = BLOCK_GOOD;
    CRC_ResetDR();
    pmap_bb->crc = CRC_CalcBlockCRC((uint32_t*)pmap_bb,(sizeof(bad_block_map_struct)-4)/4); 
    AT45_Write(AT45ADR_MAP_BB,(uint8_t*)pmap_bb,sizeof(bad_block_map_struct));
  }
}


void WriteNandPage(void *padpcm_msg)
{  
  alarm_struct *palarm = &alarm_data[alarm_data[0].pcom->index];
  
  if(palarm->PageRealWrite == 0)
  {
    if(CheckBlockGetPageAdr(HEADER_MODE)) // ÏÐÎÂÅÐÊÀ ÖÅËÎÑÒÍÎÑÒÈ ÁËÎÊÀ ÏÎÄ ÇÀÃÎËÎÂÎÊ
    {
      pwsb->sb_num = palarm->super_block_real_write;
      pwsb->id = SUPER_BLOCK_ID;
      pwsb->status = SUPER_BLOCK_OPEN;
      pwsb->time_open = GetTime();
      pwsb->super_block_this = palarm->PageAddress;
      
      pwsb->super_block_prev = palarm->super_block_current;
      palarm->super_block_current = palarm->PageAddress;
      palarm->PageAddress += PAGE_IN_BLOCK; 
     
      if(CheckBlockGetPageAdr(DATA_MODE)) // ÏÐÎÂÅÐÊÀ ÖÅËÎÑÒÍÎÑÒÈ ÁËÎÊÀ ÏÎÄ ÄÀÍÍÛÅ (ÍÀ×ÀËÜÍÀß ÑÒÐÀÍÈÖÀ ÄÀÍÍÛÕ)
      {
        if(write_page(padpcm_msg,palarm->PageAddress) == 1)
        {
          palarm->PageRealWrite++;
          palarm->PageAddress++;
        }
	else rewrite_block(REWRITE_BLOCK,padpcm_msg);
      }
    }
  }
  else if(palarm->PageRealWrite == (MAX_PAGE-1))
  {
    if(CheckBlockGetPageAdr(DATA_MODE)) // ÏÐÎÂÅÐÊÀ ÖÅËÎÑÒÍÎÑÒÈ ÁËÎÊÀ ÏÎÄ ÄÀÍÍÛÅ (ÊÎÍÅ×ÍÀß ÑÒÐÀÍÈÖÀ ÑÓÏÅÐ-ÁËÎÊÀ)
    {
      if(write_page(padpcm_msg,palarm->PageAddress) == 1)
      {        
        pwsb->page_real_write = palarm->PageRealWrite;
        pwsb->sb_num = palarm->super_block_real_write;
        palarm->PageRealWrite = 0; 
        palarm->PageAddress++;
      }
      else 
      {
        rewrite_block(REWRITE_BLOCK,padpcm_msg);
        palarm->PageRealWrite = 0; 
      }
    
      if(CheckBlockGetPageAdr(HEADER_MODE)) // ÏÐÎÂÅÐÊÀ ÖÅËÎÑÒÍÎÑÒÈ ÁËÎÊÀ ÏÎÄ ÇÀÃÎËÎÂÎÊ ÑËÅÄÓÞÙÅÃÎ ÑÓÏÅÐ ÁËÎÊÀ
      {
        pwsb->super_block_prev = palarm->super_block_prev;
	pwsb->super_block_next = palarm->PageAddress;    
        pwsb->status = SUPER_BLOCK_RECORDED;
        pwsb->time_close = GetTime();
        
        WriteSuperBlockHeader();
	
        palarm->super_block_prev = palarm->super_block_current;
        palarm->super_block_time[1] = pwsb->time_close;   // ÂÐÅÌß ÏÎÑËÅÄÍÅÃÎ ÇÀÏÈÑÀÍÍÎÃÎ ÑÓÏÅÐ ÁËÎÊÀ 
        palarm->super_block_real_write++;                 // ÍÎÌÅÐ ÏÎÑËÅÄÍÅÃÎ ÇÀÏÈÑÀÍÍÎÃÎ ÑÓÏÅÐ ÁËÎÊÀ
        palarm->super_block_real_read++;
        
      }
//    else CriticalError();
    }
  }
  else
  {
    if(CheckBlockGetPageAdr(DATA_MODE))  // ÏÐÎÂÅÐÊÀ ÖÅËÎÑÒÍÎÑÒÈ ÁËÎÊÀ ÏÎÄ ÄÀÍÍÛÅ (ÒÅÊÓÙÀß ÑÒÐÀÍÈÖÀ)
    {
      if(write_page(padpcm_msg,palarm->PageAddress) == 1)
      {
        palarm->PageRealWrite++; 
        palarm->PageAddress++;
      }
      else rewrite_block(REWRITE_BLOCK,padpcm_msg);
    }
  }
}


int CheckBlockGetPageAdr(int mode)
{
  alarm_struct *palarm = &alarm_data[alarm_data[0].pcom->index];
  
  uint32_t block_adr = palarm->PageAddress >> 6;

  if(pmap_bb->block_address[block_adr] == BLOCK_BAD)
  {
    block_adr++;
    block_adr &= 0xfff;
    palarm->PageAddress = block_adr << 6;
    return CheckBlockGetPageAdr(mode);
  }
  else if(pmap_bb->block_address[block_adr] == BLOCK_GOOD)
  {
    if(mode == HEADER_MODE) 
    {
      if((palarm->PageAddress & 0x3f) > 0) 
      {
        palarm->PageAddress = block_adr+1 << 6;
      }
    }
    else if(mode == DATA_MODE)
    {
      pwsb->ips[palarm->PageRealWrite].page_address = palarm->PageAddress;
      pwsb->ips[palarm->PageRealWrite].page_index = ADPCM_PAGE_ID;
    }

    return 1;
  }

  return 0;
}

void WriteSuperBlockHeader(void)
{ 
  alarm_struct *palarm = &alarm_data[alarm_data[0].pcom->index];
  
  uint8_t *pbuffer = (uint8_t*)pwsb;
  uint8_t buffer[2048];  

  nand_erase_block(0,palarm->super_block_current);
      
  for(int i = 0; i < 64; i++) 
  {
    memcpy(buffer,pbuffer,2048);
    nand_8bit_write_page(0,buffer,palarm->super_block_current+i);  
    pbuffer+=2048;
  }
  
   GPIO_ToggleBits(GPIOI, GPIO_Pin_0); 
   pwsb->sb_num++;
}


void ReadSuperBlockHeader(uint32_t adr, int read_mode)
{
   uint8_t *pbuffer = (uint8_t*)prsb;
  
    for(int i = 0; i < 64; i++) 
    {
      nand_8bit_read_page(0,pbuffer,adr+i);
      pbuffer+=2048;
      if(read_mode) break;
    }
    
    delay(0);
}

uint64_t GetTime(void)
{ alarm_struct *palarm = &alarm_data[alarm_data[0].pcom->index];
  return (uint64_t) palarm->pcom->time;
}


int write_page(uint8_t *page_bufer,unsigned long int page)
{
  if((page & 0x3f) == 0) 
  {
    GPIO_ToggleBits(GPIOI, GPIO_Pin_0); 
    nand_erase_block(0,page);
    GPIO_ToggleBits(GPIOI, GPIO_Pin_0); 
  }
  nand_8bit_write_page(0,page_bufer,page);
  return 1;
}

void rewrite_block(int mode_rewrite,uint8_t *page_buffer_old)
{
  static uint32_t start_adr;
  static uint32_t stop_adr;
  static uint32_t byte_real_write;
  static uint8_t  page_buffer[2048];
  static int rewrite_num;
  alarm_struct *palarm = &alarm_data[alarm_data[0].pcom->index];
  
  if(mode_rewrite == REWRITE_BLOCK)
  {
    start_adr = palarm->PageAddress & 0xFFFFFFC0;
    stop_adr  = palarm->PageAddress;
    byte_real_write = stop_adr - start_adr;    	
    pmap_bb->block_address[start_adr >> 6] = BLOCK_BAD;
    rewrite_num = 1;
//  PageRealRead = 0; // ÷èòàþ äëÿ ïðîâåðêè !!!
  }
  else if(mode_rewrite == REWRITE_BLOCK_NEXT)
       {
         pmap_bb->block_address[palarm->PageAddress >> 6] = BLOCK_BAD;
	 rewrite_num++;
       }
	
  palarm->PageAddress = ((palarm->PageAddress >> 6) + 1) << 6;

  if(CheckBlockGetPageAdr(DATA_MODE)) // ÏÐÎÂÅÐÊÀ ÖÅËÎÑÒÍÎÑÒÈ ÁËÎÊÀ ÏÎÄ ÄÀÍÍÛÅ (ÏÅÐÅÇÀÏÈÑÛÂÀÅÌÛ ÁËÎÊ)
  {
    for(uint32_t i = 0; i < byte_real_write; i++)
    {
      // nand_read_page(page_buffer,start_adr+i,fp[1]);
      // PageRealRead ++;

      if(write_page(page_buffer,palarm->PageAddress) == 1)
      {
        palarm->PageAddress++;
      }
      else 
      {
        rewrite_block(REWRITE_BLOCK_NEXT,page_buffer_old);
        return;
      }
    }

    if(write_page(page_buffer_old,palarm->PageAddress) == 1)
    {
      palarm->PageRealWrite++; 
      palarm->PageAddress++;
    }
  }
}

void erase_block(uint32_t id, uint32_t start_adr, uint32_t end_adr)
{
  start_adr *= 64;
  end_adr *= 64;
 
  while(start_adr  <= end_adr)
  {
    nand_erase_block(id,start_adr);
    start_adr+=64;
  }
}





