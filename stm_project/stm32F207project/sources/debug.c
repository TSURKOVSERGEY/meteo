#include "debug.h"
#include "main.h"
#include "nand_hw_driver.h"

char dump_buffer[1024];

int index = 0;

void myprint(char* pb)
{
  if(index == 1023) reset_index();
  
  while(*pb != 0)
  {
    dump_buffer[index++] = *(pb++);
  }

}

void reset_index(void)
{
  index = 0;
}


 
void debug(void)
{
  
  uint16_t i,j,err = 0;
  
  int max_page = 64 * 100;
   
  for(j = 0; j < max_page; j++)
  {
    nand_command_wr(1,j);
    //for(k = 0; k < 0xfff; k++);

    i = nand_command_rd(1);

    delay(0);
    
    if(i != j) err++;
  }
  
  delay(0);
  
}
 