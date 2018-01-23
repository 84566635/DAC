
typedef void (*action_t) (void *data);

typedef struct
{
  ///////////////////////////////////////////////////////
  //The module data that must be supported for the module
  int queueLength;              // Module message queue length
  int timeout;                  // Queue reception timeout
  int commandsNum;              //Number of command types (number of command callbacks)
  action_t initCallback;        //Init function called before any main loop starts
  const action_t *commandCallback;      //Action that will be taken when command comes
  action_t timeoutCallback;     //Action that will be taken only when timeout occures
  action_t alwaysCallback;      //Action thta will be taken regardles of event (command or timeout)

  //Task data
  int priority;                 //priority of the task
  int stackSize;                //size of the stack of the task in bytes
  void *stack;
  char *name;                   //Name of the task (module)

  ///////////////////////////////////////////////////////
  //The internal module data.
  void *queueHandle;
  void *privateData;
  int enqueues;
  int fenqueues;
  int dequeues;
} moduleModule_t;

// Send a message to the module
int moduleSendCommand(moduleModule_t *moduleTaskData, int command, void *data);

// Module initialize
void moduleInit(moduleModule_t *moduleTaskData);
