/*
 * gpio_example.c
 *
 * This is an example of access GPIO from a user application using the
 * kernel's GPIO interface.
 *
 * This example runs on the RZ/A1H RSK board.
 *
 * Buttons SW1, SW2 and SW3 are used as GPIO input, and LCD0 is used
 * as GPIO output.
 *
 * The ADC connected to "RV1" on the RSK board is also displayed.
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

/* globals */
int continuous_stream = 0;

/* ADC driver */
uint16_t read_adc(int channel);
int adc_open(void);
int adc_close(void);

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
	if (adc_open() != 0 )
	{
		perror("ERROR: ADC open failed.");
		return -1;
	}
	return 0;
}

int close_resources(void)
{
	if (adc_close() != 0 )
	{
		perror("ERROR: ADC close failed.");
		return -1;
	}
}


int main(int argc, char *argv[])
{
	int i;
	int count = 0;
	int led0_fd, sw1_fd, sw2_fd, sw3_fd;
	int sw1, sw2, sw3;
	char value_str[3];
	int adc;

	/*
	 * This application assumes that:
	 * - The ADC pin has already been configured.
	 *   This was already done in board-rskrza1.c in the BSP
	 * - The Module Stop bit for the ADC (MSTP67) has been cleared to 0
	 *   This was done by the u-boot in the BSP already.
	 */

	if (argc < 2)
	{
		printf(
		"gpio_example: Read buttons and ADC and display their values.\n"\
		"Usage: gpio_example [-c] [count]  \n"\
		"       -c: Continuous mode. \n"\
		"    count: how many samples to take\n"\
		);
		return -1;
	}

	/* Read in command line options */
	i = 1;
	for (i=1; i < argc; i++)
	{
		if (!strcmp(argv[i],"-c") )
		{
			continuous_stream = 1;
		}
		else
		{
			// Must be a count
			sscanf(argv[i], "%u", &count);
		}
	}

	setbuf(stdout, NULL);	/* disable stdout buffering */

	if( open_resources() )
		goto apperr;

	/*
	 * On the RSK board
	 *  LED0 = P7_1 (GPIO out)
	 *  SW1 = P1_9  (GPIO in)
	 *  SW2 = P1_8  (GPIO in)
	 *  SW3 = P1_11 (GPIO in)
	 */
	/* Allocate pins */
	system("echo 113 > /sys/class/gpio/export");	// LED0 = P7_1
	system("echo out > /sys/class/gpio/P7_1/direction");
	system("echo 25 > /sys/class/gpio/export");	// SW1 = P1_9
	system("echo in > /sys/class/gpio/P1_9/direction");
	system("echo 24 > /sys/class/gpio/export");	//SW2 = P1_8
	system("echo in > /sys/class/gpio/P1_8/direction");
	system("echo 27 > /sys/class/gpio/export");	//SW3 = P1_11
	system("echo in > /sys/class/gpio/P1_11/direction");

	/* Get file pointers */
	led0_fd = open("/sys/class/gpio/P7_1/value", O_WRONLY);
	sw1_fd = open("/sys/class/gpio/P1_9/value", O_RDONLY);
	sw2_fd = open("/sys/class/gpio/P1_8/value", O_RDONLY);
	sw3_fd = open("/sys/class/gpio/P1_11/value", O_RDONLY);

	if (continuous_stream)
	{
		printf("***********************************************\n");
		printf("Continuous Stream Mode.\n");
		printf("Press ENTER at any time to exit.\n");
		printf("***********************************************\n");
		setbuf(stdout, NULL);	/* disable stdout buffering */
	}

	printf ("AD7 = 0x     0 0 0", read_adc(7));
	while (count | continuous_stream)
	{
		/* Read current button values */
		/* Set file pointer back to begining of the driver file to avoid close/reopen. */
		lseek(sw1_fd, 0, SEEK_SET);
		read(sw1_fd, value_str, 3);
		sw1 = atoi(value_str);

		lseek(sw2_fd, 0, SEEK_SET);
		read(sw2_fd, value_str, 3);
		sw2 = atoi(value_str);

		lseek(sw3_fd, 0, SEEK_SET);
		read(sw3_fd, value_str, 3);
		sw3 = atoi(value_str);

		/* Turn on LED0 if SW1 is down */
		if (sw1)
			system("echo 1 > /sys/class/gpio/P7_1/value");
		else
			system("echo 0 > /sys/class/gpio/P7_1/value");

		/* read ADC */
		adc = read_adc(7);

		printf ("\b\b\b\b\b\b\b\b\b\b%04X %d %d %d", adc, sw1, sw2, sw3);

		if (--count)
			usleep(100000);	/* 100ms */

		if(kbhit())
			break;
	}
	printf ("\n");

	close_resources();

	/* Un-allocate pins */
	system("echo 113 > /sys/class/gpio/unexport");	// LED0 = P7_1
	system("echo 25 > /sys/class/gpio/unexport");	// SW1 = P1_9
	system("echo 24 > /sys/class/gpio/unexport");	//SW2 = P1_8
	system("echo 27 > /sys/class/gpio/unexport");	//SW3 = P1_11

	close(led0_fd);
	close(sw1_fd);
	close(sw2_fd);
	close(sw3_fd);
	return 0;

	apperr:
	printf("Done with errors\n");
	close_resources();
	return 1;
}

