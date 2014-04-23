#include "stm32f2xx.h"
#include "stm32f2x7_eth.h"
#include "stm32f2xx_can.h"
#include "stm32f2x7_eth_bsp.h"   
#include "stm32f2xx_conf.h"
#include "stm32f2xx_tim.h"
#include "stm32f2xx_rtc.h"
#include "stm32f2xx_pwr.h"
#include "stm32f2xx_dma.h"
#include "stm32f2xx_crc.h"  
#include "stm32f2xx_i2c.h"   
#include "stm32f2xx_spi.h"   
#include "netconf.h"
#include "lwip\udp.h"
#include "lwip\tcp.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"

////////////////////////////////////////////////////////////////////////////////
// SYSTEM * SYSTEM * SYSTEM * SYSTEM * SYSTEM * SYSTEM * SYSTEM * SYSTEM * SYSTE 
////////////////////////////////////////////////////////////////////////////////
//#define ENA_EXTRAPOL
//#define ENA_NAND_HANDLER
//#define PTD3

#define MAX_PAGE             5 
#define RND_CONST            1024
#define SYSTEMTICK_PERIOD_MS 100

#define	sram_bank1	     0x60000000
#define	sram_bank2	     0x64000000
#define sram_bank3	     0x68000000
#define sram_bank4	     0x6C000000

////////////////////////////////////////////////////////////////////////////////

#define CHECK_CONNECT        0

#define SEND_DATA_MODE_1     1
#define SEND_DATA_MODE_2     2

#define SET_DATA_DAC         20
#define SET_FREQ_DAC         21
#define SET_MODE_1           22
#define SET_MODE_2           23
#define SET_INITIAL_DATA     24
#define SET_ETH_PARAM        25

#define GET_ETH_PARAM        40
#define GET_CRC              41


#define START                70
#define STOP                 71


#define SET_MAC              1
#define SET_GW               2
#define SET_IP               3
#define SET_MASK             4
#define SET_TCP_RX_PORT      5
#define SET_UDP_RX_PORT      6

#define TCP_DATA_SIZE        8192
#define TCP_HEADER_SIZE      12
#define TCP_MESSAGE_SIZE     TCP_HEADER_SIZE + TCP_DATA_SIZE

#define AT45_TIMEOUT         0xffffffff
#define I2C_SPEED            100000
 
#define CAN_MODE             1
#define TIM_MODE             2


#pragma pack(push,1)   
 typedef struct  
 { uint32_t msg_crc;
   uint32_t msg_id;
   uint32_t msg_len; 
   uint8_t  data[TCP_DATA_SIZE];
 } tcp_message_struct;
#pragma pack(pop)

#pragma pack(push,1)   
typedef struct  
{
  float   DEL;
  float   DAZ;
  uint16_t NEL;
  uint16_t NAZ;
  uint16_t NS;
} initial_data_struct;
#pragma pack(pop)

#pragma pack(push,1)   
typedef struct 
{
  uint16_t EL;
  uint16_t AZ;
  uint16_t data[2048];
} mode2_msg_data_struct;
#pragma pack(pop)


#pragma pack(push,1)   
typedef struct 
{
  uint8_t  MAC_ADR[6];
  uint8_t  GW_ADR[4];
  uint8_t  IP_ADR[4];
  uint8_t  MASK[4];
  
  uint32_t TCP_RX_PORT;
  uint32_t UDP_RX_PORT;
  
  uint32_t crc;
} ethernet_initial_struct;
#pragma pack(pop)

////////////////////////////////////////////////////////////////////////////////
// FUNCTION_DEFINITION * FUNCTION_DEFINITION * FUNCTION_DEFINITION * FUNCTION_DE
////////////////////////////////////////////////////////////////////////////////

void TIM1_Config(void);
void TIM7_Config(uint32_t period);
void TIM5_Config(uint32_t period);
void delay(uint32_t delay);
void eth_cmd_handler(void);
void CAN_Config(void);
void LED_Config(void);

void SendTcpData(void* pbuffer, int len);
void SendUdpData(uint8_t* pbuffer, int len);

void ReSendTcpData(int mode);
void ReSendUdpData(void);

void NAND_Config(void);
void SysTim_Config(void);
void get_crc_handler(void);
void nend_erase_handler(void);

uint32_t crc32(void* pcBlock,  uint32_t len, uint32_t tot_len);
uint32_t crc32_t(uint32_t crc, void * pcBlock, uint32_t len, uint32_t tot_len);
uint32_t GetCrc(void);

void WriteNandPage(mode2_msg_data_struct* pmsg);
int LWIP_Config(void);
void GetNextAZ(int mode);
void GetNextEL(int mode);