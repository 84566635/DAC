void debugInit(void);
#define DPRINTF(...) { if(*bufSize < DBUFSIZE-256){xsprintf(&bufPtr[*bufSize], __VA_ARGS__);*bufSize += strlen(&bufPtr[*bufSize]);}}
