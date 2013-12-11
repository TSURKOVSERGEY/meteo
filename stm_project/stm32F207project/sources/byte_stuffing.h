
#define  DLE          0x0010
#define  STX          0x0002
#define  ETX          0x0003
#define  DLE_DLE      0x00100010
#define  DLE_STX      0x00100002 
#define  DLE_ETX      0x00100003 
#define  HEADER_SIZE  0x5           /* DLE_STX_CMD_DLE_ETX */

int StuffingWriteCmd(unsigned short cmd);
int StuffingWriteData(unsigned short cmd, unsigned short data);
int StuffingWriteMessage(unsigned short cmd, unsigned short *pBin, int size);
int StuffingReadMessage(unsigned short);
  
  
   

