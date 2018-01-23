
#ifndef DEFAULT_HANDLER
# define DEFAULT_HANDLER(arg_name) void __attribute__ ((no_instrument_function, interrupt)) arg_name(void)
#endif

void __attribute__ ((no_instrument_function)) Reset_Handler(void);

DEFAULT_HANDLER(NMI_Handler);
DEFAULT_HANDLER(HardFault_Handler);
DEFAULT_HANDLER(MemManage_Handler);
DEFAULT_HANDLER(BusFault_Handler);
DEFAULT_HANDLER(UsageFault_Handler);
DEFAULT_HANDLER(vPortSVCHandler);
DEFAULT_HANDLER(DebugMon_Handler);
DEFAULT_HANDLER(xPortPendSVHandler);
DEFAULT_HANDLER(sysTickHandler);

int main(void);
int registerIRQ(int irq, void (*handler) (void));
int amIInIRQ(void);
