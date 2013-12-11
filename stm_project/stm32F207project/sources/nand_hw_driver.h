#include "stm32f2xx.h"

void nand_command_wr(uint32_t id, uint16_t data);
uint16_t nand_command_rd(uint32_t id);

void nand_adr_wr(uint32_t id, uint16_t data);
uint16_t nand_data_rd(uint32_t id);
void nand_page_adr_wr(uint32_t id, unsigned long int page);
void nand_data_wr(uint32_t id, uint16_t data);
void nand_rst(uint32_t id);
void nand_rdy(uint32_t id);
uint8_t nand_erase_block(uint32_t id, unsigned long int page);

uint8_t nand_8bit_write_page(uint32_t id, uint8_t *page_bufer, unsigned long int page);
void nand_16bit_write_page(uint32_t id, uint16_t *page_bufer, unsigned long int page);

uint8_t nand_8bit_read_page(uint32_t id, uint8_t *page_bufer,unsigned long int page);
void nand_16bit_read_page(uint32_t id, uint16_t *page_bufer,unsigned long int page);

void nand_8bit_read_page_info(uint32_t id, uint8_t *page_bufer,unsigned long int page);
void nand_16bit_read_page_info(uint32_t id, uint16_t *page_bufer,unsigned long int page);

void nand_ecc_enable(uint32_t id);
void NAND_Config(void);
uint8_t nand_read_status(uint32_t id);