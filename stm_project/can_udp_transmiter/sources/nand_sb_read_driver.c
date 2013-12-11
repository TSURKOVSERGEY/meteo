#include "main.h"
#include "ethernet.h"
#include "nand_hw_driver.h"

#define ONE_PAGE   1
#define ALL_PAGE   0

extern struct                  tcp_pcb *pout_pcb;
extern                         tcp_message_struct  tx_tcp_msg;
extern alarm_struct            alarm_data[2];
extern super_block_struct      *prsb;

static int GetSuperBlockAdr(uint32_t *padr, uint32_t sb_num);
static void ReadSuperBlockHeader(uint32_t adr, int read_mode);
static int CheckSuperBlock(void);
extern int rew_status;

int SetReadSuperBlock(uint32_t sb_begin, uint32_t sb_end)
{
  uint32_t page_adr;
  uint32_t index = alarm_data[0].pcom->index^1;
  alarm_struct *palarm = &alarm_data[index];
  
  if(!GetSuperBlockAdr(&page_adr,sb_begin)) 
  {
    tx_tcp_msg.msg_id  = GET_SB_FAILURE;
    tx_tcp_msg.msg_len = 0;
   
    if(tcp_write(pout_pcb,&tx_tcp_msg,TCP_MESSAGE_SIZE+TCP_HEADER_SIZE,1) == ERR_OK) 
    {
      tcp_output(pout_pcb);
      tcp_sent(pout_pcb,tcp_callback_sent);  
    }
    else 
    {
      rew_status = 1;
      return 0;
    }
  }
  else ReadSuperBlockHeader(page_adr,ALL_PAGE);
   
  if(CheckSuperBlock()) palarm->super_block_real_read = 0; 
  else palarm->super_block_real_read = sb_end - sb_begin + 1;
  
  return 1;
}


void get_sb_header(void)
{
  struct super_block_header
  {
    uint64_t time_open;
    uint64_t time_close;
    uint32_t sb_num;
    uint32_t page_real_write;
  } *psbh = (struct super_block_header*) &tx_tcp_msg.data[0];
    
  psbh->time_open       = prsb->time_open;
  psbh->time_close      = prsb->time_close;    
  psbh->sb_num          = prsb->sb_num;      
  psbh->page_real_write = prsb->page_real_write;  
    
  tx_tcp_msg.msg_id  = GET_SB_HEADER;
  tx_tcp_msg.msg_len = sizeof(struct super_block_header);
  tx_tcp_msg.msg_crc = crc32((uint8_t*)&tx_tcp_msg+4,(TCP_HEADER_SIZE-4) + tx_tcp_msg.msg_len); 
  
  if(tcp_write(pout_pcb,&tx_tcp_msg,TCP_MESSAGE_SIZE+TCP_HEADER_SIZE,1) == ERR_OK) 
  {
    tcp_output(pout_pcb);
    tcp_sent(pout_pcb,tcp_callback_sent);  
  }
  else rew_status = 1;
}


void nand_sb_read_handler(void)
{
  static uint32_t page_real_write = 0;
  static int adr = 0;
 // static void *ptr = NULL;  
  
  uint32_t index = alarm_data[0].pcom->index^1;
 
  alarm_struct *palarm = &alarm_data[index];
  
  if(palarm->super_block_real_read == 0) return;
  
  if(prsb->status != SUPER_BLOCK_READ)
  {
    page_real_write = 0;
    prsb->status = SUPER_BLOCK_READ;
    get_sb_header();
    return;
  }

  if(page_real_write <= prsb->page_real_write)
  { 
    static uint8_t buffer[2048];
    
    struct page_address_struct
    {
      uint16_t sb_adr;
      uint16_t page_real_write;
      uint16_t page_adr;
      uint8_t  half_page_adr;
      uint8_t  unit_index;
      
    } *padr = (struct page_address_struct*) &tx_tcp_msg.data[0];
       
    #define PADR_SIZE   8
      
    uint32_t half_adpcm_size = sizeof(adpcm_page_struct) / 2;
      
    tx_tcp_msg.msg_id  = GET_SBLOCK;
    tx_tcp_msg.msg_len = PADR_SIZE + half_adpcm_size;
   
    padr->sb_adr = prsb->sb_num;
    padr->page_adr = page_real_write;
    padr->page_real_write = prsb->page_real_write;
    padr->half_page_adr = adr;
    padr->unit_index = index;
       
    switch(adr)
    {
       case 0:  nand_8bit_read_page(index,&buffer[0],prsb->ips[page_real_write].page_address);
                memcpy(&tx_tcp_msg.data[8],&buffer[0],half_adpcm_size);
                adr++;
       break;
       case 1:
                memcpy(&tx_tcp_msg.data[8],&buffer[half_adpcm_size],half_adpcm_size);
                page_real_write++;   
                adr = 0;
       break;
     }

     tx_tcp_msg.msg_crc = crc32((uint8_t*)&tx_tcp_msg+4,(TCP_HEADER_SIZE-4) + tx_tcp_msg.msg_len); 
     
     if(tcp_write(pout_pcb,&tx_tcp_msg,TCP_MESSAGE_SIZE+TCP_HEADER_SIZE,1) == ERR_OK) 
     {
       tcp_output(pout_pcb);
       tcp_sent(pout_pcb,tcp_callback_sent);  
     } 
     else rew_status = 1;
   
  }
  else if(palarm->super_block_real_read-- > 0)
  {
    if(prsb->super_block_next != NULL) 
    {
      ReadSuperBlockHeader(prsb->super_block_next,ALL_PAGE);
    }
    
    if(CheckSuperBlock())
    {
      tx_tcp_msg.msg_id  = GET_SB_FAILURE;
      tx_tcp_msg.msg_len = 0;
      tx_tcp_msg.msg_crc = crc32((uint8_t*)&tx_tcp_msg+4,TCP_HEADER_SIZE-4);
      tcp_write(pout_pcb,&tx_tcp_msg,tx_tcp_msg.msg_len+TCP_HEADER_SIZE,1);
      tcp_output(pout_pcb);
      tcp_sent(pout_pcb,NULL);
        
      tx_tcp_msg.msg_id  = GET_SB_FAILURE;
      tx_tcp_msg.msg_len = 0;
      if(tcp_write(pout_pcb,&tx_tcp_msg,TCP_MESSAGE_SIZE+TCP_HEADER_SIZE,1) == ERR_OK) 
      {
        tcp_output(pout_pcb);
        tcp_sent(pout_pcb,tcp_callback_sent);  
      }
      else rew_status = 1;
       
      palarm->super_block_real_read = 0; 
    }
  
  }
}


static int GetSuperBlockAdr(uint32_t *padr, uint32_t sb_num)
{
  uint32_t index = alarm_data[0].pcom->index^1;
  alarm_struct *palarm = &alarm_data[index];
    
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
  
  *padr = adr;
  
  return 1;
}


static void ReadSuperBlockHeader(uint32_t adr, int read_mode)
{
   uint32_t index = alarm_data[0].pcom->index^1;
   uint8_t *pbuffer = (uint8_t*)prsb;
  
    for(int i = 0; i < 64; i++) 
    {
      nand_8bit_read_page(index,pbuffer,adr+i);
      pbuffer+=2048;
      if(read_mode) break;
    }
    
    delay(0);
}


static int CheckSuperBlock(void)
{
  if(prsb->id == SUPER_BLOCK_ID) return 0;
  else return 1;
}