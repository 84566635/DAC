int messageRx(portNum_t portNum, char *buffer, int bufferSize, uint8_t gen);
int messageTxComplete(portNum_t portNum, int status, uint32_t handle);
int messageADCMeasure(int samplesNum, int sampleSensADC,
                      int V1accu, int V2accu,
                      int I1max, int V2I1max, int I1accu, int I2accu);
int messageCommNULL(void);
void CommunicatorInit(void);

#define COMM_TX_FAILED  0
#define COMM_TX_SUCCESS 1


//Functions to be imlemented in the project
void communicatorRx(portNum_t portNum, char *buffer, int bufferSize, uint8_t gen);
void communicatorTxComplete(portNum_t portNum, int status, uint32_t handle);

void communicatorADCMeasure(int samplesNum,int sampleSensADC,
                            int V1accu, int V2accu,
                            int I1max, int V2I1max, int I1accu, int I2accu);

void communicatorPeriodic(void);
