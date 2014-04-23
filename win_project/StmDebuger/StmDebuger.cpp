// StmDebuger.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <conio.h>
#include "StmDebuger.h"
#include <iostream>
#include <string.h>
#include <MMSYSTEM.H>

#pragma comment (lib,"WSock32.Lib")
#include <WINSOCK.H>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////

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


//#define MAX_AZ_SIZE          2048*5

#define MAX_AZ_SIZE          2048


#define TCP_DATA_SIZE        8192 
#define TCP_HEADER_SIZE      12
#define TCP_MESSAGE_SIZE     TCP_HEADER_SIZE + TCP_DATA_SIZE

#define HANDLER_16_BIT

#define FOUT0	0
#define FOUT1	1
#define FOUT2	2
#define FOUT3	3
#define FOUT4	4

#define NOPEN	1
#define OPEN	2

/////////////////////////////////////////////////////////////////////////////

#pragma pack(push,1)  
    struct tcp_message
    {
		unsigned int   msg_crc;
        unsigned int   msg_id;
        unsigned int   msg_len;
        unsigned char  data[TCP_MESSAGE_SIZE];
    } tx_tcp_msg,rx_tcp_msg;
#pragma pack(pop)

#pragma pack(push,1)   
typedef struct 
		{
		  unsigned char MAC_ADR[6];
		  unsigned char GW_ADR[4];
		  unsigned char IP_ADR[4];
		  unsigned char MASK[4];
		  unsigned int  TCP_RX_PORT;
   		  unsigned int  UDP_RX_PORT;
		  unsigned int  crc;

		} ethernet_initial_struct;
#pragma pack(pop)

#pragma pack(push,1)  
	struct initial_data_struct
	{
		float DEL;
		float DAZ;
		unsigned short NEL;
        unsigned short NAZ;
        unsigned short NS;
	
	};
#pragma pack(pop)

#ifdef HANDLER_8_BIT

#pragma pack(push,1)  
	struct 
	{
		unsigned short EL;
		unsigned short AZ;
		unsigned char  data[MAX_AZ_SIZE];
	
	} f_data;
#pragma pack(pop)

#endif

#ifdef HANDLER_8_BIT

#pragma pack(push,1)  	
	struct mode2_msg_data_struct
	{
		unsigned short EL;
		unsigned short AZ;
		unsigned short index_az;
        unsigned char  data[2048];
	
	} *pmsg;
#pragma pack(pop)

#endif


#ifdef HANDLER_16_BIT

#pragma pack(push,1)  
	struct 
	{
		unsigned short EL;
		unsigned short AZ;
		unsigned short data[MAX_AZ_SIZE];
	
	} f_data;
#pragma pack(pop)

#endif

/////////////////////////////////////////////////////////////////////////////


SOCKET      m_hSocket;
SOCKET      client_socket = NULL; 
SOCKADDR_IN client_addres;

int  param1,param2,param3,param4,param5,param6;

FILE        *pf[5] = {NULL,NULL,NULL,NULL,NULL};

char         fname[100];

initial_data_struct initial_data;

/////////////////////////////////////////////////////////////////////////////


int  CMD_CheckConnect(void);
void CMD_SetEthParam(int mode);
void CMD_GetEthParam(void);
int  TcpConnect(void);
void SendTcpMessage(unsigned int msg_id, void* data, unsigned int len);
int RecvMessage(unsigned int msg_id, void* data, unsigned int len);
unsigned int crc32(void * pcBlock, unsigned short len, unsigned short tot_len);
unsigned int crc32(unsigned int crc, void * pcBlock, unsigned short len, unsigned short tot_len);
void CMD_SendDataMode1(void);
void CMD_SendDataMode2(void);
void CMD_SetDac(void);
void CMD_SetFreq(void);
void MrlsImitator(void);
void CMD_Start(void);
void CMD_Stop(void);
void TcpDisconnect(void);

//int  CMD_GetCrc(void);
int CMD_GetCrc(int mode);

void create_pila_file(void);
int  RecvMessage(void);
void CMD_CanHandler(void);
void OpenDebugFiles(int id, char* fname);

#ifdef HANDLER_8_BIT
void WriteNandPage(mode2_msg_data_struct *pmsg, int len);
#endif

#ifdef HANDLER_16_BIT
void WriteNandPage(void);
#endif

/////////////////////////////////////////////////////////////////////////////
// The one and only application object

CWinApp theApp;

void main_handler(void);

using namespace std;

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		cerr << _T("Fatal Error: MFC initialization failed") << endl;
		nRetCode = 1;
	}
	else
	{
		// TODO: code your application's behavior here.

		main_handler();
		CString strHello;
		strHello.LoadString(IDS_HELLO);
		cout << (LPCTSTR)strHello << endl;
	}

	return nRetCode;
}

void TcpDisconnect(void)
{
  if(client_socket) 
  {
	  closesocket(client_socket);
	  client_socket = NULL;
	  WSACleanup();
  }
}

int TcpConnect(void)
{
  WSADATA WSAData;   

  const char STM_ADR[] = {"192.9.206.250"}; 
  
  #define STM_PORT 30000

  if(client_socket)
  {
  	  closesocket(client_socket);
	  client_socket = NULL;
	  WSACleanup();
  }

  if((WSAStartup(MAKEWORD(1,1), &WSAData)) != 0 ) 
  { 
	  printf("\r Failure initial WSAStartup \n"); 
      return 0;
  }
    
  if((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
	  printf("\r Failure initial socket \n");  
      return 0;
  }

  client_addres.sin_family      = AF_INET;
  client_addres.sin_addr.s_addr = inet_addr(STM_ADR); 
  client_addres.sin_port        = htons(STM_PORT); 

  if((connect(client_socket,(const struct sockaddr*)&client_addres,sizeof(struct sockaddr_in))) != 0 )
  {
	  printf("\r Failure initial connect \n");  
	  return 0;
  }

  return 1;
}

int UdpInitial(void)
{
  WSADATA WSAData; 
  
  if(client_socket) 
  {
	  closesocket(client_socket);
	  client_socket = NULL;
	  WSACleanup();
	  Sleep(300);
  }

  if(WSAStartup(MAKEWORD(1,1), &WSAData) != 0)
  {
	  printf("\r Failure initial WSAStartup \n"); 
	  return 0;
  }

  client_socket = socket(AF_INET, SOCK_DGRAM, 0);
	
  if(client_socket == INVALID_SOCKET)
  { printf("\r Failure initial socket \n"); 
	return 0;
  }
 
  
  #define STM_PORT_A 30785

  client_addres.sin_family      = AF_INET;
  client_addres.sin_addr.s_addr = INADDR_ANY;
  client_addres.sin_port        = htons(STM_PORT_A);
   
  if(bind(client_socket,(LPSOCKADDR)&client_addres, sizeof(client_addres)) == SOCKET_ERROR)
  {
	  closesocket(client_socket);
	  printf("\r Failure initial Bind \n"); 
      return 0;
  }

	
  client_addres.sin_addr.s_addr = inet_addr("192.9.206.250"); 

  return 1;

}

int GetID(char *name)
{
  const char* buffer[] = 
  {	
	"connect",        // 0
    "load1",          // 1
    "load2",          // 2
	"dac",            // 3
	"freq",           // 4
	"eth_mac",        // 5
	"eth_gw",         // 6
    "eth_ip",         // 7
    "eth_mask",       // 8
    "eth_tcprxport",  // 9
    "eth_udprxport",  // 10
    "geteth",         // 11
	"mrls",           // 12
	"crc",            // 13
	"can",            // 14
	"start",          // 15

  };

  for(int i = 0; i <= 15; i++ )
  { if((strcmp(buffer[i],name)) == 0) return i;
  }
  
  return -1;
}


void OpenDebugFiles(int id, char* fname)
{
	pf[id] = fopen(fname,"w+"); 
}

void main_handler(void)
{
	char buffer[100];
	char cmd[100];


    printf("\n Wait ");

	for(int i = 0; i < 70; i++)
	{
		printf("*");
		Sleep(30);

	}

	printf("\n");


	create_pila_file();

    if(!CMD_CheckConnect())
	{
		Sleep(2000);
		return;
	}


	while(1)
	{
		printf("\n \r ");
		
		memset(buffer,0,sizeof(buffer));

		gets(buffer); 

		if(buffer[0] != 110) sscanf(buffer,"%s %d %d %d %d %d %d",cmd,&param1,&param2,&param3,&param4,&param5,&param6);


		if(GetID(cmd) == 1 || GetID(cmd) == 2)
		{
			sscanf(buffer,"%s %s",cmd,fname);
		}



   	    switch(GetID(cmd))
		{
		    case 0:  CMD_CheckConnect();         break;
            case 1:  CMD_SendDataMode1();        break;
			case 2:  CMD_SendDataMode2();        break;
			case 3:  CMD_SetDac();               break;
			case 4:  CMD_SetFreq();              break;

            case 5:  CMD_SetEthParam(1);         break;
            case 6:  CMD_SetEthParam(2);         break;
            case 7:  CMD_SetEthParam(3);         break;
            case 8:  CMD_SetEthParam(4);         break;
            case 9:  CMD_SetEthParam(5);         break;
            case 10: CMD_SetEthParam(6);         break;

            case 11: CMD_GetEthParam();          break;
		    case 12: MrlsImitator();             break;
            case 13: CMD_GetCrc(OPEN);           break;
            case 14: CMD_CanHandler();           break;
            case 15: CMD_Start();                break;


			default: printf("\r incorrect command \n");
			break;
		}
	}
}

int GetConnect(void)
{
  WSADATA WSAData;   

  const char STM_ADR[] = {"192.9.206.202"}; 


  if(client_socket) 
  {
	  closesocket(client_socket);
	  client_socket = NULL;
	  WSACleanup();
	  Sleep(300);
  }
  
  #define STM_PORT 30000

  if((WSAStartup(MAKEWORD(1,1), &WSAData)) != 0 ) 
  { 
	  printf("\r Failure initial WSAStartupОШИБКА ! ! ! \n"); 
      return 0;
  }
    
  if((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
	  printf("\r Failure socket create \n");  
      return 0;
  }

  client_addres.sin_family      = AF_INET;
  client_addres.sin_addr.s_addr = inet_addr(STM_ADR); 
  client_addres.sin_port        = htons(STM_PORT); 

  if((connect(client_socket,(const struct sockaddr*)&client_addres,sizeof(struct sockaddr_in))) != 0 )
  {
	  printf("\r Failure tcp connect \n"); 
	  return 0;
  }

  return 1;
}


void SendTcpMessage(unsigned int msg_id, void* data, unsigned int len)
{
	tx_tcp_msg.msg_id   = msg_id;
	tx_tcp_msg.msg_len  = len;
	if(data)memcpy(&tx_tcp_msg.data[0],data,len);
	tx_tcp_msg.msg_crc  = crc32(&tx_tcp_msg.msg_id,tx_tcp_msg.msg_len + TCP_HEADER_SIZE-4,sizeof(tx_tcp_msg));
    sendto(client_socket,(char*)&tx_tcp_msg,TCP_HEADER_SIZE + tx_tcp_msg.msg_len,0,(PSOCKADDR)&client_addres,sizeof(client_addres));
}


int RecvMessage(unsigned int msg_id, void* data, unsigned int len)
{

	int status = 0,time_out = 10, result;

	fd_set  r;
	timeval t;

	while(time_out-- > 0)
    {

       FD_ZERO(&r);

       t.tv_sec  = 0; 
       t.tv_usec = 1000000; // 100 микро секунды

       FD_SET(client_socket,&r);

	   if(SOCKET_ERROR == (result = select (client_socket, &r, NULL, NULL, &t)))
	   {
		   return status;
	   }
	   else if((result != 0) && (FD_ISSET(client_socket,&r)))
	   {
		   break;
	   }
	   else return status;
 
	}
 
	if(recv(client_socket,(char*)&rx_tcp_msg,sizeof(rx_tcp_msg),1000) <= 0) return status;

	if(crc32(&rx_tcp_msg.msg_id,rx_tcp_msg.msg_len + TCP_HEADER_SIZE - 4,sizeof(tx_tcp_msg)) == rx_tcp_msg.msg_crc)
	{
		if(rx_tcp_msg.msg_id == msg_id)
		{
			status = 1;

			if(data)
			{
				memcpy(data,&rx_tcp_msg.data[0],len);
			}
				
		}
			else printf("\r Failure msg_id \n"); 
	}
		else printf("\r Failure msg_crc \n"); 
 
  return status;
}



int RecvMessage(void)
{

	if(recv(client_socket,(char*)&rx_tcp_msg,sizeof(rx_tcp_msg),0) > 0)
	{
		if(crc32(&rx_tcp_msg.msg_id,rx_tcp_msg.msg_len + TCP_HEADER_SIZE - 4,sizeof(rx_tcp_msg)) == rx_tcp_msg.msg_crc)
		{
			return rx_tcp_msg.msg_id;
	
		}
	}
	else printf("\r Failure msg_crc !!!! \n"); 
 
  return 0;
}




void CMD_GetEthParam(void)
{
	ethernet_initial_struct* pis;
 	
	if(!TcpConnect()) return;
    else printf("\n get net param");

	SendTcpMessage(GET_ETH_PARAM,NULL,0);

 	if(recv(client_socket,(char*)&rx_tcp_msg,sizeof(rx_tcp_msg),0) > 0)
	{
		if(crc32(&rx_tcp_msg.msg_id,rx_tcp_msg.msg_len + TCP_HEADER_SIZE - 4,sizeof(tx_tcp_msg)) == rx_tcp_msg.msg_crc)
		{
			if(rx_tcp_msg.msg_id == GET_ETH_PARAM+100)
			{
				pis = (ethernet_initial_struct*)&rx_tcp_msg.data[0];
 
				printf("\r mac_adr:      %d %d %d %d %d %d \n",pis->MAC_ADR[0],pis->MAC_ADR[1],pis->MAC_ADR[2],pis->MAC_ADR[3],pis->MAC_ADR[4],pis->MAC_ADR[5]);
				printf("\r gw_adr:       %d %d %d %d \n",pis->GW_ADR[0],pis->GW_ADR[1],pis->GW_ADR[2],pis->GW_ADR[3]);
				printf("\r ip_adr:       %d %d %d %d \n",pis->IP_ADR[0],pis->IP_ADR[1],pis->IP_ADR[2],pis->IP_ADR[3]);
				printf("\r mask:         %d %d %d %d \n",pis->MASK[0],pis->MASK[1],pis->MASK[2],pis->MASK[3]);
				printf("\r udp_rx_port:  %d \n",pis->UDP_RX_PORT);
				printf("\r tcp_rx_port:  %d \n",pis->TCP_RX_PORT);
			}
			else printf("\r Failure msg_id \n"); 

		}
		else printf("\r Failure msg_crc \n");  

	} else printf("\r Failure recv \n");  
}


void CMD_SetEthParam(int mode)
{
 	if(!TcpConnect()) return;

	unsigned char eth_data[7];

	printf("\n set net param");

 	switch(mode)
	{
	  case SET_MAC:

       eth_data[0] = mode;
	   eth_data[1] = param1;
	   eth_data[2] = param2;
	   eth_data[3] = param3;
	   eth_data[4] = param4;
	   eth_data[5] = param5;
	   eth_data[6] = param6;
	   SendTcpMessage(SET_ETH_PARAM,eth_data,7);

      break;

	  case SET_GW:
	  case SET_IP:
	  case SET_MASK:

       eth_data[0] = mode;
  	   eth_data[1] = param1;
	   eth_data[2] = param2;
	   eth_data[3] = param3;
	   eth_data[4] = param4;
	   SendTcpMessage(SET_ETH_PARAM,eth_data,5);

	  break;
	  

	  case SET_TCP_RX_PORT:
	  case SET_UDP_RX_PORT:
      
	  eth_data[0] = mode;
	  *((unsigned int*)&eth_data[1]) = param1;
	  SendTcpMessage(SET_ETH_PARAM,eth_data,5);

	  break;

	}
}



int CMD_CheckConnect(void)
{
	int status = 0;

 	if(!TcpConnect()) return status;

	printf("\n check connect");

	SendTcpMessage(CHECK_CONNECT,NULL,0);

 	if(recv(client_socket,(char*)&rx_tcp_msg,sizeof(rx_tcp_msg),0) > 0)
	{
		if(crc32(&rx_tcp_msg.msg_id,rx_tcp_msg.msg_len + TCP_HEADER_SIZE - 4,TCP_MESSAGE_SIZE) == rx_tcp_msg.msg_crc)
		{
			if(rx_tcp_msg.msg_id == CHECK_CONNECT+100)
			{
     	    	printf(" DONE \n"); 
				status = 1;
			}
			else printf("\r FAILURE \n"); 
		}
		else printf("\r FAILURE \n"); 

	} else printf("\r FAILURE \n");  

	return status;
}


int CMD_GetCrc(int mode)
{

	unsigned int total_crc = -1;
	unsigned int total_az = 0;
	unsigned int stm_crc;
    unsigned int f_data_size;

	CMD_Stop();
	
	OpenDebugFiles(1,"output_debug_pc_crc.txt");
	OpenDebugFiles(2,"output_debug_stm_crc.txt");

	CFile file;

	if(mode == OPEN)
	{
		if(!TcpConnect()) return 0;
	}

	
	if(!file.Open("sample.dat",CFile::modeRead))
	{
		printf("\n FAILURE open file \n");
        return 0;
	}


 	int fsize = file.GetLength();

	file.Read(&initial_data,sizeof(initial_data));

	fsize -= sizeof(initial_data);

	f_data_size = (initial_data.NS * 2) + 4;


	while(fsize > 0)
	{
		memset(&f_data,0,sizeof(f_data));
		file.Read(&f_data,f_data_size);
		total_crc = crc32(total_crc,&f_data,f_data_size,f_data_size);
		fsize -= f_data_size;
	
		fprintf(pf[FOUT1],"\n AZ = %d   EL = %d   crc = 0x%x",f_data.AZ,f_data.EL,total_crc);

		total_az++;
	}

			
	fclose(pf[FOUT1]);

	printf("\n total_az = %d NS = %d",total_az,initial_data.NS); 

 	if(!TcpConnect()) return 0;

	SendTcpMessage(GET_CRC,&total_az,sizeof(total_az));

	printf("\n calculate CRC ");


while(1)
{
		
	switch(RecvMessage())
	{
	    case 0:
			printf("\n");
			printf("\n RecvMessage FAILURE \n");
		return 0;

		case GET_CRC + 100:

			memcpy(&stm_crc,&rx_tcp_msg.data[0],4);
			
			printf("\r calculate CRC <completed 100%%>");

			if(total_crc == stm_crc) 
			{   printf("\n"); 
				printf("\n crc OK    [ %x %x ] \n",total_crc,stm_crc);
			}
            else 
			{	printf("\n"); 
				printf("\n crc FAILURE    [ %x %x ] \n",total_crc,stm_crc);
			}

			

		fclose(pf[FOUT2]);

        return 1;

		case GET_CRC + 200:
 	
			printf("\r calculate CRC <completed %d%%>",*((unsigned int*)&rx_tcp_msg.data[0]) );

		break;

				
	}
  	
}


fclose(pf[FOUT2]);

return 0;

}


#define PD3


void CMD_CanHandler(void)
{

   struct info_msg_struct
   {
	   unsigned int can_AZ;
	   unsigned int DAZ;
	   float AZ_f;
	   float VAZ;

	   unsigned int can_EL;
	   unsigned int DEL;
	   float EL_f;
	   float VEL;
	   

   
   } ims;
	

#ifdef PD3

   int tim = 0;

   unsigned int can_AZ_old = 0, err = 0;

#endif
   
   if(!UdpInitial()) return;

   while(!kbhit())
   {


#ifdef PD1
  if(recv(client_socket,(char*)&ims,sizeof(ims),0) > 0)
  {
	  printf("\r DAZ = %d   AZ = %d / %4.3f   VAZ = %4.2f \n",ims.DAZ,ims.can_AZ,ims.AZ_f,ims.VAZ,ims);
  }
#endif
#ifdef PD3
  if(recv(client_socket,(char*)&ims,sizeof(ims),0) > 0)
  {
//	  fprintf(pf,"\r IAZ = %d  IEL = %d \n",ims.can_AZ,ims.can_EL);

	  if(can_AZ_old != ims.can_AZ)
	  {
		  err++;
//		  fprintf(pf,"\r ******************* ERROR ******************* ");

	  }
	  
	  can_AZ_old = (ims.can_AZ + 1) & 0x3fff;

	  if(tim++ > 20)
	  {
		  printf("\r IAZ %d IEL %d PAGE_ADR = %d \n",ims.can_AZ,ims.can_EL,ims.DAZ);
		  tim = 0;
	  }
  }
#endif

  
   }

   getch();

//   fclose(pf);
}

void MrlsImitator(void)
{

	CFile file;
	char asci[6];
	unsigned char msg[6+12+8];

	unsigned short AZ;
    unsigned short EL;
		
	float AZ_f;
    float EL_f;

	memset(asci,0,sizeof(asci));

	sprintf((char*)msg,"0016RCV02");

    if(!UdpInitial()) return;

	
	if(!file.Open("dump.dump",CFile::modeRead))
	{
		printf("\r FAILURE open file \n");
		return;
	}

	int fsize = file.GetLength();
	
	int f = 0;

	while(fsize > 0)
	{
		for(int i = 0 ; i <= 4; i++) asci[i] = asci[i+1];
	
    	file.Read(&asci[5],1);

		fsize--;
	
		if((strcmp(asci,"RCV02")) != 0) continue; 

		file.Read(&msg[10],20);

		fsize-=20;

		if(msg[16] != 0x80) continue;

		AZ = 0;
		AZ = msg[18] << 7;
		AZ = AZ | msg[17];

		EL = 0;
		EL = msg[20] << 7;
		EL = EL | msg[19];
		
		AZ_f  = (float)AZ * 360.0 / 16384.0;
        EL_f  = (float)EL * 360.0 / 16384.0;



		initial_data.DAZ = (float)0.9;
		initial_data.DEL = (float)0.9;


		int IAZ = AZ_f / initial_data.DAZ;
        int IEL = EL_f / initial_data.DEL;
		int page_per_az = 5;


	    printf("\r %f %f      %d %d \n",AZ_f,EL_f,IAZ,IEL);

		int page_adr = (IEL * initial_data.NAZ * page_per_az) + (IAZ * page_per_az) + 0;

		fprintf(pf[FOUT0],"\n AZ = %d   EL = %d   AZ_f = %f   EL_f = %f   IAZ = %d   IEL = %d  page_adr = %d",AZ,EL,AZ_f,EL_f,IAZ,IEL,page_adr);

		if(AZ_f >= 268 && AZ_f <= 270)
		{
			Sleep(0);
		}
      
		sendto(client_socket,(char*)&msg,sizeof(msg),0,(PSOCKADDR)&client_addres,sizeof(client_addres));


		Sleep(5);

	}
	
	   printf("\n DONE \n");

}


int GetTime(void)
{
	static int time[3] = {0,0,0}; 
  
	int dt = 0;
  
	if (time[1] == 0)
	{
		time[1] = time[2] = timeGetTime();
	}
    else
	{
		time[0] = timeGetTime();
        
		dt = time[0] - time[2];
        
		if(dt > 50)
		{
			time[2] = time[0];
		}
   
		else dt = 0;
  }
 return dt;
}


void CMD_Start(void)
{
	if(TcpConnect() == 0) 	return;

	SendTcpMessage(START,NULL,0);
}


void CMD_Stop(void)
{
	if(TcpConnect() == 0) 	return;

	SendTcpMessage(STOP,NULL,0);
}


void CMD_SendDataMode1(void)
{

	CFile file;

	OpenDebugFiles(FOUT0,"output_debug_adr.txt");
	
	if(!file.Open(fname,CFile::modeRead))
	{
		printf("\r FAILURE open file \n");
        return;
	}

 	if(TcpConnect() == 0) 	return;

 	int fsize = file.GetLength();

	tx_tcp_msg.data[0] = 0;

	printf("\n");

	while(1)
	{
		if((fsize - 256) > 0)
		{				
			file.Read(&tx_tcp_msg.data[1],256);	
			SendTcpMessage(SEND_DATA_MODE_1,NULL,256+1);

			tx_tcp_msg.data[0]++;
			fsize-=256;
			printf("\r %d send %d \n",tx_tcp_msg.data[0]-1, tx_tcp_msg.data[0] * 256);
			Sleep(1);

		}
		else
		{	file.Read(&tx_tcp_msg.data[1],fsize);
			SendTcpMessage(SEND_DATA_MODE_1,NULL,fsize+1);
			printf("\r %d send %d \n",tx_tcp_msg.data[0]-1,tx_tcp_msg.data[0] * 256 + fsize);
			printf("\r done \n");
			break;

		}
	}

    SendTcpMessage(SET_MODE_1,NULL,0);

	file.Close();
}


#define HANDLER_16_BIT


void CMD_SendDataMode2(void)
{


	unsigned int f_data_size;

	CMD_Stop();
		
	OpenDebugFiles(0,"output_debug_adr.txt");


	CFile file;
	
	if(!file.Open("sample.dat",CFile::modeRead))
	{
		printf("\n FAILURE open file \n");
        return;
	}


 	if(TcpConnect() == 0) return;


 	int fsize = file.GetLength();

	file.Read(&initial_data,sizeof(initial_data));

	fsize -= sizeof(initial_data);

	fprintf(pf[FOUT0],"\n DAZ = %f \n DEL = %f \n NEL = %d \n NAZ = %d \n NS = %d ",initial_data.DAZ,initial_data.DEL,initial_data.NEL,initial_data.NAZ,initial_data.NS);

	f_data_size = (initial_data.NS * 2) + 4;  // ( количество АЦП выборок NS ) * ( 2 байта на выборку ) + ( 4 байта заголовок )

#ifdef HANDLER_8_BIT

	unsigned char *pdata;
	
	int page_per_az = initial_data.NS / 1024;

	if((page_per_az * 1024) - initial_data.NS < 0) 
	{
		page_per_az += 1;
	}

	int total_page = (initial_data.NAZ * page_per_az) * initial_data.NEL;

	fprintf(fp,"\n page_per_az = %d  \n total_page = %d \n ",page_per_az,total_page);
  
	printf("\n");
 
#endif

	SendTcpMessage(SET_INITIAL_DATA,&initial_data,sizeof(initial_data));

	int status = 1;

	while(status)
	{
		switch(RecvMessage())
		{
	       case 0:
		   	      printf("\n");
			      printf("\n RecvMessage FAILURE \n");
		          status = 0;
		   break;

		   case SET_INITIAL_DATA + 100:
		
		     	  printf("\r nand erase <completed 100%%>");
			      status = 0;
           break;

   		   case SET_INITIAL_DATA + 200:
 	
		   	      printf("\r nand erase <completed %d%%>",*((unsigned int*)&rx_tcp_msg.data[0])*10);
		
		   break;
			
		}
	}

	printf("\n");

	Sleep(1000);

#ifdef HANDLER_8_BIT

	while(fsize > 0)
	{	
		file.Read(&f_data,f_data_size);
	
		printf("\r EL = %d AZ = %d \n",f_data.EL,f_data.AZ);
	
		fsize -= f_data_size;

		pmsg = (mode2_msg_data_struct*)&tx_tcp_msg.data[0];
	
		pdata = (unsigned char*)&f_data.data[0];


		for(int i = 0; i < page_per_az; i++)
		{
			pmsg->AZ = f_data.AZ;
            pmsg->EL = f_data.EL;
            pmsg->index_az = i;

			memcpy((unsigned char*)&pmsg->data[0],pdata,2048);
		    SendTcpMessage(SEND_DATA_MODE_2,NULL,2048+6);
			pdata+=2048;

			WriteNandPage(pmsg,tx_tcp_msg.msg_len);

			pdata+=2048;

		}
	}

#endif

#ifdef HANDLER_16_BIT

	while(fsize > 0)
	{	
		memset(&f_data,0,sizeof(&f_data));

		file.Read(&f_data,f_data_size);
	
		printf("\r EL = %d AZ = %d \n",f_data.EL,f_data.AZ);
	
		fsize -= f_data_size;

	//	WriteNandPage();

        SendTcpMessage(SEND_DATA_MODE_2,&f_data,sizeof(f_data));
	}


#endif

//	TcpDisconnect();

	fclose(pf[FOUT0]);

	file.Close();

	Sleep(1000);

	CMD_GetCrc(NOPEN);
}


void create_pila_file(void)
{
	CFile file;
	
	if(!file.Open("pila.dat",CFile::modeCreate | CFile::modeWrite))
	{
		printf("\n FAILURE open file \n");
        return;
	}

	for(unsigned short i = 0; i < 8192; i++) 
	{
		file.Write(&i,2);
	}

	file.Close();

}

#ifdef HANDLER_8_BIT

void WriteNandPage(mode2_msg_data_struct *pmsg, int len)
{
//	int page_adr = (pmsg->EL * initial_data.NAZ * page_per_az) + (pmsg->AZ * page_per_az) + pmsg->index_az;
 //	if(fp) fprintf(fp,"\n AZ = %d   EL = %d   Index = %d  page_adr = %d ",pmsg->AZ,pmsg->EL,pmsg->index_az,page_adr);
}

#endif


#ifdef HANDLER_16_BIT

void WriteNandPage(void)
{

 	int page_adr = (f_data.EL * initial_data.NAZ) + f_data.AZ;

 	if(pf[FOUT0]) fprintf(pf[FOUT0],"\n AZ = %d   EL = %d  page_adr = %d ",f_data.AZ,f_data.EL,page_adr);
}

#endif


void CMD_SetDac(void)
{
 	if(TcpConnect() == 0) return;
	else
	{
		printf("\n set data dac \n %d",param1);
    	SendTcpMessage(SET_DATA_DAC,&param1,2);
	}
}

void CMD_SetFreq(void)
{

 	if(TcpConnect() == 0) return;
	else
	{
		printf("\n set frequency dac \n %d",param1);
    	SendTcpMessage(SET_FREQ_DAC,&param1,2);
	}
}


 

unsigned int crc32(void * pcBlock, unsigned short len, unsigned short tot_len)
{
	if(len > tot_len) return 0;


	unsigned char *p = (unsigned char*)pcBlock;
	
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

    unsigned int crc = -1;
    while (len--)
        crc = (crc >> 8) ^ Crc32Table[(crc ^ *p++) & 0xFF];
    return crc ^ 0xFFFFFFFF;
}


unsigned int crc32(unsigned int crc, void * pcBlock, unsigned short len, unsigned short tot_len)
{
	if(len > tot_len) return 0;

	unsigned char *p = (unsigned char*)pcBlock;
	
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
//    return crc ^ 0xFFFFFFFF;
	return crc;
}
