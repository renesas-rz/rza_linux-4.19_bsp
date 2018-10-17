#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>


/* PWM Control Registers */
#define PWM_REG_BASE		0xFCFF5000	/* Rounded down to 4K PAGE boundary */
#define PWM_REG_BASE_SIZE	0x100		/* Amount to map */

/* Register offsets from base address */
#define PWCR_1		0xE0 /* 8-bit */
#define PWPR_1		0xE4 /* 8-bit */
#define PWCYR_1		0xE6 /* 16-bit */
#define PWBFR_1A	0xE8 /* 16-bit */
#define PWBFR_1C	0xEA /* 16-bit */
#define PWBFR_1E	0xEC /* 16-bit */
#define PWBFR_1G	0xEE /* 16-bit */
#define PWCR_2		0xF0 /* 8-bit */
#define PWPR_2		0xF4 /* 8-bit */
#define PWCYR_2		0xF6 /* 16-bit */
#define PWBFR_2A	0xF8 /* 16-bit */
#define PWBFR_2C	0xFA /* 16-bit */
#define PWBFR_2E	0xFC /* 16-bit */
#define PWBFR_2G	0xFE /* 16-bit */
#define PWBTCR		0x06 /* 8-bit */

int main(int argc, char *argv[])
{
	int reg_fd;
	void *base;
	uint8_t value;


	/* Get file descriptor of /dev/mem */
	/* The /dev/mem interface allows you to access any physical address
	 * from within your user application */
	reg_fd = open("/dev/mem", O_RDWR);
	if (reg_fd < 0) {
		perror("Can't open /dev/mem");
		return -1;
	}

	/* Map registers into accessible application space. */
	/* When using mmap, you must provide a physical address that is
	 * on PAGE size (4KB) boundary. */

	base = mmap(NULL,
		PWM_REG_BASE_SIZE,	/* amount to map */
		PROT_READ | PROT_WRITE,
		MAP_SHARED,
		reg_fd,		/* file descriptor for /dev/mem */
		PWM_REG_BASE );	/* physical address of register base */

	if( base == MAP_FAILED )
	{
		perror("ERROR: mmap of registers failed");
		return -1;
	}


	/* Print a register value */
	value = *(uint8_t *)(base + PWCR_1);
	printf ("PWCR_1 = 0x%X\n", value);


	/* clean up at the end of your application */
	munmap(base, PWM_REG_BASE_SIZE);
	close(reg_fd);
}

