/*
 * adc_rza2m_example.c
 *
 * This is an example of reading an ADC from application space by directly
 * accessing the ADC registers without a kernel driver.
 *
 * It reads AN006, which on the RZ/A2M EVB board is connected to Switch 4
 * and Switch 5 on sub-board.
 */

/* Include <System include>, "Project include" */
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

/* Base address */
#define ADC_REG_BASE		0xE8005000
#define ADC_REG_BASE_SIZE	0x120

/* ADC Registers */
#define ADC_ADCSR		0x800	/* A/D Control Register */
#define ADC_ADANSA0		0x804	/* A/D Channel Select Register A0 */
#define ADC_ADADS0		0x808	/* A/D-Converted Value Addition/Average Mode Select Register 0 */
#define ADC_ADADC		0x80C	/* A/D-Converted Value Addition/Average Count Select Register */
#define ADC_ADCER		0x80E	/* A/D Control Extended Register */
#define ADC_ADSTRGR		0x810	/* A/D Start Trigger Select Register */
#define ADC_ADANSB0		0x814	/* A/D Channel Select Register B0 */
#define ADC_ADDBLDR		0x818	/* A/D Data Duplication Register */
#define ADC_ADRD		0x81E	/* A/D Self-Diagnosis Data Register */
#define ADC_ADDR0		0x820	/* A/D Data Register 0 */
#define ADC_ADDR1		0x822	/* A/D Data Register 1 */
#define ADC_ADDR2		0x824	/* A/D Data Register 2 */
#define ADC_ADDR3		0x826	/* A/D Data Register 3 */
#define ADC_ADDR4		0x828	/* A/D Data Register 4 */
#define ADC_ADDR5		0x82A	/* A/D Data Register 5 */
#define ADC_ADDR6		0x82C	/* A/D Data Register 6 */
#define ADD_ADDR7		0x82E	/* A/D Data Register 7 */
#define ADC_ADDISCR		0x87A	/* A/D Disconnection Detection Control Register */
#define ADC_ADGSPCR		0x880	/* A/D Group Scan Priority Control Register */
#define ADC_ADDBLDRA	0x884	/* A/D Data Duplication Register A */
#define ADC_ADDBLDRB	0x886	/* A/D Data Duplication Register B */
#define ADC_ADWINMON	0x88C	/* A/D Compare Function AB Status Monitor Register */
#define ADC_ADCMPCR		0x890	/* A/D Compare Control Register */
#define ADC_ADCMPANSR0	0x894	/* A/D Compare Function Window-A Channel Selection Register 0 */
#define ADC_ADCMPLR0	0x898	/* A/D Compare Function Window-A Comparison Condition Setting Register 0 */
#define ADC_ADCMPDR0	0x89C	/* A/D Compare Function Window-A Lower Level Setting Register */
#define ADC_ADCMPDR1	0x89E	/* A/D Compare Function Window-A Upper Level Setting Register */
#define ADC_ADCMPSR0	0x8A0	/* A/D Compare Function Window-A Channel Status Register 0 */
#define ADC_ADCMPBNSR	0x8A6	/* A/D Compare Function Window-B Channel Selection Register */
#define ADC_ADWINLLB	0x8A8	/* A/D Compare Function Window-B Lower Level Setting Register */
#define ADC_ADWINULB	0x8AA	/* A/D Compare Function Window-B Upper Level Setting Register */
#define ADC_ADCMPBSR	0x8AC	/* A/D Compare Function Window-B Status Register */
#define ADC_ADANSC0		0x8D4	/* A/D Channel Select Register C0 */
#define ADC_ADGCTRGR	0x8D9	/* A/D Group C Trigger Select Register */
#define ADC_ADSSTR0		0x8E0	/* A/D Sampling State Register 0 */
#define ADC_ADSSTR1		0x8E1	/* A/D Sampling State Register 1 */
#define ADC_ADSSTR2		0x8E2	/* A/D Sampling State Register 2 */
#define ADC_ADSSTR3		0x8E3	/* A/D Sampling State Register 3 */
#define ADC_ADSSTR4		0x8E4	/* A/D Sampling State Register 4 */
#define ADC_ADSSTR5		0x8E5	/* A/D Sampling State Register 5 */
#define ADC_ADSSTR6		0x8E6	/* A/D Sampling State Register 6 */
#define ADC_ADSSTR7		0x8E7	/* A/D Sampling State Register 7 */

/* A/D Control Register - ADCSR */
#define ADC_ADCSR_ADST_EN	0x8000	/* A/D Conversion Start - Enable */
#define ADC_ADCSR_ADCS_SM	0x0000	/* Single scan mode */
#define ADC_ADCSR_ADCS_GM	0x2000	/* Group scan mode */
#define ADC_ADCSR_ADCS_CM	0x4000	/* Continuous scan mode */
#define ADC_ADCSR_ADIE		0x1000	/* Scan End Interrupt Enable */
#define ADC_ADCSR_TRGE		0x0200	/* Trigger Start Enable */
#define ADC_ADCSR_EXTRG		0x0100	/* Trigger Select */
#define ADC_ADCRS_DBLE		0x0080	/* Double Trigger Mode Select */
#define ADC_ADCSR_GBADIE	0x0040	/* Group B Scan End Interrupt Enable */
//#define ADC_ADCSR_DBLANS	0x0000	/* A/D Conversion Data Duplication Channel Select Double Trigger Channel Select */

/* A/D Control Extended Register - ADCER */
#define ADC_ADCER_ADPRC_RESOLUTION12	0x0 /* A/D conversion is performed with 12-bit accuracy */
#define ADC_ADCER_ADPRC_RESOLUTION10	0x2	/* A/D conversion is performed with 10-bit accuracy */
#define ADC_ADCER_ADPRC_RESOLUTION8		0x4	/* A/D conversion is performed with 8-bit accuracy */
#define ADC_ADCER_ADRFMT_LEFT 			0x8000	/* 0: Right-alignment is selected for the A/D data register format *
												 * 1: Left-alignment is selected for the A/D data register format  */

/* ADC channel */
#define ADC_NUM_CHANNEL	8
#define ADC_CH_AN0		0x00	/* AN000 */
#define ADC_CH_AN1		0x02	/* AN001 */
#define ADC_CH_AN2		0x04	/* AN002 */
#define ADC_CH_AN3		0x08	/* AN003 */
#define ADC_CH_AN4		0x10	/* AN004 */
#define ADC_CH_AN5		0x20	/* AN005 */
#define ADC_CH_AN6		0x40	/* AN006 */
#define ADC_CH_AN7		0x80	/* AN007 */

/* For switch */
#define MAIN_PRV_SW4        (1u)
#define MAIN_PRV_SW5        (2u)
#define MAIN_PRV_NO_SWITCH  (0u)

/* Globals Variable*/
void *adc_base_addr;	/* base address of ADC */
int adc_fd;
int continuous_stream = 0;

/* Functions */
static void adc_write8(uint8_t data, int reg)
{
	*(uint8_t *)(adc_base_addr + reg) = data;
}

static uint8_t adc_read8(int reg)
{
	return *(uint8_t *)(adc_base_addr + reg);
}

static void adc_write16(uint16_t data, int reg)
{
	*(uint16_t *)(adc_base_addr + reg) = data;
}

static uint16_t adc_read16(int reg)
{
	return *(uint16_t *)(adc_base_addr + reg);
}

static int kbhit()	/* detect a press on the keyboard */
{
	struct timeval tv;
	fd_set fds;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);	//STDIN_FILENO is 0
	select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
	return FD_ISSET(STDIN_FILENO, &fds);
}

static int map_adc_value_to_switches (uint16_t channel_6_adc)
{
    /* SW4  SW5  Nominal Value   Midpoint
       Off  Off  0xFFF
                                 0xBFF
       Off  On   0x7FF
                                 0x5EF
       On   Off  0x3DF
                                 0x37F
       On   On   0x31F
    */

    if (channel_6_adc > 0xBFF)
    {
        return MAIN_PRV_NO_SWITCH;
    }

    if (channel_6_adc < 0x37F)
    {
        return MAIN_PRV_SW4 | MAIN_PRV_SW5;
    }

    if ((channel_6_adc > 0x5EF))
    {
        return MAIN_PRV_SW5;
    }

    return MAIN_PRV_SW4;
}

static void adc_init(void)
{
	/* Setting Resolution */
	adc_write16(ADC_ADCER_ADPRC_RESOLUTION12, ADC_ADCER); /* resolution 12 */
	
	/* Setting Alignment */
	adc_write16(ADC_ADCER_ADRFMT_LEFT, ADC_ADCER);	/* left alignment */
	
	/* Setting Scan Mode */
	adc_write16(ADC_ADCSR_ADCS_SM, ADC_ADCSR);	/* single mode */
	
	/* Setting Channel */
	adc_write16(ADC_CH_AN6, ADC_ADANSA0);	/* channel 6 */
	
	/* Setting Sample Time */
	adc_write8(0x32, ADC_ADSSTR6);	/* 50 */
}

static void adc_trigger(void)
{
	/* Enable a conversion */
	adc_write16(ADC_ADCSR_ADST_EN, ADC_ADCSR);
}

static uint16_t read_adc(void)
{
	return adc_read16(ADC_ADDR6) >> 4;
}

static int adc_open(void)
{
	adc_fd = open("/dev/mem", O_RDWR);
	if (adc_fd < 0) {
		perror("can't open /dev/mem");
		return -1;
	}
	
	adc_base_addr = mmap(NULL,
						ADC_REG_BASE_SIZE,		/* memory size */
						PROT_READ | PROT_WRITE,
						MAP_SHARED,
						adc_fd,					/* file description */
						ADC_REG_BASE);			/* ADC base address */
						
	if (adc_base_addr == MAP_FAILED) {
		perror("MAP_FAILED");
		return -1;
	}
	
	return 0;
}

static int adc_close(void)
{
	if (adc_base_addr)
		munmap(adc_base_addr, ADC_REG_BASE);

	if (adc_fd)
		close(adc_fd);
}

int main(int argc, char *argv[])
{
	const char *p_switches_pressed_string[] = {
		"No switch pressed  ",
	 	"SW4 pressed        ", 
	 	"SW5 pressed        ", 
	 	"SW4 and SW5 pressed"
	};

	int i;
	int count = 0;
	uint16_t adc_value;
	int switches_pressed;

	/*
	 * This application assumes that:
	 * - The ADC pin has already been configured.
	 *   This was already done in board-rza2mevb.c in the BSP
	 * - The Module Stop bit for the ADC (MSTP57) has been cleared to 0
	 *   This was done by the u-boot in the BSP already.
	 */

	if(argc < 2)
	{
		printf(
		"adc_rza2m_example: Read ADC and display.\n"\
		"Usage: adc_rza2m_example [-c] [count]  \n"\
		"       -c: Continuous mode. \n"\
		"    count: how many samples to take\n"\
		);
		return -1;
	}
	if(adc_open())
		goto apperr;

	for(i = 1; i < argc; i++)
	{
		if( !strcmp(argv[i],"-c") )
		{
			continuous_stream = 1;
		}
		else
		{
			// Must be a count
			sscanf(argv[i], "%u", &count);
		}
	}

	if(continuous_stream)
	{
		printf("***********************************************\n");
		printf("Continuous Stream Mode.\n");
		printf("Press ENTER at any time to exit.\n");
		printf("***********************************************\n");
		setbuf(stdout, NULL);	/* disable stdout buffering */
	}

	setbuf(stdout, NULL);	/* disable stdout buffering */

	adc_init();

	adc_trigger();

	printf ("AN006 = 0x                       ", read_adc());
	while(count | continuous_stream)
	{
		adc_trigger();

		adc_value = read_adc();

		switches_pressed = map_adc_value_to_switches(adc_value);

		/* To overlap the string */
		for (i = 0; i < 23; i++)
			printf("\b");

        printf("%X %s", adc_value, p_switches_pressed_string[switches_pressed]);

		if (--count)
			usleep(100000);	/* 100ms */

		if(kbhit())
			break;
	}
	printf ("\n");

	adc_close();
	return 0;

	apperr:
	printf("Done with errors\n");
	adc_close();
	return 1;
}