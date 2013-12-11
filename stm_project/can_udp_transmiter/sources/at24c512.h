
uint32_t AT24_Config(void);
uint32_t AT45_PageWrite(uint16_t Addr, uint8_t *pdata);
uint32_t AT45_Write(uint16_t Addr, uint8_t *pdata, int size);
uint32_t AT45_Read(uint16_t Addr, uint8_t *pdata, int size);

