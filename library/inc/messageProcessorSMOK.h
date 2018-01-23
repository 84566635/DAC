
int sendStatus(int MsgCode, int val1, int val2);
int sendStatusLong(int MsgCode, int val1, int val2, int val3, int val4);
void kaCCEnable(int enable);
int pll_set_retry(int audioOutputType, int force, int retryNUm);
int pll_set(int audioOutputType, int force);
extern const char masterMap[2][2][2];
extern int amIStereo;

void sharcTransfer(uint8_t *txBuffer, uint8_t *rxBuffer, int size);
