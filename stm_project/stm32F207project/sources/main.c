#include "main.h"
#include "debug.h"
#include "ethernet.h"
#include "at24c512.h"
#include "nand_hw_driver.h"

////////////////////////////////////////////////////////////////////////////////
// VARIABLE * VARIABLE * VARIABLE * VARIABLE * VARIABLE * VARIABLE * VARIABLE * 
////////////////////////////////////////////////////////////////////////////////

tcp_message_struct         tx_tcp_msg;
tcp_message_struct         rx_tcp_msg;
initial_data_struct        initial_data;
uint8_t                    rls_msg_buffer[1024];
__IO uint32_t              LocalTime = 0;
int                        rew_status  = 0;
int                        page_per_az = 0;
int                        mode = 0;
int                        get_crc_flag = 0;
int                        nand_erase_flag = 0;

int32_t us_timer_az[3] = {1,1,1000};

int page_per_az;
int el_ext_ena = 0;

// agfhytfufg

struct 
{
  uint16_t EL;
  uint16_t AZ;
  uint16_t data[2048];

  //uint8_t  data[5][2048];

} f_data;

  
////////////////////////////////////////////////////////////////////////////////
// MAIN * MAIN * MAIN * MAIN * MAIN * MAIN * MAIN * MAIN * MAIN * MAIN * MAIN * 
////////////////////////////////////////////////////////////////////////////////

// preved medved ++

#define TCP_ETH

void main()
{
  unsigned long int data_r = 0;
  initial_data.DAZ = 0.9;
  initial_data.DEL = 0.9;
  initial_data.NAZ = 400;
  initial_data.NEL = 21;
  initial_data.NS  = 1000;
  page_per_az      = 1;
  
  do
  { data_r = *(__IO uint16_t*) (sram_bank3 + 2);
  } while (data_r != 0x33A5);
 
  LED_Config();
  AT24_Config();
  
#ifdef TCP_ETH
  
  LWIP_Config();
  TCP_SERVER_Config();
  UDP_Config();
  
#endif
  
  NAND_Config();
        
  CAN_Config();
  
  *(uint16_t *)(sram_bank4 + 4) = 50000 / 1000;

  
 TIM5_Config(10); // точность - микросекунд
 
 TIM1_Config();
  
/*  
#define PAGE 1
  
 uint16_t data[2048];
  nand_erase_block(1,0);
  nand_16bit_read_page(1,data,PAGE);
  for(int i = 0; i < 2048; i++) data[i] = i;
  nand_16bit_write_page(1,data,PAGE);
  nand_16bit_read_page(1,data,PAGE);
  */

  GPIO_ToggleBits(GPIOI, GPIO_Pin_1); 
	
#ifdef TCP_ETH
  
  while(1)
  {
    if(ETH_CheckFrameReceived())
    {
      LwIP_Pkt_Handle();
    }
      
    LwIP_Periodic_Handle(LocalTime);
    
    if(rew_status > 0) ReSendTcpData(rew_status);
 
    get_crc_handler();
  
    nend_erase_handler();

  }
  
#else
  
  while(1);
  
#endif
 
}

void nend_erase_handler(void)
{
  static uint32_t erase_adr = 0;
  static uint32_t step;
  static uint32_t erase_index = 0;   
  static int total_page;
    
  if(nand_erase_flag == 0) 
  {
    return;
  }
  else if(nand_erase_flag == 1) 
  {
    erase_adr = 0;
    memcpy(&initial_data,&rx_tcp_msg.data[0],sizeof(initial_data));
     
    total_page = (initial_data.NEL * initial_data.NAZ) + initial_data.NAZ;
    
    step = total_page / 10;
    erase_index = 1;
    nand_erase_flag++;
  }
  
  delay(200); 
  
  if(erase_adr >= total_page)
  {
    tx_tcp_msg.msg_id  = SET_INITIAL_DATA + 100;
    tx_tcp_msg.msg_len = 0;
    tx_tcp_msg.msg_crc = crc32((uint8_t*)&tx_tcp_msg+4,(TCP_HEADER_SIZE-4) + tx_tcp_msg.msg_len,sizeof(rx_tcp_msg)); 
    SendTcpData(&tx_tcp_msg,TCP_HEADER_SIZE+tx_tcp_msg.msg_len);
    nand_erase_flag = 0;
  }
  else
  {
    nand_erase_block(1,erase_adr+=64);
  
    if(erase_adr > step * erase_index)
    {
       tx_tcp_msg.msg_id  = SET_INITIAL_DATA + 200;
       tx_tcp_msg.msg_len = 4;
       memcpy(&tx_tcp_msg.data[0],&erase_index,4);
       tx_tcp_msg.msg_crc = crc32((uint8_t*)&tx_tcp_msg+4,(TCP_HEADER_SIZE-4) + tx_tcp_msg.msg_len,sizeof(rx_tcp_msg)); 
       SendTcpData(&tx_tcp_msg,TCP_HEADER_SIZE+tx_tcp_msg.msg_len);
       erase_index++; 
    }
  }
  
  
  
}

void get_crc_handler(void)
{
  static uint32_t total_crc;
  static uint32_t total_az;
  static uint32_t f_data_size;
  static uint32_t page_adr;
  static uint32_t step;
  static uint32_t az_index;
  
  if(get_crc_flag == 0) 
  {
    return;
  }
  else if(get_crc_flag == 1) 
  {
    total_crc = -1;
    page_adr = 0;
    az_index = 0;
    f_data.AZ = 0;
    f_data.EL = 0;
    memset(&f_data,0,sizeof(f_data));
    memcpy(&total_az,&rx_tcp_msg.data[0],sizeof(total_az));
    f_data_size = (initial_data.NS * 2) + 4;
    step = total_az / 100;
    get_crc_flag++;
  }

   nand_16bit_read_page(1,(uint16_t*)&f_data.data[0],page_adr++);
   total_crc = crc32_t(total_crc,&f_data,f_data_size,f_data_size);

   if(f_data.AZ++ >= initial_data.NAZ-1)
   {
     f_data.AZ = 0;
     f_data.EL ++;
   }
   
   if(page_adr == total_az)
   {  
  //   delay(300); 
     tx_tcp_msg.msg_id  = GET_CRC + 100;
     tx_tcp_msg.msg_len = 4;
     memcpy(&tx_tcp_msg.data[0],&total_crc,4);
     tx_tcp_msg.msg_crc = crc32((uint8_t*)&tx_tcp_msg+4,(TCP_HEADER_SIZE-4) + tx_tcp_msg.msg_len,sizeof(rx_tcp_msg)); 
     SendTcpData(&tx_tcp_msg,TCP_HEADER_SIZE+tx_tcp_msg.msg_len);
     get_crc_flag = 0;
   
   }
   else if((az_index < 95) && (page_adr > step * az_index))
   {
     tx_tcp_msg.msg_id  = GET_CRC + 200;
     tx_tcp_msg.msg_len = 4;
     memcpy(&tx_tcp_msg.data[0],&az_index,4);
     tx_tcp_msg.msg_crc = crc32((uint8_t*)&tx_tcp_msg+4,(TCP_HEADER_SIZE-4) + tx_tcp_msg.msg_len,sizeof(rx_tcp_msg)); 
     SendTcpData(&tx_tcp_msg,TCP_HEADER_SIZE+tx_tcp_msg.msg_len);
     az_index++; 
   }
 
}



void WriteNandPage(mode2_msg_data_struct* pmsg)
{
  //if(page_per_az == 0) return;
  //int page_adr = (pmsg->EL * initial_data.NAZ * page_per_az) + (pmsg->AZ * page_per_az) + pmsg->index_az;
  //nand_8bit_write_page(1,&pmsg->data[0],page_adr);
  
  int page_adr = (pmsg->EL * initial_data.NAZ) + pmsg->AZ;
  
  nand_16bit_write_page(1,(uint16_t*)pmsg+2,page_adr);
  
  delay(20);

}



/*
uint32_t GetCrc(void)
{
  int i,j;
  uint32_t total_crc = -1;
  uint32_t page_adr = 0;
  memset(&f_data,0,sizeof(f_data));
 
  NAND_Config();
  delay(100);
  
  f_data.AZ = 0;
  f_data.EL = 0;
  
  memcpy(&crc_initial_info,&rx_tcp_msg.data[0],sizeof(crc_initial_info));

  for(i = 0; i < crc_initial_info.total_az; i++)
  {
   // for(j = 0; j < crc_initial_info.page_per_az; j++)
    {
       nand_16bit_read_page(1,(uint1_t*)&f_data.data[0],page_adr++);
    }
       
    total_crc = crc32_t(total_crc,&f_data,crc_initial_info.byte_per_az+4,crc_initial_info.byte_per_az+4);
     
    if(f_data.AZ++ >= crc_initial_info.NAZ-1)
    {
      f_data.AZ = 0;
      f_data.EL ++;
    }
  }
  
  
  //NAND_Config();

  return total_crc;
}   
*/



////////////////////////////////////////////////////////////////////////////////
// SYSTEM * SYSTEM * SYSTEM * SYSTEM * SYSTEM * SYSTEM * SYSTEM * SYSTEM * SYSTE
//////////////////////////////////////////////////////////////////////////////// 
void SysTick_Handler(void)
{
  LocalTime += SYSTEMTICK_PERIOD_MS;
     
}

void TIM5_IRQHandler(void)
{
  TIM_ClearITPendingBit(TIM5,TIM5_IRQn);

  us_timer_az[0]++;

  if(us_timer_az[1] == 0)
  {
      #ifdef ENA_EXTRAPOL
       GetNextAZ(TIM_MODE);
      #endif
    
     us_timer_az[1] = us_timer_az[2];
    }
    else if(us_timer_az[1] > 0) 
    {
      us_timer_az[1]--;
    }

}



void delay(uint32_t nCount)
{
  uint32_t timingdelay = LocalTime + nCount;  
  while(timingdelay > LocalTime);
}


uint32_t crc32(void* pcBlock,  uint32_t len, uint32_t tot_len)
{
  uint8_t *p = (uint8_t*) pcBlock;
  
  if(len > tot_len) return 0;
    
	static uint32_t Crc32Table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
    0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
    0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
    0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
    0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
    0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
    0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
    0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
    0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
    0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
    0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
    0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
    0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
    0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
    0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
    0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
    0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
    0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
    0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
    0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
    0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
    0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

    uint32_t crc = -1;
    while (len--)
        crc = (crc >> 8) ^ Crc32Table[(crc ^ *p++) & 0xFF];
    return crc ^ 0xFFFFFFFF;
}

uint32_t crc32_t(uint32_t crc, void * pcBlock, uint32_t len, uint32_t tot_len)
{
	if(len > tot_len) return 0;

	uint8_t *p = (uint8_t*)pcBlock;
	
	static unsigned int Crc32Table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
    0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
    0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
    0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
    0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
    0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
    0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
    0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
    0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
    0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
    0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
    0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
    0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
    0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
    0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
    0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
    0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
    0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
    0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
    0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
    0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
    0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

    while (len--)
	{ 
		crc = (crc >> 8) ^ Crc32Table[(crc ^ *p++) & 0xFF];
	}
    
    //return crc ^ 0xFFFFFFFF;
    return crc; 
}
