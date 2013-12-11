#include "main.h"
#include "nand_hw_driver.h"

#define REWRITE_BLOCK        1
#define REWRITE_BLOCK_NEXT   2

extern alarm_struct            alarm_data[2];
extern super_block_struct     *pwsb;
extern bad_block_map_struct   *pmap_bb;
extern uint8_t                 adpcm_ready;
extern adpcm_page_ctrl_struct  adpcm_ctrl[MAX_CHANNEL];
extern adpcm_page_struct      *padpcm[2][MAX_CHANNEL]; 

static int write_page(uint8_t *page_bufer,unsigned long int page);
static int CheckBlockGetPageAdr(int mode);
static void WriteSuperBlockHeader(void);
static void rewrite_block(int mode_rewrite,uint8_t *page_buffer_old);
static void WriteNandPage(void *padpcm_msg);



void nand_sb_write_handler(void)
{   
  if(adpcm_ready == 0) return;

  for(int i = 0; i < MAX_CHANNEL; i++)
  {
    if(adpcm_ctrl[i].done == 1)
    {
      int id = adpcm_ctrl[i].id ^ 0x1;
       
      GPIO_ToggleBits(GPIOI, GPIO_Pin_1); 
        
      WriteNandPage(padpcm[id][i]);
      
      adpcm_ctrl[i].done = 0;
    }
  }
  adpcm_ready = 0;
}


static void WriteNandPage(void *padpcm_msg)
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
               
        GPIO_ToggleBits(GPIOI, GPIO_Pin_0); 
	
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


static int CheckBlockGetPageAdr(int mode)
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

static void WriteSuperBlockHeader(void)
{ 
  alarm_struct *palarm = &alarm_data[alarm_data[0].pcom->index];
  
  uint32_t index = palarm->pcom->index;
    
  uint8_t *pbuffer = (uint8_t*)pwsb;
  uint8_t buffer[2048];  

  nand_erase_block(index,palarm->super_block_current);
      
  for(int i = 0; i < 64; i++) 
  {
    memcpy(buffer,pbuffer,2048);
    nand_8bit_write_page(index,buffer,palarm->super_block_current+i);  
    pbuffer+=2048;
  }
  
   pwsb->sb_num++;
}



/*
static void ReadSuperBlockHeader(uint32_t adr, int read_mode)
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
*/


static int write_page(uint8_t *page_bufer,unsigned long int page)
{
  uint32_t index = alarm_data[0].pcom->index;
                         
  if((page & 0x3f) == 0) 
  {
    nand_erase_block(index,page);
  }
  nand_8bit_write_page(index,page_bufer,page);
  return 1;
}


static void rewrite_block(int mode_rewrite,uint8_t *page_buffer_old)
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






