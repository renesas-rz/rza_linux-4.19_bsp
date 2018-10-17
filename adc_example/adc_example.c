/*
 * adc_example.c
 *
 * This is an example of reading an ADC from application space by directly
 * accessing the ADC registers without a kernel driver.
 *
 * It reads AD7, which on the RZ/A1H RSK board is connected to potentiometer
 * labeled "RV1" next to the push buttons.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/time.h>

/* globals */
int adc_fd;
void *adc_base;		/* base address for remapped ADC register */
int continuous_stream = 0;

#define ADC_REG_BASE		0xE8005000	/* Rounded down to 4K PAGE boundary */
#define ADC_REG_BASE_SIZE	0x70

#define ADC_ADDR_MASK	0xffc0
#define ADC_ADDR_SHIFT	6

/* ADC Registers */
#define ADC_ADDRA	0x800	/* A/D data register A */
#define ADC_ADDRB	0x802	/* A/D data register B */
#define ADC_ADDRC	0x804	/* A/D data register C */
#define ADC_ADDRD	0x806	/* A/D data register D */
#define ADC_ADDRE	0x808	/* A/D data register E */
#define ADC_ADDRF	0x80a	/* A/D data register F */
#define ADC_ADDRG	0x80c	/* A/D data register G */
#define ADC_ADDRH	0x80e	/* A/D data register H */

#define ADC_ADCSR	0x860	/* A/D control/status register */
#define ADCMPER		0x862	/* A/D comparison interrupt enable register */
#define ADCMPSR		0x864	/* A/D comparison status register */

#define ADCMPHA		0x820	/* A/D comparison upper limit value register A */
#define ADCMPLA		0x822	/* A/D comparison lower limit value register A */


/* ADCSR */
#define ADC_ADCSR_ADF		0x8000
#define ADC_ADCSR_ADIE		0x4000
#define ADC_ADCSR_ADST		0x2000
#define ADC_ADCSR_TRGS_MASK	0x1e00
#define ADC_ADCSR_TRGS_NON	0x0000	/* external trigger input is disable */
#define ADC_ADCSR_TRGS_TRGAN	0x0200	/* MTU2, TRGAN */
#define ADC_ADCSR_TRGS_TRG0N	0x0400	/* MTU2, TRG0N */
#define ADC_ADCSR_TRGS_TRG4AN	0x0600	/* MTU2, TRG4AN */
#define ADC_ADCSR_TRGS_TRG4BN	0x0800	/* MTU2, TRG4BN */
#define ADC_ADCSR_TRGS_EXT	0x1200	/* external pin trigger */
#define ADC_ADCSR_CKS_MASK	0x01c0
#define ADC_ADCSR_CKS_256	0x0000
#define ADC_ADCSR_CKS_298	0x0040
#define ADC_ADCSR_CKS_340	0x0080
#define ADC_ADCSR_CKS_382	0x00c0
#define ADC_ADCSR_MDS_MASK	0x0038
#define ADC_ADCSR_MDS_SINGLE	0x0000
#define ADC_ADCSR_MDS_M_1_4	0x0020	/* multi mode, channel 1 to 4 */
#define ADC_ADCSR_MDS_M_1_8	0x0028	/* multi mode, channel 1 to 8 */
#define ADC_ADCSR_MDS_S_1_4	0x0030	/* scan mode, channel 1 to 4 */
#define ADC_ADCSR_MDS_S_1_8	0x0038	/* scan mode, channel 1 to 8 */
#define ADC_ADCSR_CH_MASK	0x0003
#define ADC_ADCSR_CH_AN0	0x0000
#define ADC_ADCSR_CH_AN1	0x0001
#define ADC_ADCSR_CH_AN2	0x0002
#define ADC_ADCSR_CH_AN3	0x0003
#define ADC_ADCSR_CH_AN4	0x0004
#define ADC_ADCSR_CH_AN5	0x0005
#define ADC_ADCSR_CH_AN6	0x0006
#define ADC_ADCSR_CH_AN7	0x0007

#define ADC_NUM_CHANNEL	8
#define adc_data(ch)	(ADC_ADDRA + ch * 2)
#define adc_compare_upper(ch)	(ADCMPHA + ch * 4)
#define adc_compare_lower(ch)	(ADCMPLA + ch * 4)


static void adc_write(uint16_t data, int reg)
{
	*(uint16_t *)(adc_base + reg) = data;
}

static uint16_t adc_read(int reg)
{
	return *(uint16_t *)(adc_base + reg);
}

static void adc_clear_bit(uint16_t val, int reg)
{
	uint16_t tmp;

	tmp = adc_read(reg);
	tmp &= ~val;
	adc_write(tmp, reg);
}

static void adc_set_bit(uint16_t val, int reg)
{
	uint16_t tmp;

	tmp = adc_read(reg);
	tmp |= val;
	adc_write(tmp, reg);
}

static void adc_start_adc(int channel)
{
	uint16_t mds_ch = 0;

	/* single scan */
	mds_ch = (channel | ADC_ADCSR_MDS_SINGLE);

	/* multi scan */
	//mds_ch |= (cnt > 4 ? ADC_ADCSR_MDS_M_1_8 :
	//	   ADC_ADCSR_MDS_M_1_4);
	//adc_clear_bit(ADC_ADCSR_CH_MASK, ADC_ADCSR);
	//adc_set_bit(mds_ch, ADC_ADCSR);

	/* Stop current ADC conversion */
	adc_clear_bit(ADC_ADCSR_ADST, ADC_ADCSR);

	/* Set the A/D conversion time (slowest) */
	adc_set_bit(ADC_ADCSR_CKS_382, ADC_ADCSR);

	if (channel >= 0)
	{
		adc_clear_bit(ADC_ADCSR_TRGS_MASK, ADC_ADCSR);
		adc_clear_bit(ADC_ADCSR_MDS_MASK, ADC_ADCSR);	/* Single Mode */

		adc_clear_bit(ADC_ADCSR_CH_MASK, ADC_ADCSR);
		adc_set_bit((ADC_ADCSR_ADST | channel),	ADC_ADCSR);	/* select channel and start */
	}
	else
	{
		adc_clear_bit(ADC_ADCSR_TRGS_MASK, ADC_ADCSR);
		adc_set_bit(ADC_ADCSR_TRGS_TRGAN, ADC_ADCSR);
		//start_timer()
	}
}

uint16_t read_adc(int channel)
{
	struct timeval tv;
	struct timespec req;
	int timeout = 1000;	/* 1000 x 1us = 1ms */
	uint16_t adcsr;

	req.tv_sec = 0;
	req.tv_nsec = 1000;	/* 1us */

	adc_start_adc(channel);

	/* wait for conversation to be done */
	while (1)
	{
		/* wait for ADF=1 and ADST=0 */
		adcsr = adc_read(ADC_ADCSR);
		if((adcsr & (ADC_ADCSR_ADF | ADC_ADCSR_ADST)) == ADC_ADCSR_ADF)
			break;

		nanosleep(&req, NULL);
		if ( --timeout == 0 )
		{
			printf("timeout waiting for ADC\n");
			return -1;
		}
	}

	/* Clear A/D End Flag */
	adc_clear_bit(ADC_ADCSR_ADF, ADC_ADCSR);

	/* Read the sampled value */
	/* lower 4 bits are always 0 */
	return adc_read(adc_data(channel)) >> 4;
}

/* detect a press on the keyboard */
int kbhit()
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

int open_resources(void)
{
	/* Get file descriptor of /dev/mem */
	adc_fd = open("/dev/mem", O_RDWR);
	if (adc_fd < 0) {
		perror("Can't open /dev/mem");
		return -1;
	}

	/* Map registers into accessible application space. */
	/* When using mmap, you must provide a physical address that is
	 * on PAGE size (4KB) boundary. */

	adc_base = mmap(NULL,
		ADC_REG_BASE_SIZE,	/* amount to map */
		PROT_READ | PROT_WRITE,
		MAP_SHARED,
		adc_fd,		/* file descriptor for /dev/mem */
		ADC_REG_BASE );	/* physical address of register base */


	if( adc_base == MAP_FAILED )
	{
		perror("ERROR: mmap of adc registers failed.");
		return -1;
	}

	//printf("adc_base=%p\n", adc_base);

	return 0;
}

int close_resources(void)
{
	if(adc_base)
		munmap(adc_base, ADC_REG_BASE_SIZE);
	if(adc_fd)
		close(adc_fd);
}


int main(int argc, char *argv[])
{
	int i;
	int count = 0;

	/*
	 * This application assumes that:
	 * - The ADC pin has already been configured.
	 *   This was already done in board-rskrza1.c in the BSP
	 * - The Module Stop bit for the ADC (MSTP67) has been cleared to 0
	 *   This was done by the u-boot in the BSP already.
	 */

	if( argc < 2)
	{
		printf(
		"adc_example: Read ADC and display.\n"\
		"Usage: adc_example [-c] [count]  \n"\
		"       -c: Continuous mode. \n"\
		"    count: how many samples to take\n"\
		);
		return -1;
	}
	if( open_resources() )
		goto apperr;

	i = 1;
	for( i=1; i < argc; i++)
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

	if( continuous_stream )
	{
		printf("***********************************************\n");
		printf("Continuous Stream Mode.\n");
		printf("Press ENTER at any time to exit.\n");
		printf("***********************************************\n");
		setbuf(stdout, NULL);	/* disable stdout buffering */
	}

	setbuf(stdout, NULL);	/* disable stdout buffering */

	printf ("AD7 = 0x    ", read_adc(7));
	while(count | continuous_stream)
	{
		printf ("\b\b\b\b%04X", read_adc(7));
		if (--count )
			usleep(100000);	/* 100ms */

		if(kbhit())
			break;

	}
	printf ("\n");

	close_resources();
	return 0;

	apperr:
	printf("Done with errors\n");
	close_resources();
	return 1;
}

