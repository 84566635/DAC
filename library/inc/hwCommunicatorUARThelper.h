
#define USART_HANDLER(arg_port, arg_usart) static void arg_usart##_IRQHandler(void) \
  {                                 \
    rxTxComplete_e CENdir = 0;                      \
    if (USART_GetITStatus(arg_usart, USART_IT_TXE) != RESET)        \
    {                                 \
      CENdir |= RXTX_COMPLETE_TX;                 \
      \
      /*Send next data form Round Robing buffer */            \
      int data = rrGet(&(((usartHelper_t *) portData[arg_port].UARThelper)->rrTx)); \
      \
      if (data >= 0)				      \
        USART_SendData(arg_usart, data);              \
      else                                \
        USART_ITConfig(arg_usart, USART_IT_TXE, DISABLE);     \
    }                                 \
    while (USART_GetITStatus(arg_usart, USART_IT_RXNE) != RESET)	\
    {                                 \
      if(portData[arg_port].UARThelper != NULL)				\
	{								\
	  CENdir |= RXTX_COMPLETE_RX;					\
	  rrPut(&(((usartHelper_t *) portData[arg_port].UARThelper)->rrRx), USART_ReceiveData(arg_usart)); \
	}								\
      else								\
	{								\
	  (void)USART_ReceiveData(arg_usart);				\
	}								\
      /* USART_ClearITPendingBit(arg_usart, USART_IT_RXNE); */		\
    }									\
    if (CENdir)								\
      if(portDataTxRxComplete(arg_port, PORT_TYPE_USART, CENdir) < 0);	\
  }


#define USART_INIT_HANDLER(arg_usart)               \
  {                             \
    registerIRQ(arg_usart##_IRQn, arg_usart##_IRQHandler);  \
    NVIC_SetPriority(arg_usart##_IRQn, configMAX_SYSCALL_INTERRUPT_PRIORITY);           \
    NVIC_EnableIRQ(arg_usart##_IRQn);               \
    USART_ITConfig(arg_usart, USART_IT_RXNE, ENABLE);            \
  }

int USARTreceiveData(portNum_t portNum, char *buffer, int maxSize);
int USARTsendData(portNum_t portNum, char *buffer, int size);

typedef struct
{
  roundRobin_t rrTx;
  roundRobin_t rrRx;
  void *usart;
} usartHelper_t;
