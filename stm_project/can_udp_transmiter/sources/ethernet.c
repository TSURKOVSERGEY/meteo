#include "main.h"
#include "ethernet.h"
#include "spi_f207.h"
#include "at24c512.h"

struct pbuf*            pOut;
ethernet_initial_struct eth_ini_dat;
struct                  udp_pcb *pudp_pcb;
extern int              rew_status;

uint8_t udp_buffer[1024];
uint32_t udp_len;

int LWIP_Config(void)
{
  eth_ini_dat.IP_ADR[0] = IP_ADDR0;
  eth_ini_dat.IP_ADR[1] = IP_ADDR1;
  eth_ini_dat.IP_ADR[2] = IP_ADDR2;
  eth_ini_dat.IP_ADR[3] = IP_ADDR3;
    
  eth_ini_dat.GW_ADR[0] = GW_ADDR0;
  eth_ini_dat.GW_ADR[1] = GW_ADDR1;
  eth_ini_dat.GW_ADR[2] = GW_ADDR2;
  eth_ini_dat.GW_ADR[3] = GW_ADDR3;
      
  eth_ini_dat.MASK[0] = NETMASK_ADDR0;
  eth_ini_dat.MASK[1] = NETMASK_ADDR1;
  eth_ini_dat.MASK[2] = NETMASK_ADDR2;
  eth_ini_dat.MASK[3] = NETMASK_ADDR3;

  eth_ini_dat.MAC_ADR[0] = MAC_ADDR0;
  eth_ini_dat.MAC_ADR[1] = MAC_ADDR1;
  eth_ini_dat.MAC_ADR[2] = MAC_ADDR2;
  eth_ini_dat.MAC_ADR[3] = MAC_ADDR3;
  eth_ini_dat.MAC_ADR[4] = MAC_ADDR4;
  eth_ini_dat.MAC_ADR[5] = MAC_ADDR5;
    
  eth_ini_dat.TCP_RX_PORT = STM_PORT_RX_TCP;
  eth_ini_dat.UDP_RX_PORT = STM_PORT_TX_UDP;

  if(ETH_BSP_Config() == ETH_ERROR) return 0;
   
  LwIP_Init();
    
  return 1;
}


void UDP_Config(void)
{
  if((pudp_pcb = udp_new()) == NULL)  return;
  
  pbuf_free(pOut);
 
  pOut = pbuf_alloc(PBUF_TRANSPORT,1024,PBUF_RAM);
  
}

void SendMessage(uint8_t* pbuffer, int len)
{
 
//  pbuf_free(pOut);
// 
//  pOut = pbuf_alloc(PBUF_TRANSPORT,len,PBUF_RAM);
  
  pOut->tot_len = len;
  
  udp_len = len;
  memcpy(udp_buffer,pbuffer,len);
     
  if(pbuf_take(pOut,udp_buffer,len) == ERR_OK)
  {
     if(udp_sendto(pudp_pcb,pOut,IP_ADDR_BROADCAST,30785) != ERR_OK)  rew_status = 2;
  }
  else rew_status = 1;

}

void ReSendUdpData(void)
{
  if(rew_status == 1)
  {  
    pOut->tot_len = udp_len;
    
    if(pbuf_take(pOut,udp_buffer,udp_len) == ERR_OK)
    {
      if(udp_sendto(pudp_pcb,pOut,IP_ADDR_BROADCAST,30785) != ERR_OK)  rew_status = 2;
    }
  }
  else if(rew_status == 2)
  {
    if(udp_sendto(pudp_pcb,pOut,IP_ADDR_BROADCAST,30785) == ERR_OK) rew_status = 0;
  
  }
}




