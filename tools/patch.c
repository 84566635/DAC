#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define RET() {printf("Error %d\n", __LINE__);return __LINE__;}
int main(int argc, char *argv[])
{
  if(argc < 4)
    RET();
  int tableIdx = 0;
  if(argc >= 5)
    tableIdx = strtol(argv[4], NULL, 0);
  int symbol[1000] = {0};

  FILE *f;
  f = fopen(argv[1], "r");
  if(f == NULL)
    RET();

  char line[256];
  int param = 0;
  while(NULL != fgets(line, 254, f))
    {
      char *ptr = line;
      int i = tableIdx+1;
      while(i-- && ptr)
	{
	  ptr = strchr(ptr, ';');
	  if(ptr) ptr++;
	}
      if(ptr)
	{
	  if(sscanf(ptr, "\t0x%x", &symbol[param]) != 1)
	    if(sscanf(ptr, "\t%d", &symbol[param]) != 1)
	      continue;
	}
      else
	{
	  continue;
	}
      param++;
    }

  fclose(f);
  if(param == 0)
    RET();

  f = fopen(argv[2], "r");
  if(f == NULL)
    RET();
  char *buffer = calloc(1, 10*1024*1024);
  if(buffer == NULL)
    {
      fclose(f);
      RET();
    }

  int bufSize  = fread(buffer, 1, 10*1024*1024, f);
  fclose(f);
  if(bufSize == 0)
    {
      free(buffer);
      RET();
    }

  int idx;
  int replaced = 0;
  for(idx = 0; idx < bufSize - 3; idx++)
    {
      int *sptr = (int*)&buffer[idx];
      if((sptr[0] & 0xFFFFFF00) == 0xABCDEF00)
        {
          int p = sptr[0]&0xFF;
          if(p >= param)
            printf("Symbol %d not defined in %s \n", p, argv[1]);
          else
            {
              sptr[0] = symbol[p];
              replaced++;
            }
        }
    }

  f = fopen(argv[3], "w+");
  if(f == NULL)
    {
      free(buffer);
      RET();
    }

  fwrite(buffer, 1, bufSize, f);
  fclose(f);
  free(buffer);
  printf(" replaced occurences %d of %d symbols (%s:%s->%s) \n", replaced, param, argv[1], argv[2], argv[3]);
  return 0;
}
