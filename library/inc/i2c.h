int i2cInit(I2C_TypeDef *i2c);
int i2cWrite(I2C_TypeDef *i2c, uint8_t addr, uint8_t *data, uint16_t size);
int i2cRead(I2C_TypeDef *i2c, uint8_t addr, uint8_t *data, uint16_t size);
int i2cTransfer(I2C_TypeDef *i2c, uint8_t addr, uint8_t *txData, uint16_t txSize, uint8_t *rxData, uint16_t rxSize);
