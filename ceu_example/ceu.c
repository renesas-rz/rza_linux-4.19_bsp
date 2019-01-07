#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "ceu.h"

static void *ceu_base;

static inline uint32_t ceu_read32(int offset)
{
	return *(uint32_t *)(ceu_base + offset);
}

static inline void ceu_write32(int offset, uint32_t data)
{
	*(uint32_t *)(ceu_base + offset) = data;
}

void ceu_set_base_addr(void *base_addr)
{
	ceu_base = base_addr;
}

void ceu_print_register(void)
{
//	printf("ceu_base = %p\n", ceu_base);	/* Capture start register */

	printf("CAPSR = 0x%08X\n", ceu_read32(CAPSR));	/* Capture start register */
	printf("CAPCR = 0x%08X\n", ceu_read32(CAPCR));	/* Capture control register */
	printf("CAMCR = 0x%08X\n", ceu_read32(CAMCR));	/* Capture interface control register */
	printf("CMCYR = 0x%08X\n", ceu_read32(CMCYR));	/* Capture interface cycle  register */
	printf("CAMOR = 0x%08X\n", ceu_read32(CAMOR));	/* Capture interface offset register */
	printf("CAPWR = 0x%08X\n", ceu_read32(CAPWR));	/* Capture interface width register */
	printf("CAIFR = 0x%08X\n", ceu_read32(CAIFR));	/* Capture interface input format register */
	printf("CSTCR = 0x%08X\n", ceu_read32(CSTCR));	/* Camera strobe control register (<= sh7722) */
	printf("CSECR = 0x%08X\n", ceu_read32(CSECR));	/* Camera strobe emission count register (<= sh7722) */
	printf("CRCNTR = 0x%08X\n", ceu_read32(CRCNTR));	/* CEU register control register */
	printf("CRCMPR = 0x%08X\n", ceu_read32(CRCMPR));	/* CEU register forcible control register */
	printf("CFLCR = 0x%08X\n", ceu_read32(CFLCR));	/* Capture filter control register */
	printf("CFSZR = 0x%08X\n", ceu_read32(CFSZR));	/* Capture filter size clip register */
	printf("CDWDR = 0x%08X\n", ceu_read32(CDWDR));	/* Capture destination width register */
	printf("CDAYR = 0x%08X\n", ceu_read32(CDAYR));	/* Capture data address Y register */
	printf("CDACR = 0x%08X\n", ceu_read32(CDACR));	/* Capture data address C register */
	printf("CDBYR = 0x%08X\n", ceu_read32(CDBYR));	/* Capture data bottom-field address Y register */
	printf("CDBCR = 0x%08X\n", ceu_read32(CDBCR));	/* Capture data bottom-field address C register */
	printf("CBDSR = 0x%08X\n", ceu_read32(CBDSR));	/* Capture bundle destination size register */
	printf("CFWCR = 0x%08X\n", ceu_read32(CFWCR));	/* Firewall operation control register */
	printf("CLFCR = 0x%08X\n", ceu_read32(CLFCR));	/* Capture low-pass filter control register */
	printf("CDOCR = 0x%08X\n", ceu_read32(CDOCR));	/* Capture data output control register */
	printf("CDDCR = 0x%08X\n", ceu_read32(CDDCR));	/* Capture data complexity level register */
	printf("CDDAR = 0x%08X\n", ceu_read32(CDDAR));	/* Capture data complexity level address register */
	printf("CEIER = 0x%08X\n", ceu_read32(CEIER));	/* Capture event interrupt enable register */
	printf("CETCR = 0x%08X\n", ceu_read32(CETCR));	/* Capture event flag clear register */
	printf("CSTSR = 0x%08X\n", ceu_read32(CSTSR));	/* Capture status register */
	printf("CSRTR = 0x%08X\n", ceu_read32(CSRTR));	/* Capture software reset register */
	printf("CDSSR = 0x%08X\n", ceu_read32(CDSSR));	/* Capture data size register */
	printf("CDAYR2 = 0x%08X\n", ceu_read32(CDAYR2));	/* Capture data address Y register 2 */
	printf("CDACR2 = 0x%08X\n", ceu_read32(CDACR2));	/* Capture data address C register 2 */
	printf("CDBYR2 = 0x%08X\n", ceu_read32(CDBYR2));	/* Capture data bottom-field address Y register 2 */
	printf("CDBCR2 = 0x%08X\n", ceu_read32(CDBCR2));	/* Capture data bottom-field address C register 2 */
}

void ceu_start_cap(void)
{
	ceu_write32(CETCR,0x00000000);	/* Clear out any previous status */
	ceu_write32(CAPSR,0x00000001);	/* Capture start register */
}

int ceu_is_capturing(void)
{
//	if ((ceu_read32(CSTSR) == 0) && (ceu_read32(CSTSR) == 0) )
//	{
//		ceu_print_register();
//		printf("CDSSR = 0x%08X\n", ceu_read32(CDSSR));	/* Capture data size register */
//		printf("CETCR = 0x%08X\n", ceu_read32(CETCR));	/* Capture event flag clear register */
//}

	return ceu_read32(CSTSR) | ceu_read32(CAPSR);
}

void ceu_set_buffer_addr(uint32_t phys_addr)
{
	ceu_write32(CDAYR,phys_addr);		/* Capture data address Y register */
	ceu_write32(CDACR,phys_addr + (640*480));	/* Capture data address C register */
}

static int ceu_soft_reset(void)
{
	int i, success = 0;

	ceu_write32(CAPSR, 1 << 16); /* reset */

	/* wait CSTSR.CPTON bit */
	for (i = 0; i < 1000; i++) {
		if (!(ceu_read32(CSTSR) & 1)) {
			success++;
			break;
		}
		usleep(1000);
	}

	/* wait CAPSR.CPKIL bit */
	for (i = 0; i < 1000; i++) {
		if (!(ceu_read32(CAPSR) & (1 << 16))) {
			success++;
			break;
		}
		usleep(1000);
	}

	if (2 != success) {
		printf("soft reset time out\n");
		return -1;
	}
 
	ceu_write32(CETCR,0x00000000);	/* Clear any pending interrupts */
	return 0;
}


int ceu_zero_all_regs(void)
{
	/* The Power on reset state is all zeros */
	ceu_write32(CAPSR,0x00000000);	/* Capture start register */
	ceu_write32(CAPCR,0x00000000);	/* Capture control register */
	ceu_write32(CAMCR,0x00000000);	/* Capture interface control register */
	ceu_write32(CMCYR,0x00000000);	/* Capture interface cycle  register */
	ceu_write32(CAMOR,0x00000000);	/* Capture interface offset register */
	ceu_write32(CAPWR,0x00000000);	/* Capture interface width register */
	ceu_write32(CAIFR,0x00000000);	/* Capture interface input format register */
	ceu_write32(CRCNTR,0x00000000);	/* CEU register control register */
	ceu_write32(CRCMPR,0x00000000);	/* CEU register forcible control register */
	ceu_write32(CFLCR,0x00000000);	/* Capture filter control register */
	ceu_write32(CFSZR,0x00000000);	/* Capture filter size clip register */
	ceu_write32(CDWDR,0x00000000);	/* Capture destination width register */
	ceu_write32(CDAYR,0x00000000);	/* Capture data address Y register */
	ceu_write32(CDACR,0x00000000);	/* Capture data address C register */
	ceu_write32(CDBYR,0x00000000);	/* Capture data bottom-field address Y register */
	ceu_write32(CDBCR,0x00000000);	/* Capture data bottom-field address C register */
	ceu_write32(CBDSR,0x00000000);	/* Capture bundle destination size register */
	ceu_write32(CFWCR,0x00000000);	/* Firewall operation control register */
	ceu_write32(CLFCR,0x00000000);	/* Capture low-pass filter control register */
	ceu_write32(CDOCR,0x00000000);	/* Capture data output control register */
	ceu_write32(CEIER,0x00000000);	/* Capture event interrupt enable register */
	ceu_write32(CETCR,0x00000000);	/* Capture event flag clear register */
//	ceu_write32(CSTSR,0x00000000);	/* Capture status register */
	ceu_write32(CSRTR,0x00000000);	/* Capture software reset register */
	ceu_write32(CDSSR,0x00000000);	/* Capture data size register */
	ceu_write32(CDAYR2,0x00000000);	/* Capture data address Y register 2 */
	ceu_write32(CDACR2,0x00000000);	/* Capture data address C register 2 */
	ceu_write32(CDBYR2,0x00000000);	/* Capture data bottom-field address Y register 2 */
	ceu_write32(CDBCR2,0x00000000);	/* Capture data bottom-field address C register 2 */	

}


/*
 * These values assume YUV422
 *
 * width: The capture width of the camera image
 * height: The capture hight of the camera image
 * mode: 0 = Image Capture Mode (Y/CbCr split buffer mode)
 *       1 = Data Synchronous Fetch Mode (data is read from the camera and written exactly the same to the image buffer)
 *       2 = Data enable fetch mode (not supported in some RZ/A devices)
 */
int ceu_init(int width, int height, int mode)
{
	uint32_t value;

	ceu_soft_reset();

	/* Just so we know everything is in a known state */
	ceu_zero_all_regs();

	/* Capture interface control register */
	/* Data synchronous fetch mode */
	/* 8-bit data bus */
	/* DTARY: Image input data is fetched in the order of Y0, Cb0, Y1, and Cr0 */
	value = (0 << 12);	/* DTIF: 8-bit data bus */
	if (mode == 0)
		value |= (0 << 4);	/* JPG[1:0] =  Image capture mode */
	if (mode == 1)
		value |= (1 << 4);	/* JPG[1:0] =  Data synchronous fetch mode */
	if (mode == 2)
		value |= (2 << 4);	/* JPG[1:0] =  Data enable fetch mode */
	ceu_write32(CAMCR, value);

	/* Capture control register */
	/* Y data and C data are transferred in 256-byte or 128-byte units */
	ceu_write32(CAPCR,
		(3 << 20) | 	/* MTCM: Data is transferred in 256-byte units */
		//(2 << 20) | 	/* MTCM: Data is transferred in 128-byte units */
		(0 << 16 ) |	/* CTNCP: One-frame capture when the CE bit is */
		0);

	/* Horizontal and vertical blanking periods */
	ceu_write32(CAMOR,0x00000000);	/* Capture interface offset register */

	/* Capture interface input format register */
	/* Not capturing interlaced images so you can ignore all the 'field' registers */
	ceu_write32(CAIFR,0x00000000);

	/* Capture interface width register */
	ceu_write32(CAPWR,(height << 16) |	/*  Vertical capture period (number of vertical lines) */
			  (width * 2));		/*  Horizontal capture period (number of clocks to make
						    full horizontal line: YCbCr422 = 2 clks per pixel ). */

	/* Capture filter size clip register */
	/* Just set it to your entire image (no clipping the image to be smaller) */
	ceu_write32(CFSZR, (height << 16) | width); /*  Vertical resolution, Horizontal resolution */

	/* Capture destination width register */
	/* In the case where the image coming from the camera is smaller than the frame buffer
	 * are directly writing it to, you can use this register to say how big the frame
	 * buffer is */
	if (mode == 0)
		ceu_write32(CDWDR, width );
	else
		 /* In "Data synchronous fetch mode", you should set this the same as 'HWDTH' in 'CAPWR' */
		ceu_write32(CDWDR, width * 2);

//	// Fill in these registers using ceu_set_buffer_addr()
//	ceu_write32(CDAYR,0x20600000);	/* Capture data address Y register */
//	ceu_write32(CDACR,0x2064b000);	/* Capture data address C register */

	/* Capture data output control register */
	/* Output YCbCr422 */
	/* Data is swapped in 32-bit unit */
	/* Data is swapped in 16-bit units */
	/* Data is swapped in 8-bit units */
	ceu_write32(CDOCR,0x00000017);

	/* Capture event interrupt enable register */
//	ceu_write32(CEIER,0x00100001);

	/* Capture event flag clear register */
	/* Writing 00s clears everything */
	ceu_write32(CETCR,0x00000000);

	return 0;
}
