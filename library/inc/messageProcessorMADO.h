
int __attribute__((weak))testInputPatternChange(int pattern);

#define SELECTOR_WM8805 0
#define SELECTOR_SAI    1
void selectorSet(int value);
void i2sSelectSource(int wmNum, int input);
void sendKA(portNum_t sendPort);
int sendOnOff(portNum_t portNum, int on);
void showSpkSets(int *bufSize, char *bufPtr);
void updateAllSMOKs(void);

