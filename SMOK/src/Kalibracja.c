#include "common.h" /* massert */
#include <piny_ADC_FPGA.h>

void Kalibracja_init(void);
void Kalibracja_(void);

static void uart6_send(uint8_t tx_data);
static uint8_t SPI_ADC_Wr_Rd(uint8_t Data);
static uint8_t SPI_FPGA_L_Wr_Rd(uint8_t Data);
static uint8_t SPI_FPGA_R_Wr_Rd(uint8_t Data);
static int32_t get_SPI_ADC(uint8_t Channels);
static void set_FPGA(uint64_t L, uint64_t R);
void delay_us(uint32_t delay);
void delay_100ns(uint32_t delay);
static uint32_t CalcCRC32(uint32_t crc, uint8_t *buffer, uint8_t length);
static uint64_t R_value_compensate(int32_t data);
static uint64_t I_compensate(uint64_t data);


union uint32_t_LSB_MSB
    {
    struct
        {
        uint8_t XSB_0;
        uint8_t XSB_1;
        uint8_t XSB_2;
        uint8_t XSB_3;
        };

        uint32_t XSB;
    };

union int32_t_LSB_MSB
    {
    struct
        {
        uint8_t XSB_0;
        uint8_t XSB_1;
        uint8_t XSB_2;
        uint8_t XSB_3;
        };

        int32_t XSB;
    };

#define RA_ADDR (0ULL << 34)
#define RB_ADDR (1ULL << 34)
#define RC_ADDR (2ULL << 34)

#define RA_MUX_ADDR (3ULL << 34)
#define RB_MUX_ADDR (4ULL << 34)
#define RC_MUX_ADDR (5ULL << 34)

#define I2S_Format_ADDR (6ULL << 34)

#define RA_x_start_ADDR 64ULL
#define RB_x_start_ADDR 128ULL
#define RC_x_start_ADDR 192ULL



volatile uint8_t Rx2_buffer[64];
volatile uint8_t Rx2_buffer_size = 0;

volatile uint8_t Tx2_buffer[256];
volatile uint8_t Tx2_WR = 0;
volatile uint8_t Tx2_RD = 0;


volatile uint8_t Tx4_buffer[256];
volatile uint8_t Tx4_WR = 0;
volatile uint8_t Tx4_RD = 0;


volatile uint8_t Rx6_buffer[64];
volatile uint8_t Rx6_buffer_size = 0;

volatile uint8_t Tx6_buffer[64];


union uint32_t_LSB_MSB CRC_;

union int32_t_LSB_MSB ADC_0;
union int32_t_LSB_MSB ADC_1;
union int32_t_LSB_MSB ADC_2;
union int32_t_LSB_MSB ADC_3;
union int32_t_LSB_MSB ADC_4;
union int32_t_LSB_MSB ADC_5;
union int32_t_LSB_MSB ADC_6;
union int32_t_LSB_MSB ADC_7;

volatile uint64_t DAC_L_value;
volatile uint64_t DAC_R_value;
volatile uint64_t DAC_L_value_to_send;
volatile uint64_t DAC_R_value_to_send;


union int32_t_LSB_MSB RR[34][8] __attribute__((section (".sram2_data")));
union int32_t_LSB_MSB RL[34][8] __attribute__((section (".sram2_data")));
union int32_t_LSB_MSB VP0[34][8] __attribute__((section (".sram2_data")));
union int32_t_LSB_MSB VP1[34][8] __attribute__((section (".sram2_data")));
union int32_t_LSB_MSB VN[34][8] __attribute__((section (".sram2_data")));

volatile uint8_t Sine_generate;

volatile uint8_t R_MUX;


volatile uint64_t Calibration_state[34][8] __attribute__((section (".sram2_data")));

volatile uint8_t Licznik_kalibracji = 0;


//-------------------------------------------------------------------------
void Kalibracja_init(void)
{
	Calibration_state[27][0] = 0b0000000000000000000000000000000000ULL;
	Calibration_state[27][1] = 0b1000000000000000000000000000000000ULL;
	Calibration_state[27][2] = 0b1100000000000000000000000000000000ULL;
	Calibration_state[27][3] = 0b1110000000000000000000000000000000ULL;
	Calibration_state[27][4] = 0b1111000000000000000000000000000000ULL;
	Calibration_state[27][5] = 0b1111100000000000000000000000000000ULL;
	Calibration_state[27][6] = 0b1111110000000000000000000000000000ULL;
	Calibration_state[27][7] = 0b1111110110000000000000000000000000ULL;

	Calibration_state[28][0] = 0b0000000000000000000000000000000000ULL;
	Calibration_state[28][1] = 0b1000000000000000000000000000000000ULL;
	Calibration_state[28][2] = 0b1100000000000000000000000000000000ULL;
	Calibration_state[28][3] = 0b1110000000000000000000000000000000ULL;
	Calibration_state[28][4] = 0b1111000000000000000000000000000000ULL;
	Calibration_state[28][5] = 0b1111100000000000000000000000000000ULL;
	Calibration_state[28][6] = 0b1111101000000000000000000000000000ULL;
	Calibration_state[28][7] = 0b1111101110000000000000000000000000ULL;

	Calibration_state[29][0] = 0b0000000000000000000000000000000000ULL;
	Calibration_state[29][1] = 0b1000000000000000000000000000000000ULL;
	Calibration_state[29][2] = 0b1100000000000000000000000000000000ULL;
	Calibration_state[29][3] = 0b1110000000000000000000000000000000ULL;
	Calibration_state[29][4] = 0b1111000000000000000000000000000000ULL;
	Calibration_state[29][5] = 0b1111010000000000000000000000000000ULL;
	Calibration_state[29][6] = 0b1111010000000000000000000000000000ULL;
	Calibration_state[29][7] = 0b1111011110000000000000000000000000ULL;

	Calibration_state[30][0] = 0b0000000000000000000000000000000000ULL;
	Calibration_state[30][1] = 0b1000000000000000000000000000000000ULL;
	Calibration_state[30][2] = 0b1100000000000000000000000000000000ULL;
	Calibration_state[30][3] = 0b1110000000000000000000000000000000ULL;
	Calibration_state[30][4] = 0b1110100000000000000000000000000000ULL;
	Calibration_state[30][5] = 0b1110100000000000000000000000000000ULL;
	Calibration_state[30][6] = 0b1110110000000000000000000000000000ULL;
	Calibration_state[30][7] = 0b1110111110000000000000000000000000ULL;

	Calibration_state[31][0] = 0b0000000000000000000000000000000000ULL;
	Calibration_state[31][1] = 0b1000000000000000000000000000000000ULL;
	Calibration_state[31][2] = 0b1100000000000000000000000000000000ULL;
	Calibration_state[31][3] = 0b1101000000000000000000000000000000ULL;
	Calibration_state[31][4] = 0b1101000000000000000000000000000000ULL;
	Calibration_state[31][5] = 0b1101100000000000000000000000000000ULL;
	Calibration_state[31][6] = 0b1101110000000000000000000000000000ULL;
	Calibration_state[31][7] = 0b1101111110000000000000000000000000ULL;

	Calibration_state[32][0] = 0b0000000000000000000000000000000000ULL;
	Calibration_state[32][1] = 0b1000000000000000000000000000000000ULL;
	Calibration_state[32][2] = 0b1010000000000000000000000000000000ULL;
	Calibration_state[32][3] = 0b1010000000000000000000000000000000ULL;
	Calibration_state[32][4] = 0b1011000000000000000000000000000000ULL;
	Calibration_state[32][5] = 0b1011100000000000000000000000000000ULL;
	Calibration_state[32][6] = 0b1011110000000000000000000000000000ULL;
	Calibration_state[32][7] = 0b1011111110000000000000000000000000ULL;

	Calibration_state[33][0] = 0b0000000000000000000000000000000000ULL;
	Calibration_state[33][1] = 0b0100000000000000000000000000000000ULL;
	Calibration_state[33][2] = 0b0100000000000000000000000000000000ULL;
	Calibration_state[33][3] = 0b0110000000000000000000000000000000ULL;
	Calibration_state[33][4] = 0b0111000000000000000000000000000000ULL;
	Calibration_state[33][5] = 0b0111100000000000000000000000000000ULL;
	Calibration_state[33][6] = 0b0111110000000000000000000000000000ULL;
	Calibration_state[33][7] = 0b0111111110000000000000000000000000ULL;




	ADC_CS_L;
	SPI_ADC_Wr_Rd(0x06);    //RESET
	ADC_CS_H;

	delay_us(1000);

	ADC_CS_L;
	SPI_ADC_Wr_Rd(0x40 + 0x04);    //WREG DATARATE
	SPI_ADC_Wr_Rd(0x00);
	//SPI_ADC_Wr_Rd(0b00110000);    //Single-shot conversion mode, Low-latency filter, 2.5 SPS
	SPI_ADC_Wr_Rd(0b00110010);    //Single-shot conversion mode, Low-latency filter, 10 SPS
	ADC_CS_H;

	delay_us(1000);

	ADC_CS_L;
	SPI_ADC_Wr_Rd(0x40 + 0x05);    //WREG REF
	SPI_ADC_Wr_Rd(0x00);
	SPI_ADC_Wr_Rd(0b00111010);    //Internal reference is always on, even in power-down mode
	                              //Internal 2.5-V reference, REFN_BUF = Disabled, REFP_BUF = Disabled
	ADC_CS_H;

	delay_us(1000);


	ADC_CS_L;
	SPI_ADC_Wr_Rd(0x40 + 0x02);    //WREG INPMUX
	SPI_ADC_Wr_Rd(0x00);
	SPI_ADC_Wr_Rd(0xAC);    //MUXP = AIN5, MUXN = AINCOM
	ADC_CS_H;

	delay_us(100);
}
//-------------------------------------------------------------------------
void Kalibracja_(void)
{
    int32_t i;
    int32_t j;




#if Kalibracja == 0
    	for (j=9; j<27; j++)
    	    {
    		if (R_MUX == 0)
    		    {
    			set_FPGA(0ULL | RB_MUX_ADDR, 0ULL | RB_MUX_ADDR);
    			delay_us(1000);
    		    }

    		if (R_MUX == 1)
    		    {
    			set_FPGA(1ULL | RB_MUX_ADDR, 1ULL | RB_MUX_ADDR);
    			delay_us(1000);
    		    }

    		if (R_MUX == 2)
    		    {
    			for (i=8; i<34; i++)
    			    {
    				set_FPGA((R[i][3].XSB | ((RB_x_start_ADDR + (uint64_t)i) << 34)), (R[i][3].XSB | ((RB_x_start_ADDR + (uint64_t)i) << 34)));
    				delay_us(1000);
    			    }

    			set_FPGA(1ULL | RB_MUX_ADDR, 1ULL | RB_MUX_ADDR);
    			delay_us(1000);

    			R_MUX = 1;
    		    }

    		set_FPGA(0ULL, 0b0000000000000000000000000000000111ULL | RB_ADDR);    //Z korekcj¹ poboru pr¹du
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, 0b0000000000000000000000000000000111ULL | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][0].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][0].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][0].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][0].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, 0b1000000000000000000000000000100101ULL | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, 0b1000000000000000000000000000100101ULL | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][1].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][1].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][1].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][1].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, 0b1100000000000000000000000000000101ULL | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, 0b1100000000000000000000000000000101ULL | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][2].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][2].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][2].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][2].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, 0b1110000000000000000000000000000100ULL | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, 0b1110000000000000000000000000000100ULL | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][3].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][3].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][3].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][3].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, 0b1111000000000000000000000000010001ULL | RB_ADDR);    //Z korekcj¹ poboru pr¹du
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, 0b1111000000000000000000000000010001ULL | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][4].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][4].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][4].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][4].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, 0b1111100000000000000000000000010000ULL | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, 0b1111100000000000000000000000010000ULL | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][5].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][5].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][5].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][5].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, 0b1111110000000000000000000000000001ULL | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, 0b1111110000000000000000000000000001ULL | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][6].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][6].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][6].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][6].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, 0b1111111000000000000000000000000000ULL | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, 0b1111111000000000000000000000000000ULL | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][7].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][7].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][7].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][7].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		/*set_FPGA(0ULL, 0b0000000000000000000000000000000000ULL | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, 0b0000000000000000000000000000000000ULL | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][0].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][0].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][0].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][0].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, 0b1000000000000000000000000000000000ULL | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, 0b1000000000000000000000000000000000ULL | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][1].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][1].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][1].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][1].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, 0b1100000000000000000000000000000000ULL | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, 0b1100000000000000000000000000000000ULL | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][2].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][2].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][2].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][2].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, 0b1110000000000000000000000000000000ULL | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, 0b1110000000000000000000000000000000ULL | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][3].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][3].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][3].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][3].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, 0b1111000000000000000000000000000000ULL | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, 0b1111000000000000000000000000000000ULL | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][4].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][4].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][4].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][4].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, 0b1111100000000000000000000000000000ULL | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, 0b1111100000000000000000000000000000ULL | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][5].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][5].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][5].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][5].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, 0b1111110000000000000000000000000000ULL | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, 0b1111110000000000000000000000000000ULL | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][6].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][6].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][6].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][6].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, 0b1111111000000000000000000000000000ULL | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, 0b1111111000000000000000000000000000ULL | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][7].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][7].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][7].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][7].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM*/


    		R[j][0].XSB = (R[j][0].XSB * 9) / 8;
    		R[j][1].XSB = (R[j][1].XSB * 9) / 8;
    		R[j][2].XSB = (R[j][2].XSB * 9) / 8;
    		R[j][3].XSB = (R[j][3].XSB * 9) / 8;
    		R[j][4].XSB = (R[j][4].XSB * 9) / 8;
    		R[j][5].XSB = (R[j][5].XSB * 9) / 8;
    		R[j][6].XSB = (R[j][6].XSB * 9) / 8;
    		R[j][7].XSB = (R[j][7].XSB * 9) / 8;

    		/*R[j][0].XSB = R[j][2].XSB;
    		R[j][1].XSB = R[j][2].XSB;
    		R[j][4].XSB = R[j][2].XSB;
    		R[j][5].XSB = R[j][2].XSB;
    		R[j][6].XSB = R[j][2].XSB;
    		R[j][7].XSB = R[j][2].XSB;*/

            Tx6_buffer[0] = 0x55;
        	Tx6_buffer[1] = R[j][0].XSB_0;
        	Tx6_buffer[2] = R[j][0].XSB_1;
        	Tx6_buffer[3] = R[j][0].XSB_2;
        	Tx6_buffer[4] = R[j][1].XSB_0;
        	Tx6_buffer[5] = R[j][1].XSB_1;
        	Tx6_buffer[6] = R[j][1].XSB_2;
        	Tx6_buffer[7] = R[j][2].XSB_0;
        	Tx6_buffer[8] = R[j][2].XSB_1;
        	Tx6_buffer[9] = R[j][2].XSB_2;
        	Tx6_buffer[10] = R[j][3].XSB_0;
        	Tx6_buffer[11] = R[j][3].XSB_1;
        	Tx6_buffer[12] = R[j][3].XSB_2;
        	Tx6_buffer[13] = R[j][4].XSB_0;
        	Tx6_buffer[14] = R[j][4].XSB_1;
        	Tx6_buffer[15] = R[j][4].XSB_2;
        	Tx6_buffer[16] = R[j][5].XSB_0;
        	Tx6_buffer[17] = R[j][5].XSB_1;
        	Tx6_buffer[18] = R[j][5].XSB_2;
        	Tx6_buffer[19] = R[j][6].XSB_0;
        	Tx6_buffer[20] = R[j][6].XSB_1;
        	Tx6_buffer[21] = R[j][6].XSB_2;
        	Tx6_buffer[22] = R[j][7].XSB_0;
        	Tx6_buffer[23] = R[j][7].XSB_1;
        	Tx6_buffer[24] = R[j][7].XSB_2;
        	Tx6_buffer[25] = j+50;
        	CRC_.XSB = CalcCRC32(0xFFFFFFFF, (uint8_t*)(Tx6_buffer), 26);
        	Tx6_buffer[26] = CRC_.XSB_0;
        	Tx6_buffer[27] = CRC_.XSB_1;
        	Tx6_buffer[28] = CRC_.XSB_2;
        	Tx6_buffer[29] = CRC_.XSB_3;

        	for (i=0; i<30; i++)
        	    uart6_send(Tx6_buffer[i]);


        	delay_us(1000);


            Tx6_buffer[0] = 0x55;
        	Tx6_buffer[1] = VP0[j][0].XSB_0;
        	Tx6_buffer[2] = VP0[j][0].XSB_1;
        	Tx6_buffer[3] = VP0[j][0].XSB_2;
        	Tx6_buffer[4] = VP0[j][1].XSB_0;
        	Tx6_buffer[5] = VP0[j][1].XSB_1;
        	Tx6_buffer[6] = VP0[j][1].XSB_2;
        	Tx6_buffer[7] = VP0[j][2].XSB_0;
        	Tx6_buffer[8] = VP0[j][2].XSB_1;
        	Tx6_buffer[9] = VP0[j][2].XSB_2;
        	Tx6_buffer[10] = VP0[j][3].XSB_0;
        	Tx6_buffer[11] = VP0[j][3].XSB_1;
        	Tx6_buffer[12] = VP0[j][3].XSB_2;
        	Tx6_buffer[13] = VP0[j][4].XSB_0;
        	Tx6_buffer[14] = VP0[j][4].XSB_1;
        	Tx6_buffer[15] = VP0[j][4].XSB_2;
        	Tx6_buffer[16] = VP0[j][5].XSB_0;
        	Tx6_buffer[17] = VP0[j][5].XSB_1;
        	Tx6_buffer[18] = VP0[j][5].XSB_2;
        	Tx6_buffer[19] = VP0[j][6].XSB_0;
        	Tx6_buffer[20] = VP0[j][6].XSB_1;
        	Tx6_buffer[21] = VP0[j][6].XSB_2;
        	Tx6_buffer[22] = VP0[j][7].XSB_0;
        	Tx6_buffer[23] = VP0[j][7].XSB_1;
        	Tx6_buffer[24] = VP0[j][7].XSB_2;
        	Tx6_buffer[25] = j+100;
        	CRC_.XSB = CalcCRC32(0xFFFFFFFF, (uint8_t*)(Tx6_buffer), 26);
        	Tx6_buffer[26] = CRC_.XSB_0;
        	Tx6_buffer[27] = CRC_.XSB_1;
        	Tx6_buffer[28] = CRC_.XSB_2;
        	Tx6_buffer[29] = CRC_.XSB_3;

        	for (i=0; i<30; i++)
        	    uart6_send(Tx6_buffer[i]);

        	delay_us(1000);


            Tx6_buffer[0] = 0x55;
        	Tx6_buffer[1] = VP1[j][0].XSB_0;
        	Tx6_buffer[2] = VP1[j][0].XSB_1;
        	Tx6_buffer[3] = VP1[j][0].XSB_2;
        	Tx6_buffer[4] = VP1[j][1].XSB_0;
        	Tx6_buffer[5] = VP1[j][1].XSB_1;
        	Tx6_buffer[6] = VP1[j][1].XSB_2;
        	Tx6_buffer[7] = VP1[j][2].XSB_0;
        	Tx6_buffer[8] = VP1[j][2].XSB_1;
        	Tx6_buffer[9] = VP1[j][2].XSB_2;
        	Tx6_buffer[10] = VP1[j][3].XSB_0;
        	Tx6_buffer[11] = VP1[j][3].XSB_1;
        	Tx6_buffer[12] = VP1[j][3].XSB_2;
        	Tx6_buffer[13] = VP1[j][4].XSB_0;
        	Tx6_buffer[14] = VP1[j][4].XSB_1;
        	Tx6_buffer[15] = VP1[j][4].XSB_2;
        	Tx6_buffer[16] = VP1[j][5].XSB_0;
        	Tx6_buffer[17] = VP1[j][5].XSB_1;
        	Tx6_buffer[18] = VP1[j][5].XSB_2;
        	Tx6_buffer[19] = VP1[j][6].XSB_0;
        	Tx6_buffer[20] = VP1[j][6].XSB_1;
        	Tx6_buffer[21] = VP1[j][6].XSB_2;
        	Tx6_buffer[22] = VP1[j][7].XSB_0;
        	Tx6_buffer[23] = VP1[j][7].XSB_1;
        	Tx6_buffer[24] = VP1[j][7].XSB_2;
        	Tx6_buffer[25] = j+150;
        	CRC_.XSB = CalcCRC32(0xFFFFFFFF, (uint8_t*)(Tx6_buffer), 26);
        	Tx6_buffer[26] = CRC_.XSB_0;
        	Tx6_buffer[27] = CRC_.XSB_1;
        	Tx6_buffer[28] = CRC_.XSB_2;
        	Tx6_buffer[29] = CRC_.XSB_3;

        	for (i=0; i<30; i++)
        	    uart6_send(Tx6_buffer[i]);

        	delay_us(1000);


            Tx6_buffer[0] = 0x55;
        	Tx6_buffer[1] = VN[j][0].XSB_0;
        	Tx6_buffer[2] = VN[j][0].XSB_1;
        	Tx6_buffer[3] = VN[j][0].XSB_2;
        	Tx6_buffer[4] = VN[j][1].XSB_0;
        	Tx6_buffer[5] = VN[j][1].XSB_1;
        	Tx6_buffer[6] = VN[j][1].XSB_2;
        	Tx6_buffer[7] = VN[j][2].XSB_0;
        	Tx6_buffer[8] = VN[j][2].XSB_1;
        	Tx6_buffer[9] = VN[j][2].XSB_2;
        	Tx6_buffer[10] = VN[j][3].XSB_0;
        	Tx6_buffer[11] = VN[j][3].XSB_1;
        	Tx6_buffer[12] = VN[j][3].XSB_2;
        	Tx6_buffer[13] = VN[j][4].XSB_0;
        	Tx6_buffer[14] = VN[j][4].XSB_1;
        	Tx6_buffer[15] = VN[j][4].XSB_2;
        	Tx6_buffer[16] = VN[j][5].XSB_0;
        	Tx6_buffer[17] = VN[j][5].XSB_1;
        	Tx6_buffer[18] = VN[j][5].XSB_2;
        	Tx6_buffer[19] = VN[j][6].XSB_0;
        	Tx6_buffer[20] = VN[j][6].XSB_1;
        	Tx6_buffer[21] = VN[j][6].XSB_2;
        	Tx6_buffer[22] = VN[j][7].XSB_0;
        	Tx6_buffer[23] = VN[j][7].XSB_1;
        	Tx6_buffer[24] = VN[j][7].XSB_2;
        	Tx6_buffer[25] = j+200;
        	CRC_.XSB = CalcCRC32(0xFFFFFFFF, (uint8_t*)(Tx6_buffer), 26);
        	Tx6_buffer[26] = CRC_.XSB_0;
        	Tx6_buffer[27] = CRC_.XSB_1;
        	Tx6_buffer[28] = CRC_.XSB_2;
        	Tx6_buffer[29] = CRC_.XSB_3;

        	for (i=0; i<30; i++)
        	    uart6_send(Tx6_buffer[i]);



        	ADC_2.XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM

        	ADC_6.XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3REF, MUXN = AINCOM

        	ADC_3.XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

        	ADC_1.XSB = get_SPI_ADC(ADC_RX);    //MUXP = AIN1 = ADC_RB, MUXN = AINCOM

            Tx6_buffer[0] = 0x55;
        	Tx6_buffer[1] = ADC_0.XSB_0;
        	Tx6_buffer[2] = ADC_0.XSB_1;
        	Tx6_buffer[3] = ADC_0.XSB_2;
        	Tx6_buffer[4] = ADC_1.XSB_0;
        	Tx6_buffer[5] = ADC_1.XSB_1;
        	Tx6_buffer[6] = ADC_1.XSB_2;
        	Tx6_buffer[7] = ADC_2.XSB_0;
        	Tx6_buffer[8] = ADC_2.XSB_1;
        	Tx6_buffer[9] = ADC_2.XSB_2;
        	Tx6_buffer[10] = ADC_3.XSB_0;
        	Tx6_buffer[11] = ADC_3.XSB_1;
        	Tx6_buffer[12] = ADC_3.XSB_2;
        	Tx6_buffer[13] = ADC_4.XSB_0;
        	Tx6_buffer[14] = ADC_4.XSB_1;
        	Tx6_buffer[15] = ADC_4.XSB_2;
        	Tx6_buffer[16] = ADC_5.XSB_0;
        	Tx6_buffer[17] = ADC_5.XSB_1;
        	Tx6_buffer[18] = ADC_5.XSB_2;
        	Tx6_buffer[19] = ADC_6.XSB_0;
        	Tx6_buffer[20] = ADC_6.XSB_1;
        	Tx6_buffer[21] = ADC_6.XSB_2;
        	Tx6_buffer[22] = ADC_7.XSB_0;
        	Tx6_buffer[23] = ADC_7.XSB_1;
        	Tx6_buffer[24] = ADC_7.XSB_2;
        	Tx6_buffer[25] = 0;
        	CRC_.XSB = CalcCRC32(0xFFFFFFFF, (uint8_t*)(Tx6_buffer), 26);
        	Tx6_buffer[26] = CRC_.XSB_0;
        	Tx6_buffer[27] = CRC_.XSB_1;
        	Tx6_buffer[28] = CRC_.XSB_2;
        	Tx6_buffer[29] = CRC_.XSB_3;

        	for (i=0; i<30; i++)
        	    uart6_send(Tx6_buffer[i]);
    	    }



    	for (j=27; j<34; j++)
    	    {
    		if (R_MUX == 0)
    		    {
    			set_FPGA(0ULL | RB_MUX_ADDR, 0ULL | RB_MUX_ADDR);
    			delay_us(1000);
    		    }

    		if (R_MUX == 1)
    		    {
    			set_FPGA(1ULL | RB_MUX_ADDR, 1ULL | RB_MUX_ADDR);
    			delay_us(1000);
    		    }

    		if (R_MUX == 2)
    		    {
    			for (i=8; i<34; i++)
    			    {
    				set_FPGA((R[i][3].XSB | ((RB_x_start_ADDR + (uint64_t)i) << 34)), (R[i][3].XSB | ((RB_x_start_ADDR + (uint64_t)i) << 34)));
    				delay_us(1000);
    			    }

    			set_FPGA(1ULL | RB_MUX_ADDR, 1ULL | RB_MUX_ADDR);
    			delay_us(1000);

    			R_MUX = 1;
    		    }

    		/*set_FPGA(0ULL, 0b0000000000000000000000000000011111ULL | RB_ADDR);    //Z korekcj¹ poboru pr¹du
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, 0b0000000000000000000000000000011110ULL | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][0].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][0].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][0].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][0].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, 0b0010000000000000000000000000011110ULL | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, 0b0010000000000000000000000000011011ULL | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][1].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][1].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][1].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][1].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, 0b1100000000000000000000000000011101ULL | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, 0b1100000000000000000000000000011100ULL | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][2].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][2].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][2].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][2].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, 0b1110000000000000000000000000011100ULL | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, 0b1110000000000000000000000000011001ULL | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][3].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][3].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][3].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][3].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM*/


    		set_FPGA(0ULL, I_compensate(Calibration_state[j][0]) | RB_ADDR);    //Z korekcj¹ poboru pr¹du
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, I_compensate(Calibration_state[j][0] | (1ULL << (uint64_t)j)) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][0].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][0].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][0].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][0].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, I_compensate(Calibration_state[j][1]) | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, I_compensate(Calibration_state[j][1] | (1ULL << (uint64_t)j)) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][1].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][1].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][1].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][1].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, I_compensate(Calibration_state[j][2]) | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, I_compensate(Calibration_state[j][2] | (1ULL << (uint64_t)j)) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][2].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][2].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][2].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][2].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, I_compensate(Calibration_state[j][3]) | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, I_compensate(Calibration_state[j][3] | (1ULL << (uint64_t)j)) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][3].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][3].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][3].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][3].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, I_compensate(Calibration_state[j][4]) | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, I_compensate(Calibration_state[j][4] | (1ULL << (uint64_t)j)) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][4].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][4].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][4].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][4].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, I_compensate(Calibration_state[j][5]) | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, I_compensate(Calibration_state[j][5] | (1ULL << (uint64_t)j)) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][5].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][5].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][5].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][5].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, I_compensate(Calibration_state[j][6]) | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, I_compensate(Calibration_state[j][6] | (1ULL << (uint64_t)j)) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][6].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][6].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][6].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][6].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, I_compensate(Calibration_state[j][7]) | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, I_compensate(Calibration_state[j][7] | (1ULL << (uint64_t)j)) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][7].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][7].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][7].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][7].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM



    		/*set_FPGA(0ULL, Calibration_state[j][0] | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, Calibration_state[j][0] | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][0].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][0].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][0].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][0].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, Calibration_state[j][1] | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, Calibration_state[j][1] | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][1].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][1].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][1].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][1].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, Calibration_state[j][2] | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, Calibration_state[j][2] | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][2].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][2].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][2].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][2].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, Calibration_state[j][3] | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, Calibration_state[j][3] | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][3].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][3].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][3].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][3].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, Calibration_state[j][4] | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, Calibration_state[j][4] | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][4].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][4].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][4].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][4].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, Calibration_state[j][5] | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, Calibration_state[j][5] | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][5].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][5].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][5].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][5].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, Calibration_state[j][6] | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, Calibration_state[j][6] | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][6].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][6].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][6].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][6].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    		set_FPGA(0ULL, Calibration_state[j][7] | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, Calibration_state[j][7] | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		R[j][7].XSB = ADC_4.XSB - ADC_5.XSB;
    		VP0[j][7].XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM
    		VP1[j][7].XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3, MUXN = AINCOM
    		VN[j][7].XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM*/


    		R[j][0].XSB = (R[j][0].XSB * 9) / 8;
    		R[j][1].XSB = (R[j][1].XSB * 9) / 8;
    		R[j][2].XSB = (R[j][2].XSB * 9) / 8;
    		R[j][3].XSB = (R[j][3].XSB * 9) / 8;
    		R[j][4].XSB = (R[j][4].XSB * 9) / 8;
    		R[j][5].XSB = (R[j][5].XSB * 9) / 8;
    		R[j][6].XSB = (R[j][6].XSB * 9) / 8;
    		R[j][7].XSB = (R[j][7].XSB * 9) / 8;

    		/*R[j][0].XSB = R[j][2].XSB;
    		R[j][1].XSB = R[j][2].XSB;
    		R[j][4].XSB = R[j][2].XSB;
    		R[j][5].XSB = R[j][2].XSB;
    		R[j][6].XSB = R[j][2].XSB;
    		R[j][7].XSB = R[j][2].XSB;*/

            Tx6_buffer[0] = 0x55;
        	Tx6_buffer[1] = R[j][0].XSB_0;
        	Tx6_buffer[2] = R[j][0].XSB_1;
        	Tx6_buffer[3] = R[j][0].XSB_2;
        	Tx6_buffer[4] = R[j][1].XSB_0;
        	Tx6_buffer[5] = R[j][1].XSB_1;
        	Tx6_buffer[6] = R[j][1].XSB_2;
        	Tx6_buffer[7] = R[j][2].XSB_0;
        	Tx6_buffer[8] = R[j][2].XSB_1;
        	Tx6_buffer[9] = R[j][2].XSB_2;
        	Tx6_buffer[10] = R[j][3].XSB_0;
        	Tx6_buffer[11] = R[j][3].XSB_1;
        	Tx6_buffer[12] = R[j][3].XSB_2;
        	Tx6_buffer[13] = R[j][4].XSB_0;
        	Tx6_buffer[14] = R[j][4].XSB_1;
        	Tx6_buffer[15] = R[j][4].XSB_2;
        	Tx6_buffer[16] = R[j][5].XSB_0;
        	Tx6_buffer[17] = R[j][5].XSB_1;
        	Tx6_buffer[18] = R[j][5].XSB_2;
        	Tx6_buffer[19] = R[j][6].XSB_0;
        	Tx6_buffer[20] = R[j][6].XSB_1;
        	Tx6_buffer[21] = R[j][6].XSB_2;
        	Tx6_buffer[22] = R[j][7].XSB_0;
        	Tx6_buffer[23] = R[j][7].XSB_1;
        	Tx6_buffer[24] = R[j][7].XSB_2;
        	Tx6_buffer[25] = j+50;
        	CRC_.XSB = CalcCRC32(0xFFFFFFFF, (uint8_t*)(Tx6_buffer), 26);
        	Tx6_buffer[26] = CRC_.XSB_0;
        	Tx6_buffer[27] = CRC_.XSB_1;
        	Tx6_buffer[28] = CRC_.XSB_2;
        	Tx6_buffer[29] = CRC_.XSB_3;

        	for (i=0; i<30; i++)
        	    uart6_send(Tx6_buffer[i]);


        	delay_us(1000);


            Tx6_buffer[0] = 0x55;
        	Tx6_buffer[1] = VP0[j][0].XSB_0;
        	Tx6_buffer[2] = VP0[j][0].XSB_1;
        	Tx6_buffer[3] = VP0[j][0].XSB_2;
        	Tx6_buffer[4] = VP0[j][1].XSB_0;
        	Tx6_buffer[5] = VP0[j][1].XSB_1;
        	Tx6_buffer[6] = VP0[j][1].XSB_2;
        	Tx6_buffer[7] = VP0[j][2].XSB_0;
        	Tx6_buffer[8] = VP0[j][2].XSB_1;
        	Tx6_buffer[9] = VP0[j][2].XSB_2;
        	Tx6_buffer[10] = VP0[j][3].XSB_0;
        	Tx6_buffer[11] = VP0[j][3].XSB_1;
        	Tx6_buffer[12] = VP0[j][3].XSB_2;
        	Tx6_buffer[13] = VP0[j][4].XSB_0;
        	Tx6_buffer[14] = VP0[j][4].XSB_1;
        	Tx6_buffer[15] = VP0[j][4].XSB_2;
        	Tx6_buffer[16] = VP0[j][5].XSB_0;
        	Tx6_buffer[17] = VP0[j][5].XSB_1;
        	Tx6_buffer[18] = VP0[j][5].XSB_2;
        	Tx6_buffer[19] = VP0[j][6].XSB_0;
        	Tx6_buffer[20] = VP0[j][6].XSB_1;
        	Tx6_buffer[21] = VP0[j][6].XSB_2;
        	Tx6_buffer[22] = VP0[j][7].XSB_0;
        	Tx6_buffer[23] = VP0[j][7].XSB_1;
        	Tx6_buffer[24] = VP0[j][7].XSB_2;
        	Tx6_buffer[25] = j+100;
        	CRC_.XSB = CalcCRC32(0xFFFFFFFF, (uint8_t*)(Tx6_buffer), 26);
        	Tx6_buffer[26] = CRC_.XSB_0;
        	Tx6_buffer[27] = CRC_.XSB_1;
        	Tx6_buffer[28] = CRC_.XSB_2;
        	Tx6_buffer[29] = CRC_.XSB_3;

        	for (i=0; i<30; i++)
        	    uart6_send(Tx6_buffer[i]);

        	delay_us(1000);


            Tx6_buffer[0] = 0x55;
        	Tx6_buffer[1] = VP1[j][0].XSB_0;
        	Tx6_buffer[2] = VP1[j][0].XSB_1;
        	Tx6_buffer[3] = VP1[j][0].XSB_2;
        	Tx6_buffer[4] = VP1[j][1].XSB_0;
        	Tx6_buffer[5] = VP1[j][1].XSB_1;
        	Tx6_buffer[6] = VP1[j][1].XSB_2;
        	Tx6_buffer[7] = VP1[j][2].XSB_0;
        	Tx6_buffer[8] = VP1[j][2].XSB_1;
        	Tx6_buffer[9] = VP1[j][2].XSB_2;
        	Tx6_buffer[10] = VP1[j][3].XSB_0;
        	Tx6_buffer[11] = VP1[j][3].XSB_1;
        	Tx6_buffer[12] = VP1[j][3].XSB_2;
        	Tx6_buffer[13] = VP1[j][4].XSB_0;
        	Tx6_buffer[14] = VP1[j][4].XSB_1;
        	Tx6_buffer[15] = VP1[j][4].XSB_2;
        	Tx6_buffer[16] = VP1[j][5].XSB_0;
        	Tx6_buffer[17] = VP1[j][5].XSB_1;
        	Tx6_buffer[18] = VP1[j][5].XSB_2;
        	Tx6_buffer[19] = VP1[j][6].XSB_0;
        	Tx6_buffer[20] = VP1[j][6].XSB_1;
        	Tx6_buffer[21] = VP1[j][6].XSB_2;
        	Tx6_buffer[22] = VP1[j][7].XSB_0;
        	Tx6_buffer[23] = VP1[j][7].XSB_1;
        	Tx6_buffer[24] = VP1[j][7].XSB_2;
        	Tx6_buffer[25] = j+150;
        	CRC_.XSB = CalcCRC32(0xFFFFFFFF, (uint8_t*)(Tx6_buffer), 26);
        	Tx6_buffer[26] = CRC_.XSB_0;
        	Tx6_buffer[27] = CRC_.XSB_1;
        	Tx6_buffer[28] = CRC_.XSB_2;
        	Tx6_buffer[29] = CRC_.XSB_3;

        	for (i=0; i<30; i++)
        	    uart6_send(Tx6_buffer[i]);

        	delay_us(1000);


            Tx6_buffer[0] = 0x55;
        	Tx6_buffer[1] = VN[j][0].XSB_0;
        	Tx6_buffer[2] = VN[j][0].XSB_1;
        	Tx6_buffer[3] = VN[j][0].XSB_2;
        	Tx6_buffer[4] = VN[j][1].XSB_0;
        	Tx6_buffer[5] = VN[j][1].XSB_1;
        	Tx6_buffer[6] = VN[j][1].XSB_2;
        	Tx6_buffer[7] = VN[j][2].XSB_0;
        	Tx6_buffer[8] = VN[j][2].XSB_1;
        	Tx6_buffer[9] = VN[j][2].XSB_2;
        	Tx6_buffer[10] = VN[j][3].XSB_0;
        	Tx6_buffer[11] = VN[j][3].XSB_1;
        	Tx6_buffer[12] = VN[j][3].XSB_2;
        	Tx6_buffer[13] = VN[j][4].XSB_0;
        	Tx6_buffer[14] = VN[j][4].XSB_1;
        	Tx6_buffer[15] = VN[j][4].XSB_2;
        	Tx6_buffer[16] = VN[j][5].XSB_0;
        	Tx6_buffer[17] = VN[j][5].XSB_1;
        	Tx6_buffer[18] = VN[j][5].XSB_2;
        	Tx6_buffer[19] = VN[j][6].XSB_0;
        	Tx6_buffer[20] = VN[j][6].XSB_1;
        	Tx6_buffer[21] = VN[j][6].XSB_2;
        	Tx6_buffer[22] = VN[j][7].XSB_0;
        	Tx6_buffer[23] = VN[j][7].XSB_1;
        	Tx6_buffer[24] = VN[j][7].XSB_2;
        	Tx6_buffer[25] = j+200;
        	CRC_.XSB = CalcCRC32(0xFFFFFFFF, (uint8_t*)(Tx6_buffer), 26);
        	Tx6_buffer[26] = CRC_.XSB_0;
        	Tx6_buffer[27] = CRC_.XSB_1;
        	Tx6_buffer[28] = CRC_.XSB_2;
        	Tx6_buffer[29] = CRC_.XSB_3;

        	for (i=0; i<30; i++)
        	    uart6_send(Tx6_buffer[i]);



        	ADC_2.XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM

        	ADC_6.XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3REF, MUXN = AINCOM

        	ADC_3.XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

        	ADC_1.XSB = get_SPI_ADC(ADC_RX);    //MUXP = AIN1 = ADC_RB, MUXN = AINCOM


            Tx6_buffer[0] = 0x55;
        	Tx6_buffer[1] = ADC_0.XSB_0;
        	Tx6_buffer[2] = ADC_0.XSB_1;
        	Tx6_buffer[3] = ADC_0.XSB_2;
        	Tx6_buffer[4] = ADC_1.XSB_0;
        	Tx6_buffer[5] = ADC_1.XSB_1;
        	Tx6_buffer[6] = ADC_1.XSB_2;
        	Tx6_buffer[7] = ADC_2.XSB_0;
        	Tx6_buffer[8] = ADC_2.XSB_1;
        	Tx6_buffer[9] = ADC_2.XSB_2;
        	Tx6_buffer[10] = ADC_3.XSB_0;
        	Tx6_buffer[11] = ADC_3.XSB_1;
        	Tx6_buffer[12] = ADC_3.XSB_2;
        	Tx6_buffer[13] = ADC_4.XSB_0;
        	Tx6_buffer[14] = ADC_4.XSB_1;
        	Tx6_buffer[15] = ADC_4.XSB_2;
        	Tx6_buffer[16] = ADC_5.XSB_0;
        	Tx6_buffer[17] = ADC_5.XSB_1;
        	Tx6_buffer[18] = ADC_5.XSB_2;
        	Tx6_buffer[19] = ADC_6.XSB_0;
        	Tx6_buffer[20] = ADC_6.XSB_1;
        	Tx6_buffer[21] = ADC_6.XSB_2;
        	Tx6_buffer[22] = ADC_7.XSB_0;
        	Tx6_buffer[23] = ADC_7.XSB_1;
        	Tx6_buffer[24] = ADC_7.XSB_2;
        	Tx6_buffer[25] = 0;
        	CRC_.XSB = CalcCRC32(0xFFFFFFFF, (uint8_t*)(Tx6_buffer), 26);
        	Tx6_buffer[26] = CRC_.XSB_0;
        	Tx6_buffer[27] = CRC_.XSB_1;
        	Tx6_buffer[28] = CRC_.XSB_2;
        	Tx6_buffer[29] = CRC_.XSB_3;

        	for (i=0; i<30; i++)
        	    uart6_send(Tx6_buffer[i]);
    	    }
#endif


#if Kalibracja == 2
    	for (j=9; j<27; j++)
    	    {
    		set_FPGA(0ULL, 0b0000000000000000000000000000000111ULL | RB_ADDR);    //Z korekcj¹ poboru pr¹du
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RB);
    		set_FPGA(0ULL, 0b0000000000000000000000000000000111ULL | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RB);
    		RR[j][0].XSB = ADC_4.XSB - ADC_5.XSB;

    		set_FPGA(0ULL, 0b1000000000000000000000000000100101ULL | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RB);
    		set_FPGA(0ULL, 0b1000000000000000000000000000100101ULL | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RB);
    		RR[j][1].XSB = ADC_4.XSB - ADC_5.XSB;

    		set_FPGA(0ULL, 0b1100000000000000000000000000000101ULL | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RB);
    		set_FPGA(0ULL, 0b1100000000000000000000000000000101ULL | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RB);
    		RR[j][2].XSB = ADC_4.XSB - ADC_5.XSB;

    		set_FPGA(0ULL, 0b1110000000000000000000000000000100ULL | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RB);
    		set_FPGA(0ULL, 0b1110000000000000000000000000000100ULL | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RB);
    		RR[j][3].XSB = ADC_4.XSB - ADC_5.XSB;

    		set_FPGA(0ULL, 0b1111000000000000000000000000010001ULL | RB_ADDR);    //Z korekcj¹ poboru pr¹du
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RB);
    		set_FPGA(0ULL, 0b1111000000000000000000000000010001ULL | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RB);
    		RR[j][4].XSB = ADC_4.XSB - ADC_5.XSB;

    		set_FPGA(0ULL, 0b1111100000000000000000000000010000ULL | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RB);
    		set_FPGA(0ULL, 0b1111100000000000000000000000010000ULL | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RB);
    		RR[j][5].XSB = ADC_4.XSB - ADC_5.XSB;

    		set_FPGA(0ULL, 0b1111110000000000000000000000000001ULL | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RB);
    		set_FPGA(0ULL, 0b1111110000000000000000000000000001ULL | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RB);
    		RR[j][6].XSB = ADC_4.XSB - ADC_5.XSB;

    		set_FPGA(0ULL, 0b1111111000000000000000000000000000ULL | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RB);
    		set_FPGA(0ULL, 0b1111111000000000000000000000000000ULL | (1ULL << (uint64_t)j) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RB);
    		RR[j][7].XSB = ADC_4.XSB - ADC_5.XSB;




    		/*RR[j][0].XSB = (RR[j][0].XSB * 9) / 8;
    		RR[j][1].XSB = (RR[j][1].XSB * 9) / 8;
    		RR[j][2].XSB = (RR[j][2].XSB * 9) / 8;
    		RR[j][3].XSB = (RR[j][3].XSB * 9) / 8;
    		RR[j][4].XSB = (RR[j][4].XSB * 9) / 8;
    		RR[j][5].XSB = (RR[j][5].XSB * 9) / 8;
    		RR[j][6].XSB = (RR[j][6].XSB * 9) / 8;
    		RR[j][7].XSB = (RR[j][7].XSB * 9) / 8;*/
    		RR[j][0].XSB = (RR[j][0].XSB * 11) / 8;
    		RR[j][1].XSB = (RR[j][1].XSB * 11) / 8;
    		RR[j][2].XSB = (RR[j][2].XSB * 11) / 8;
    		RR[j][3].XSB = (RR[j][3].XSB * 11) / 8;
    		RR[j][4].XSB = (RR[j][4].XSB * 11) / 8;
    		RR[j][5].XSB = (RR[j][5].XSB * 11) / 8;
    		RR[j][6].XSB = (RR[j][6].XSB * 11) / 8;
    		RR[j][7].XSB = (RR[j][7].XSB * 11) / 8;



            Tx6_buffer[0] = 0x55;
        	Tx6_buffer[1] = RR[j][0].XSB_0;
        	Tx6_buffer[2] = RR[j][0].XSB_1;
        	Tx6_buffer[3] = RR[j][0].XSB_2;
        	Tx6_buffer[4] = RR[j][1].XSB_0;
        	Tx6_buffer[5] = RR[j][1].XSB_1;
        	Tx6_buffer[6] = RR[j][1].XSB_2;
        	Tx6_buffer[7] = RR[j][2].XSB_0;
        	Tx6_buffer[8] = RR[j][2].XSB_1;
        	Tx6_buffer[9] = RR[j][2].XSB_2;
        	Tx6_buffer[10] = RR[j][3].XSB_0;
        	Tx6_buffer[11] = RR[j][3].XSB_1;
        	Tx6_buffer[12] = RR[j][3].XSB_2;
        	Tx6_buffer[13] = RR[j][4].XSB_0;
        	Tx6_buffer[14] = RR[j][4].XSB_1;
        	Tx6_buffer[15] = RR[j][4].XSB_2;
        	Tx6_buffer[16] = RR[j][5].XSB_0;
        	Tx6_buffer[17] = RR[j][5].XSB_1;
        	Tx6_buffer[18] = RR[j][5].XSB_2;
        	Tx6_buffer[19] = RR[j][6].XSB_0;
        	Tx6_buffer[20] = RR[j][6].XSB_1;
        	Tx6_buffer[21] = RR[j][6].XSB_2;
        	Tx6_buffer[22] = RR[j][7].XSB_0;
        	Tx6_buffer[23] = RR[j][7].XSB_1;
        	Tx6_buffer[24] = RR[j][7].XSB_2;
        	Tx6_buffer[25] = j+50;
        	CRC_.XSB = CalcCRC32(0xFFFFFFFF, (uint8_t*)(Tx6_buffer), 26);
        	Tx6_buffer[26] = CRC_.XSB_0;
        	Tx6_buffer[27] = CRC_.XSB_1;
        	Tx6_buffer[28] = CRC_.XSB_2;
        	Tx6_buffer[29] = CRC_.XSB_3;

        	for (i=0; i<30; i++)
        	    uart6_send(Tx6_buffer[i]);


        	delay_us(1000);
    	    }



    	for (j=27; j<34; j++)
    	    {
    		set_FPGA(0ULL, I_compensate(Calibration_state[j][0]) | RB_ADDR);    //Z korekcj¹ poboru pr¹du
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RB);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, I_compensate(Calibration_state[j][0] | (1ULL << (uint64_t)j)) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RB);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		RR[j][0].XSB = ADC_4.XSB - ADC_5.XSB;

    		set_FPGA(0ULL, I_compensate(Calibration_state[j][1]) | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RB);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, I_compensate(Calibration_state[j][1] | (1ULL << (uint64_t)j)) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RB);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		RR[j][1].XSB = ADC_4.XSB - ADC_5.XSB;

    		set_FPGA(0ULL, I_compensate(Calibration_state[j][2]) | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RB);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, I_compensate(Calibration_state[j][2] | (1ULL << (uint64_t)j)) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RB);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		RR[j][2].XSB = ADC_4.XSB - ADC_5.XSB;

    		set_FPGA(0ULL, I_compensate(Calibration_state[j][3]) | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RB);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, I_compensate(Calibration_state[j][3] | (1ULL << (uint64_t)j)) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RB);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		RR[j][3].XSB = ADC_4.XSB - ADC_5.XSB;

    		set_FPGA(0ULL, I_compensate(Calibration_state[j][4]) | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RB);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, I_compensate(Calibration_state[j][4] | (1ULL << (uint64_t)j)) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RB);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		RR[j][4].XSB = ADC_4.XSB - ADC_5.XSB;

    		set_FPGA(0ULL, I_compensate(Calibration_state[j][5]) | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RB);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, I_compensate(Calibration_state[j][5] | (1ULL << (uint64_t)j)) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RB);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		RR[j][5].XSB = ADC_4.XSB - ADC_5.XSB;

    		set_FPGA(0ULL, I_compensate(Calibration_state[j][6]) | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RB);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, I_compensate(Calibration_state[j][6] | (1ULL << (uint64_t)j)) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RB);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		RR[j][6].XSB = ADC_4.XSB - ADC_5.XSB;

    		set_FPGA(0ULL, I_compensate(Calibration_state[j][7]) | RB_ADDR);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_RB);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(0ULL, I_compensate(Calibration_state[j][7] | (1ULL << (uint64_t)j)) | RB_ADDR);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_RB);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		RR[j][7].XSB = ADC_4.XSB - ADC_5.XSB;




    		/*RR[j][0].XSB = (RR[j][0].XSB * 9) / 8;
    		RR[j][1].XSB = (RR[j][1].XSB * 9) / 8;
    		RR[j][2].XSB = (RR[j][2].XSB * 9) / 8;
    		RR[j][3].XSB = (RR[j][3].XSB * 9) / 8;
    		RR[j][4].XSB = (RR[j][4].XSB * 9) / 8;
    		RR[j][5].XSB = (RR[j][5].XSB * 9) / 8;
    		RR[j][6].XSB = (RR[j][6].XSB * 9) / 8;
    		RR[j][7].XSB = (RR[j][7].XSB * 9) / 8;*/
    		RR[j][0].XSB = (RR[j][0].XSB * 11) / 8;
    		RR[j][1].XSB = (RR[j][1].XSB * 11) / 8;
    		RR[j][2].XSB = (RR[j][2].XSB * 11) / 8;
    		RR[j][3].XSB = (RR[j][3].XSB * 11) / 8;
    		RR[j][4].XSB = (RR[j][4].XSB * 11) / 8;
    		RR[j][5].XSB = (RR[j][5].XSB * 11) / 8;
    		RR[j][6].XSB = (RR[j][6].XSB * 11) / 8;
    		RR[j][7].XSB = (RR[j][7].XSB * 11) / 8;



            Tx6_buffer[0] = 0x55;
        	Tx6_buffer[1] = RR[j][0].XSB_0;
        	Tx6_buffer[2] = RR[j][0].XSB_1;
        	Tx6_buffer[3] = RR[j][0].XSB_2;
        	Tx6_buffer[4] = RR[j][1].XSB_0;
        	Tx6_buffer[5] = RR[j][1].XSB_1;
        	Tx6_buffer[6] = RR[j][1].XSB_2;
        	Tx6_buffer[7] = RR[j][2].XSB_0;
        	Tx6_buffer[8] = RR[j][2].XSB_1;
        	Tx6_buffer[9] = RR[j][2].XSB_2;
        	Tx6_buffer[10] = RR[j][3].XSB_0;
        	Tx6_buffer[11] = RR[j][3].XSB_1;
        	Tx6_buffer[12] = RR[j][3].XSB_2;
        	Tx6_buffer[13] = RR[j][4].XSB_0;
        	Tx6_buffer[14] = RR[j][4].XSB_1;
        	Tx6_buffer[15] = RR[j][4].XSB_2;
        	Tx6_buffer[16] = RR[j][5].XSB_0;
        	Tx6_buffer[17] = RR[j][5].XSB_1;
        	Tx6_buffer[18] = RR[j][5].XSB_2;
        	Tx6_buffer[19] = RR[j][6].XSB_0;
        	Tx6_buffer[20] = RR[j][6].XSB_1;
        	Tx6_buffer[21] = RR[j][6].XSB_2;
        	Tx6_buffer[22] = RR[j][7].XSB_0;
        	Tx6_buffer[23] = RR[j][7].XSB_1;
        	Tx6_buffer[24] = RR[j][7].XSB_2;
        	Tx6_buffer[25] = j+50;
        	CRC_.XSB = CalcCRC32(0xFFFFFFFF, (uint8_t*)(Tx6_buffer), 26);
        	Tx6_buffer[26] = CRC_.XSB_0;
        	Tx6_buffer[27] = CRC_.XSB_1;
        	Tx6_buffer[28] = CRC_.XSB_2;
        	Tx6_buffer[29] = CRC_.XSB_3;

        	for (i=0; i<30; i++)
        	    uart6_send(Tx6_buffer[i]);


        	delay_us(1000);
    	    }





#if Jeden_kanal == 0
    	for (j=9; j<27; j++)
    	    {
    		set_FPGA(0b0000000000000000000000000000000111ULL | RB_ADDR, 0ULL);    //Z korekcj¹ poboru pr¹du
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_LB);
    		set_FPGA(0b0000000000000000000000000000000111ULL | (1ULL << (uint64_t)j) | RB_ADDR, 0ULL);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_LB);
    		RL[j][0].XSB = ADC_4.XSB - ADC_5.XSB;

    		set_FPGA(0b1000000000000000000000000000100101ULL | RB_ADDR, 0ULL);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_LB);
    		set_FPGA(0b1000000000000000000000000000100101ULL | (1ULL << (uint64_t)j) | RB_ADDR, 0ULL);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_LB);
    		RL[j][1].XSB = ADC_4.XSB - ADC_5.XSB;

    		set_FPGA(0b1100000000000000000000000000000101ULL | RB_ADDR, 0ULL);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_LB);
    		set_FPGA(0b1100000000000000000000000000000101ULL | (1ULL << (uint64_t)j) | RB_ADDR, 0ULL);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_LB);
    		RL[j][2].XSB = ADC_4.XSB - ADC_5.XSB;

    		set_FPGA(0b1110000000000000000000000000000100ULL | RB_ADDR, 0ULL);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_LB);
    		set_FPGA(0b1110000000000000000000000000000100ULL | (1ULL << (uint64_t)j) | RB_ADDR, 0ULL);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_LB);
    		RL[j][3].XSB = ADC_4.XSB - ADC_5.XSB;

    		set_FPGA(0b1111000000000000000000000000010001ULL | RB_ADDR, 0ULL);    //Z korekcj¹ poboru pr¹du
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_LB);
    		set_FPGA(0b1111000000000000000000000000010001ULL | (1ULL << (uint64_t)j) | RB_ADDR, 0ULL);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_LB);
    		RL[j][4].XSB = ADC_4.XSB - ADC_5.XSB;

    		set_FPGA(0b1111100000000000000000000000010000ULL | RB_ADDR, 0ULL);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_LB);
    		set_FPGA(0b1111100000000000000000000000010000ULL | (1ULL << (uint64_t)j) | RB_ADDR, 0ULL);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_LB);
    		RL[j][5].XSB = ADC_4.XSB - ADC_5.XSB;

    		set_FPGA(0b1111110000000000000000000000000001ULL | RB_ADDR, 0ULL);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_LB);
    		set_FPGA(0b1111110000000000000000000000000001ULL | (1ULL << (uint64_t)j) | RB_ADDR, 0ULL);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_LB);
    		RL[j][6].XSB = ADC_4.XSB - ADC_5.XSB;

    		set_FPGA(0b1111111000000000000000000000000000ULL | RB_ADDR, 0ULL);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_LB);
    		set_FPGA(0b1111111000000000000000000000000000ULL | (1ULL << (uint64_t)j) | RB_ADDR, 0ULL);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_LB);
    		RL[j][7].XSB = ADC_4.XSB - ADC_5.XSB;




    		/*RL[j][0].XSB = (RL[j][0].XSB * 9) / 8;
    		RL[j][1].XSB = (RL[j][1].XSB * 9) / 8;
    		RL[j][2].XSB = (RL[j][2].XSB * 9) / 8;
    		RL[j][3].XSB = (RL[j][3].XSB * 9) / 8;
    		RL[j][4].XSB = (RL[j][4].XSB * 9) / 8;
    		RL[j][5].XSB = (RL[j][5].XSB * 9) / 8;
    		RL[j][6].XSB = (RL[j][6].XSB * 9) / 8;
    		RL[j][7].XSB = (RL[j][7].XSB * 9) / 8;*/
    		RL[j][0].XSB = (RL[j][0].XSB * 11) / 8;
    		RL[j][1].XSB = (RL[j][1].XSB * 11) / 8;
    		RL[j][2].XSB = (RL[j][2].XSB * 11) / 8;
    		RL[j][3].XSB = (RL[j][3].XSB * 11) / 8;
    		RL[j][4].XSB = (RL[j][4].XSB * 11) / 8;
    		RL[j][5].XSB = (RL[j][5].XSB * 11) / 8;
    		RL[j][6].XSB = (RL[j][6].XSB * 11) / 8;
    		RL[j][7].XSB = (RL[j][7].XSB * 11) / 8;



            Tx6_buffer[0] = 0x55;
        	Tx6_buffer[1] = RL[j][0].XSB_0;
        	Tx6_buffer[2] = RL[j][0].XSB_1;
        	Tx6_buffer[3] = RL[j][0].XSB_2;
        	Tx6_buffer[4] = RL[j][1].XSB_0;
        	Tx6_buffer[5] = RL[j][1].XSB_1;
        	Tx6_buffer[6] = RL[j][1].XSB_2;
        	Tx6_buffer[7] = RL[j][2].XSB_0;
        	Tx6_buffer[8] = RL[j][2].XSB_1;
        	Tx6_buffer[9] = RL[j][2].XSB_2;
        	Tx6_buffer[10] = RL[j][3].XSB_0;
        	Tx6_buffer[11] = RL[j][3].XSB_1;
        	Tx6_buffer[12] = RL[j][3].XSB_2;
        	Tx6_buffer[13] = RL[j][4].XSB_0;
        	Tx6_buffer[14] = RL[j][4].XSB_1;
        	Tx6_buffer[15] = RL[j][4].XSB_2;
        	Tx6_buffer[16] = RL[j][5].XSB_0;
        	Tx6_buffer[17] = RL[j][5].XSB_1;
        	Tx6_buffer[18] = RL[j][5].XSB_2;
        	Tx6_buffer[19] = RL[j][6].XSB_0;
        	Tx6_buffer[20] = RL[j][6].XSB_1;
        	Tx6_buffer[21] = RL[j][6].XSB_2;
        	Tx6_buffer[22] = RL[j][7].XSB_0;
        	Tx6_buffer[23] = RL[j][7].XSB_1;
        	Tx6_buffer[24] = RL[j][7].XSB_2;
        	Tx6_buffer[25] = j+50;
        	CRC_.XSB = CalcCRC32(0xFFFFFFFF, (uint8_t*)(Tx6_buffer), 26);
        	Tx6_buffer[26] = CRC_.XSB_0;
        	Tx6_buffer[27] = CRC_.XSB_1;
        	Tx6_buffer[28] = CRC_.XSB_2;
        	Tx6_buffer[29] = CRC_.XSB_3;

        	for (i=0; i<30; i++)
        	    uart6_send(Tx6_buffer[i]);


        	delay_us(1000);
    	    }



    	for (j=27; j<34; j++)
    	    {
    		set_FPGA(I_compensate(Calibration_state[j][0]) | RB_ADDR, 0ULL);    //Z korekcj¹ poboru pr¹du
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_LB);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(I_compensate(Calibration_state[j][0] | (1ULL << (uint64_t)j)) | RB_ADDR, 0ULL);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_LB);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		RL[j][0].XSB = ADC_4.XSB - ADC_5.XSB;

    		set_FPGA(I_compensate(Calibration_state[j][1]) | RB_ADDR, 0ULL);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_LB);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(I_compensate(Calibration_state[j][1] | (1ULL << (uint64_t)j)) | RB_ADDR, 0ULL);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_LB);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		RL[j][1].XSB = ADC_4.XSB - ADC_5.XSB;

    		set_FPGA(I_compensate(Calibration_state[j][2]) | RB_ADDR, 0ULL);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_LB);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(I_compensate(Calibration_state[j][2] | (1ULL << (uint64_t)j)) | RB_ADDR, 0ULL);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_LB);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		RL[j][2].XSB = ADC_4.XSB - ADC_5.XSB;

    		set_FPGA(I_compensate(Calibration_state[j][3]) | RB_ADDR, 0ULL);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_LB);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(I_compensate(Calibration_state[j][3] | (1ULL << (uint64_t)j)) | RB_ADDR, 0ULL);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_LB);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		RL[j][3].XSB = ADC_4.XSB - ADC_5.XSB;

    		set_FPGA(I_compensate(Calibration_state[j][4]) | RB_ADDR, 0ULL);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_LB);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(I_compensate(Calibration_state[j][4] | (1ULL << (uint64_t)j)) | RB_ADDR, 0ULL);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_LB);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		RL[j][4].XSB = ADC_4.XSB - ADC_5.XSB;

    		set_FPGA(I_compensate(Calibration_state[j][5]) | RB_ADDR, 0ULL);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_LB);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(I_compensate(Calibration_state[j][5] | (1ULL << (uint64_t)j)) | RB_ADDR, 0ULL);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_LB);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		RL[j][5].XSB = ADC_4.XSB - ADC_5.XSB;

    		set_FPGA(I_compensate(Calibration_state[j][6]) | RB_ADDR, 0ULL);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_LB);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(I_compensate(Calibration_state[j][6] | (1ULL << (uint64_t)j)) | RB_ADDR, 0ULL);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_LB);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		RL[j][6].XSB = ADC_4.XSB - ADC_5.XSB;

    		set_FPGA(I_compensate(Calibration_state[j][7]) | RB_ADDR, 0ULL);
    		delay_us(10000);
    		ADC_4.XSB = get_SPI_ADC(ADC_LB);
    		//ADC_4.XSB = get_SPI_ADC(ADC_RX);
    		set_FPGA(I_compensate(Calibration_state[j][7] | (1ULL << (uint64_t)j)) | RB_ADDR, 0ULL);
    		delay_us(10000);
    		ADC_5.XSB = get_SPI_ADC(ADC_LB);
    		//ADC_5.XSB = get_SPI_ADC(ADC_RX);
    		RL[j][7].XSB = ADC_4.XSB - ADC_5.XSB;




    		/*RL[j][0].XSB = (RL[j][0].XSB * 9) / 8;
    		RL[j][1].XSB = (RL[j][1].XSB * 9) / 8;
    		RL[j][2].XSB = (RL[j][2].XSB * 9) / 8;
    		RL[j][3].XSB = (RL[j][3].XSB * 9) / 8;
    		RL[j][4].XSB = (RL[j][4].XSB * 9) / 8;
    		RL[j][5].XSB = (RL[j][5].XSB * 9) / 8;
    		RL[j][6].XSB = (RL[j][6].XSB * 9) / 8;
    		RL[j][7].XSB = (RL[j][7].XSB * 9) / 8;*/
    		RL[j][0].XSB = (RL[j][0].XSB * 11) / 8;
    		RL[j][1].XSB = (RL[j][1].XSB * 11) / 8;
    		RL[j][2].XSB = (RL[j][2].XSB * 11) / 8;
    		RL[j][3].XSB = (RL[j][3].XSB * 11) / 8;
    		RL[j][4].XSB = (RL[j][4].XSB * 11) / 8;
    		RL[j][5].XSB = (RL[j][5].XSB * 11) / 8;
    		RL[j][6].XSB = (RL[j][6].XSB * 11) / 8;
    		RL[j][7].XSB = (RL[j][7].XSB * 11) / 8;



            Tx6_buffer[0] = 0x55;
        	Tx6_buffer[1] = RL[j][0].XSB_0;
        	Tx6_buffer[2] = RL[j][0].XSB_1;
        	Tx6_buffer[3] = RL[j][0].XSB_2;
        	Tx6_buffer[4] = RL[j][1].XSB_0;
        	Tx6_buffer[5] = RL[j][1].XSB_1;
        	Tx6_buffer[6] = RL[j][1].XSB_2;
        	Tx6_buffer[7] = RL[j][2].XSB_0;
        	Tx6_buffer[8] = RL[j][2].XSB_1;
        	Tx6_buffer[9] = RL[j][2].XSB_2;
        	Tx6_buffer[10] = RL[j][3].XSB_0;
        	Tx6_buffer[11] = RL[j][3].XSB_1;
        	Tx6_buffer[12] = RL[j][3].XSB_2;
        	Tx6_buffer[13] = RL[j][4].XSB_0;
        	Tx6_buffer[14] = RL[j][4].XSB_1;
        	Tx6_buffer[15] = RL[j][4].XSB_2;
        	Tx6_buffer[16] = RL[j][5].XSB_0;
        	Tx6_buffer[17] = RL[j][5].XSB_1;
        	Tx6_buffer[18] = RL[j][5].XSB_2;
        	Tx6_buffer[19] = RL[j][6].XSB_0;
        	Tx6_buffer[20] = RL[j][6].XSB_1;
        	Tx6_buffer[21] = RL[j][6].XSB_2;
        	Tx6_buffer[22] = RL[j][7].XSB_0;
        	Tx6_buffer[23] = RL[j][7].XSB_1;
        	Tx6_buffer[24] = RL[j][7].XSB_2;
        	Tx6_buffer[25] = j+50;
        	CRC_.XSB = CalcCRC32(0xFFFFFFFF, (uint8_t*)(Tx6_buffer), 26);
        	Tx6_buffer[26] = CRC_.XSB_0;
        	Tx6_buffer[27] = CRC_.XSB_1;
        	Tx6_buffer[28] = CRC_.XSB_2;
        	Tx6_buffer[29] = CRC_.XSB_3;

        	for (i=0; i<30; i++)
        	    uart6_send(Tx6_buffer[i]);


        	delay_us(1000);
    	    }
#endif
#endif


    	/*FPGA_L_CS_L;
    	SPI_FPGA_L_Wr_Rd(0x00);
    	SPI_FPGA_L_Wr_Rd(0b01011011);  //REG0
    	SPI_FPGA_L_Wr_Rd(0b01111011);  //REG1
    	SPI_FPGA_L_Wr_Rd(0b00011010);  //REG2
    	SPI_FPGA_L_Wr_Rd(0b00011001);  //REG3
    	SPI_FPGA_L_Wr_Rd(0b00011011);  //REG4
    	SPI_FPGA_L_Wr_Rd(0b00000011);  //REG5
    	SPI_FPGA_L_Wr_Rd(0b01000011);  //REG6
    	SPI_FPGA_L_Wr_Rd(0b01000011);  //REG7
    	FPGA_L_CS_H;

    	FPGA_R_CS_L;
    	SPI_FPGA_R_Wr_Rd(0x00);
    	SPI_FPGA_R_Wr_Rd(0b01011011);  //REG0
    	SPI_FPGA_R_Wr_Rd(0b01111011);  //REG1
    	SPI_FPGA_R_Wr_Rd(0b00011010);  //REG2
    	SPI_FPGA_R_Wr_Rd(0b00011001);  //REG3
    	SPI_FPGA_R_Wr_Rd(0b00011011);  //REG4
    	SPI_FPGA_R_Wr_Rd(0b00000011);  //REG5
    	SPI_FPGA_R_Wr_Rd(0b01000011);  //REG6
    	SPI_FPGA_R_Wr_Rd(0b01000011);  //REG7
    	FPGA_R_CS_H;*/


#if Kalibracja == 1
    	ADC_2.XSB = get_SPI_ADC(0xAC);    //MUXP = AIN10 = +3.3, MUXN = AINCOM

    	ADC_6.XSB = get_SPI_ADC(0x6C);    //MUXP = AIN6 = +3.3REF, MUXN = AINCOM

    	ADC_3.XSB = get_SPI_ADC(0xBC);    //MUXP = AIN11 = -3.3, MUXN = AINCOM

    	ADC_1.XSB = get_SPI_ADC(ADC_RX);    //MUXP = AIN1 = ADC_RB, MUXN = AINCOM

    	ADC_4.XSB = get_SPI_ADC(0x4C);    //MUXP = AIN4 = BATT1, MUXN = AINCOM

    	ADC_5.XSB = get_SPI_ADC(0x5C);    //MUXP = AIN5 = BATT2, MUXN = AINCOM


        Tx6_buffer[0] = 0x55;
    	Tx6_buffer[1] = ADC_0.XSB_0;
    	Tx6_buffer[2] = ADC_0.XSB_1;
    	Tx6_buffer[3] = ADC_0.XSB_2;
    	Tx6_buffer[4] = ADC_1.XSB_0;
    	Tx6_buffer[5] = ADC_1.XSB_1;
    	Tx6_buffer[6] = ADC_1.XSB_2;
    	Tx6_buffer[7] = ADC_2.XSB_0;
    	Tx6_buffer[8] = ADC_2.XSB_1;
    	Tx6_buffer[9] = ADC_2.XSB_2;
    	Tx6_buffer[10] = ADC_3.XSB_0;
    	Tx6_buffer[11] = ADC_3.XSB_1;
    	Tx6_buffer[12] = ADC_3.XSB_2;
    	Tx6_buffer[13] = ADC_4.XSB_0;
    	Tx6_buffer[14] = ADC_4.XSB_1;
    	Tx6_buffer[15] = ADC_4.XSB_2;
    	Tx6_buffer[16] = ADC_5.XSB_0;
    	Tx6_buffer[17] = ADC_5.XSB_1;
    	Tx6_buffer[18] = ADC_5.XSB_2;
    	Tx6_buffer[19] = ADC_6.XSB_0;
    	Tx6_buffer[20] = ADC_6.XSB_1;
    	Tx6_buffer[21] = ADC_6.XSB_2;
    	Tx6_buffer[22] = ADC_7.XSB_0;
    	Tx6_buffer[23] = ADC_7.XSB_1;
    	Tx6_buffer[24] = ADC_7.XSB_2;
    	Tx6_buffer[25] = 0;
    	CRC_.XSB = CalcCRC32(0xFFFFFFFF, (uint8_t*)(Tx6_buffer), 26);
    	Tx6_buffer[26] = CRC_.XSB_0;
    	Tx6_buffer[27] = CRC_.XSB_1;
    	Tx6_buffer[28] = CRC_.XSB_2;
    	Tx6_buffer[29] = CRC_.XSB_3;

    	for (i=0; i<30; i++)
    	    uart6_send(Tx6_buffer[i]);


    	__disable_irq();
    	DAC_L_value_to_send = DAC_L_value;
    	DAC_R_value_to_send = DAC_R_value;
    	__enable_irq();

    	DAC_L_value_to_send = DAC_L_value_to_send | RB_ADDR;
    	DAC_R_value_to_send = DAC_R_value_to_send | RB_ADDR;

    	for(i=0; i<42; i++)
            {
            if(DAC_L_value_to_send & 0x20000000000ULL)
            	FPGA_L_DI_H;
            else
            	FPGA_L_DI_L;

            if(DAC_R_value_to_send & 0x20000000000ULL)
            	FPGA_R_DI_H;
            else
            	FPGA_R_DI_L;

            delay_us(100);

            FPGA_R_CCLK2_H;
            FPGA_L_CCLK2_H;

            delay_us(100);

            FPGA_R_CCLK2_L;
            FPGA_L_CCLK2_L;

            delay_us(100);

            DAC_L_value_to_send <<= 1;
            DAC_R_value_to_send <<= 1;
            };

        delay_us(1000);

        FPGA_R_CS_H;
        FPGA_L_CS_H;

        delay_us(100);

        FPGA_R_CS_L;
        FPGA_L_CS_L;

        delay_us(10);
#endif


        if (Sine_generate == 1)
            {
        	Sine_generate = 2;

        	while (1)
        	    {
        		/*for(i=0; i<256; i++)
        	        {
        			//set_FPGA(0ULL, R_value_compensate(1000000+(i*1000)));
        			set_FPGA(0ULL, R_value_compensate(sinus[i*4]));
        	        }*/
        	    }
            }


#if Kalibracja == 2
        Licznik_kalibracji++;

        if (Licznik_kalibracji >= 2)
            {
        	Licznik_kalibracji = 2;

        	set_FPGA(0b1111000000000000000000000000000000ULL | RB_ADDR, 0b1111000000000000000000000000000000ULL | RB_ADDR);

			for (i=8; i<34; i++)
			    {
				RL[i][0].XSB = 0;
				RR[i][0].XSB = 0;

				for (j=1; j<8; j++)
					{
					RL[i][0].XSB += RL[i][j].XSB;
					RR[i][0].XSB += RR[i][j].XSB;
					}

				RL[i][0].XSB = RL[i][0].XSB / 7;
				RR[i][0].XSB = RR[i][0].XSB / 7;
			    }


			for (i=8; i<34; i++)
			    {
				set_FPGA((RL[i][0].XSB | ((RB_x_start_ADDR + (uint64_t)i) << 34)), (RR[i][0].XSB | ((RB_x_start_ADDR + (uint64_t)i) << 34)));
				delay_us(1000);
			    }

			set_FPGA(1ULL | RB_MUX_ADDR, 1ULL | RB_MUX_ADDR);
			delay_us(1000);

			//while (1);
            }
#endif

#if Kalibracja == 3
        RL[16][0].XSB = 2855;
        RL[17][0].XSB = 5495;
        RL[18][0].XSB = 9866;
        RL[19][0].XSB = 16618;
        RL[20][0].XSB = 30542;
        RL[21][0].XSB = 54736;
        RL[22][0].XSB = 98178;
        RL[23][0].XSB = 166600;
		RL[24][0].XSB = 297309;
		RL[25][0].XSB = 547866;
		RL[26][0].XSB = 982537;
		RL[27][0].XSB = 1653315;
		RL[28][0].XSB = 1653315;
		RL[29][0].XSB = 1653315;
		RL[30][0].XSB = 1653315;
		RL[31][0].XSB = 1653315;
		RL[32][0].XSB = 1653315;
		RL[33][0].XSB = 1653315;


        Licznik_kalibracji = 2;

        if (Licznik_kalibracji >= 2)
            {
        	Licznik_kalibracji = 2;

        	set_FPGA(0b1111000000000000000000000000000000ULL | RB_ADDR, 0b1111000000000000000000000000000000ULL | RB_ADDR);


			for (i=8; i<34; i++)
			    {
				set_FPGA((RL[i][0].XSB | ((RB_x_start_ADDR + (uint64_t)i) << 34)), (RL[i][0].XSB | ((RB_x_start_ADDR + (uint64_t)i) << 34)));
				delay_us(1000);
			    }

			set_FPGA(1ULL | RB_MUX_ADDR, 1ULL | RB_MUX_ADDR);
			delay_us(1000);

			while (1);
            }
#endif
    	delay_us(100);
}
//-------------------------------------------------------------------------
static void uart6_send(uint8_t tx_data)
{
    uint8_t i;
}
//-------------------------------------------------------------------------
static uint8_t SPI_ADC_Wr_Rd(uint8_t Data)
{
    uint8_t i;
    uint8_t Data2 = 0;

    delay_us(10);

    for(i=0; i<8; i++)
        {
    	ADC_SCLK_H;

    	delay_us(10);

        if(Data & 0x80)
        	ADC_MOSI_H;
        else
        	ADC_MOSI_L;

        delay_us(10);

        ADC_SCLK_L;

        delay_us(10);

        Data2 <<= 1;

        if (ADC_MISO != 0)
        	Data2 |= 0x01;

        Data <<= 1;

        delay_us(10);
        };

    return Data2;
}
//-------------------------------------------------------------------------
static uint8_t SPI_FPGA_L_Wr_Rd(uint8_t Data)
{
    uint8_t i;
    uint8_t Data2 = 0;

    delay_us(10);

    for(i=0; i<8; i++)
        {
    	FPGA_L_CCLK2_L;

        if(Data & 0x80)
        	FPGA_L_DI_H;
        else
        	FPGA_L_DI_L;

        delay_us(10);

        FPGA_L_CCLK2_H;

        delay_us(10);

        Data2 <<= 1;

        if (FPGA_DO_L != 0)
        	Data2 |= 0x01;

        Data <<= 1;

        delay_us(10);
        };

    return Data2;
}
//-------------------------------------------------------------------------
static uint8_t SPI_FPGA_R_Wr_Rd(uint8_t Data)
{
    uint8_t i;
    uint8_t Data2 = 0;

    delay_us(10);

    for(i=0; i<8; i++)
        {
    	FPGA_R_CCLK2_L;

        if(Data & 0x80)
        	FPGA_R_DI_H;
        else
        	FPGA_R_DI_L;

        delay_us(10);

        FPGA_R_CCLK2_H;

        delay_us(10);

        Data2 <<= 1;

        if (FPGA_DO_R != 0)
        	Data2 |= 0x01;

        Data <<= 1;

        delay_us(10);
        };

    return Data2;
}
//-------------------------------------------------------------------------
static int32_t get_SPI_ADC(uint8_t Channels)
{
	union uint32_t_LSB_MSB ADC_data;

	ADC_CS_L;
	SPI_ADC_Wr_Rd(0x40 + 0x02);    //WREG INPMUX
	SPI_ADC_Wr_Rd(0x00);
	SPI_ADC_Wr_Rd(Channels);
	ADC_CS_H;

	delay_us(10);

	ADC_CS_L;
	SPI_ADC_Wr_Rd(0x08);    //START
	ADC_CS_H;


	//delay_us(700000);
	delay_us(10);
	while (ADC_DRDY != 0);

	ADC_CS_L;
	SPI_ADC_Wr_Rd(0x12);    //RDATA
	ADC_data.XSB_2 = SPI_ADC_Wr_Rd(0x00);
	ADC_data.XSB_1 = SPI_ADC_Wr_Rd(0x00);
	ADC_data.XSB_0 = SPI_ADC_Wr_Rd(0x00);
	ADC_CS_H;


	if ((ADC_data.XSB_2 & 0x80) != 0)
		ADC_data.XSB_3 = 0xFF;
	else
		ADC_data.XSB_3 = 0x00;


	return ADC_data.XSB;
}
//-------------------------------------------------------------------------
static void set_FPGA(uint64_t L, uint64_t R)
{
    int8_t i;
    uint64_t L2;
    uint64_t R2;

    L2 = L;// | 0b0000000000000000000000110000000000ULL;
    R2 = R;// | 0b0000000000000000000000110000000000ULL;

    for(i=0; i<42; i++)
        {
        if(L2 & 0x20000000000ULL)
        	FPGA_L_DI_H;
        else
        	FPGA_L_DI_L;

        if(R2 & 0x20000000000ULL)
        	FPGA_R_DI_H;
        else
        	FPGA_R_DI_L;

        delay_100ns(1);

        FPGA_R_CCLK2_H;
        FPGA_L_CCLK2_H;

        delay_100ns(1);

        FPGA_R_CCLK2_L;
        FPGA_L_CCLK2_L;

        delay_100ns(1);

        L2 <<= 1;
        R2 <<= 1;
        };

    delay_100ns(1);

    FPGA_R_CS_H;
    FPGA_L_CS_H;

    delay_100ns(1);

    FPGA_R_CS_L;
    FPGA_L_CS_L;

    delay_100ns(1);
}
//-------------------------------------------------------------------------
void delay_us(uint32_t delay)
{
    volatile uint32_t i;

    for (i=0; i<delay*18; i++);
}
//-------------------------------------------------------------------------
void delay_100ns(uint32_t delay)
{
    volatile uint32_t i;

    for (i=0; i<delay*2; i++);
}
//-------------------------------------------------------------------------
const uint32_t crc_table[] = {
0x00000000,0x77073096,0xee0e612c,0x990951ba,0x076dc419,0x706af48f,0xe963a535,0x9e6495a3,
0x0edb8832,0x79dcb8a4,0xe0d5e91e,0x97d2d988,0x09b64c2b,0x7eb17cbd,0xe7b82d07,0x90bf1d91,
0x1db71064,0x6ab020f2,0xf3b97148,0x84be41de,0x1adad47d,0x6ddde4eb,0xf4d4b551,0x83d385c7,
0x136c9856,0x646ba8c0,0xfd62f97a,0x8a65c9ec,0x14015c4f,0x63066cd9,0xfa0f3d63,0x8d080df5,
0x3b6e20c8,0x4c69105e,0xd56041e4,0xa2677172,0x3c03e4d1,0x4b04d447,0xd20d85fd,0xa50ab56b,
0x35b5a8fa,0x42b2986c,0xdbbbc9d6,0xacbcf940,0x32d86ce3,0x45df5c75,0xdcd60dcf,0xabd13d59,
0x26d930ac,0x51de003a,0xc8d75180,0xbfd06116,0x21b4f4b5,0x56b3c423,0xcfba9599,0xb8bda50f,
0x2802b89e,0x5f058808,0xc60cd9b2,0xb10be924,0x2f6f7c87,0x58684c11,0xc1611dab,0xb6662d3d,
0x76dc4190,0x01db7106,0x98d220bc,0xefd5102a,0x71b18589,0x06b6b51f,0x9fbfe4a5,0xe8b8d433,
0x7807c9a2,0x0f00f934,0x9609a88e,0xe10e9818,0x7f6a0dbb,0x086d3d2d,0x91646c97,0xe6635c01,
0x6b6b51f4,0x1c6c6162,0x856530d8,0xf262004e,0x6c0695ed,0x1b01a57b,0x8208f4c1,0xf50fc457,
0x65b0d9c6,0x12b7e950,0x8bbeb8ea,0xfcb9887c,0x62dd1ddf,0x15da2d49,0x8cd37cf3,0xfbd44c65,
0x4db26158,0x3ab551ce,0xa3bc0074,0xd4bb30e2,0x4adfa541,0x3dd895d7,0xa4d1c46d,0xd3d6f4fb,
0x4369e96a,0x346ed9fc,0xad678846,0xda60b8d0,0x44042d73,0x33031de5,0xaa0a4c5f,0xdd0d7cc9,
0x5005713c,0x270241aa,0xbe0b1010,0xc90c2086,0x5768b525,0x206f85b3,0xb966d409,0xce61e49f,
0x5edef90e,0x29d9c998,0xb0d09822,0xc7d7a8b4,0x59b33d17,0x2eb40d81,0xb7bd5c3b,0xc0ba6cad,
0xedb88320,0x9abfb3b6,0x03b6e20c,0x74b1d29a,0xead54739,0x9dd277af,0x04db2615,0x73dc1683,
0xe3630b12,0x94643b84,0x0d6d6a3e,0x7a6a5aa8,0xe40ecf0b,0x9309ff9d,0x0a00ae27,0x7d079eb1,
0xf00f9344,0x8708a3d2,0x1e01f268,0x6906c2fe,0xf762575d,0x806567cb,0x196c3671,0x6e6b06e7,
0xfed41b76,0x89d32be0,0x10da7a5a,0x67dd4acc,0xf9b9df6f,0x8ebeeff9,0x17b7be43,0x60b08ed5,
0xd6d6a3e8,0xa1d1937e,0x38d8c2c4,0x4fdff252,0xd1bb67f1,0xa6bc5767,0x3fb506dd,0x48b2364b,
0xd80d2bda,0xaf0a1b4c,0x36034af6,0x41047a60,0xdf60efc3,0xa867df55,0x316e8eef,0x4669be79,
0xcb61b38c,0xbc66831a,0x256fd2a0,0x5268e236,0xcc0c7795,0xbb0b4703,0x220216b9,0x5505262f,
0xc5ba3bbe,0xb2bd0b28,0x2bb45a92,0x5cb36a04,0xc2d7ffa7,0xb5d0cf31,0x2cd99e8b,0x5bdeae1d,
0x9b64c2b0,0xec63f226,0x756aa39c,0x026d930a,0x9c0906a9,0xeb0e363f,0x72076785,0x05005713,
0x95bf4a82,0xe2b87a14,0x7bb12bae,0x0cb61b38,0x92d28e9b,0xe5d5be0d,0x7cdcefb7,0x0bdbdf21,
0x86d3d2d4,0xf1d4e242,0x68ddb3f8,0x1fda836e,0x81be16cd,0xf6b9265b,0x6fb077e1,0x18b74777,
0x88085ae6,0xff0f6a70,0x66063bca,0x11010b5c,0x8f659eff,0xf862ae69,0x616bffd3,0x166ccf45,
0xa00ae278,0xd70dd2ee,0x4e048354,0x3903b3c2,0xa7672661,0xd06016f7,0x4969474d,0x3e6e77db,
0xaed16a4a,0xd9d65adc,0x40df0b66,0x37d83bf0,0xa9bcae53,0xdebb9ec5,0x47b2cf7f,0x30b5ffe9,
0xbdbdf21c,0xcabac28a,0x53b39330,0x24b4a3a6,0xbad03605,0xcdd70693,0x54de5729,0x23d967bf,
0xb3667a2e,0xc4614ab8,0x5d681b02,0x2a6f2b94,0xb40bbe37,0xc30c8ea1,0x5a05df1b,0x2d02ef8d
};

static uint32_t CalcCRC32(uint32_t crc, uint8_t *buffer, uint8_t length)
{
    uint16_t i;

    for (i=0; i<length; i++)
        {
        crc = (crc >> 8) ^ crc_table[*buffer++ ^ (crc & 0xFF)];
        }

    return crc;
}
//-------------------------------------------------------------------------
static uint64_t R_value_compensate(int32_t data)
{
    uint16_t i;
    int32_t Signal_value;
    uint64_t R_value;
    uint8_t R_in_use;

    R_in_use = 3;

    R_value = 0;


    /*if (data > (1875000*1))
    	R_in_use = 1;
    if (data > (1875000*2))
    	R_in_use = 2;
    if (data > (1875000*3))
    	R_in_use = 3;*/

/*    if (R_in_use == 1)
    	data += 20;
    if (R_in_use == 2)
    	data += 20;
    if (R_in_use == 3)
    	data += 20;*/


    for (i=33; i>8; i--)
        {
    	if (data > (Signal_value + RR[i][R_in_use].XSB))
    	    {
    		R_value |= (1ULL << (uint64_t)i);

    		Signal_value += RR[i][R_in_use].XSB;

    		if (i > 26)
    		    {
    		    //R_in_use++;
    		    //R_value++;
    		    }
    	    }
        }


    return R_value;
}
//-------------------------------------------------------------------------
static uint64_t I_compensate(uint64_t data)
{
	uint16_t i;
	uint16_t j;
	uint64_t data_out;

	i = 0;
	j = 0;

	data_out = data;


	if (data & 0b1000000000000000000000000000000000ULL)
		i++;
	if (data & 0b0100000000000000000000000000000000ULL)
		i++;

	if (data & 0b0010000000000000000000000000000000ULL)
		j++;
	if (data & 0b0001000000000000000000000000000000ULL)
		j++;
	if (data & 0b0000100000000000000000000000000000ULL)
		j++;
	if (data & 0b0000010000000000000000000000000000ULL)
		j++;
	if (data & 0b0000001000000000000000000000000000ULL)
		j++;


	if (i == 0)
		data_out |= 0b0000000000000000000000000000000010ULL;
	if (i == 1)
		data_out |= 0b0000000000000000000000000000100000ULL;

	if (j == 0)
		data_out |= 0b0000000000000000000000000000000101ULL;
	if (j == 1)
		data_out |= 0b0000000000000000000000000000000100ULL;
	if (j == 2)
		data_out |= 0b0000000000000000000000000000010001ULL;
	if (j == 3)
		data_out |= 0b0000000000000000000000000000010000ULL;
	if (j == 4)
		data_out |= 0b0000000000000000000000000000000001ULL;

	return data_out;
}
//-------------------------------------------------------------------------
