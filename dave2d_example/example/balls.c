#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <linux/fb.h>
#include <sys/mman.h>

#include "dave_driver.h"
#include "jpeg_regs.h"

#include "bg_800x480.c"

#define D2_FORMAT d2_mode_rgb565
#define SUBPXL	16	/* 16 subpixels per pixel */

#define RUN_TIME 10 /* runs for 10 seconds, then exists */

int lcd_width;
int lcd_height;
int lcd_refresh_us; /* time in us between screen refresh */
int lcd_bpp;
void *framebuffer = (void *)0x80000000;

#define MAX_X (lcd_width * SUBPXL)
#define MAX_Y (lcd_height * SUBPXL)
#define MAX_Z (60 * SUBPXL)
#define MAX_SPEED (10*SUBPXL)

#define MIN_X (0)
#define MIN_Y (0)
#define MIN_Z (20 * SUBPXL)
#define MIN_SPEED (SUBPXL)

struct ball {
	int x;
	int y;
	int z;
	d2_color color;
	int x_vector;
	int y_vector;
	int z_vector;
};

#define TOTAL_BALLS 32
struct ball balls[TOTAL_BALLS];

struct fb_fix_screeninfo fix;
struct fb_var_screeninfo var;

void *jpeg_base;

#define	JPEG_WB_PHYS	0x80300000	// Physical RAM address to hold JPEG file to decompress
#define JPEG_IMG_SIZE	(128*1024)		// The max size of a .jpg file to decode
#define JPEG_WB_SIZE	(JPEG_IMG_SIZE*2)	// Size of total work buffer area (double buffered)

#define USE_DEVFB
//#define USE_SYSFS
int detect_display(void)
{
	int fd = 0;
	int ret = -1;
	struct timeval tv1, tv2, tv3;
	char buffer[200];
	ssize_t size;
	int i;
	unsigned long value = 0;

	fd = open("/dev/fb0", O_RDWR);
	if (fd < 0)
	{
		perror("Can't open /dev/fb0");
		goto out;
	}

#ifdef USE_DEVFB
	if (ioctl(fd, FBIOGET_FSCREENINFO, &fix) < 0)
	{
		perror("ioctl FBIOGET_FSCREENINFO");
		goto out;
	}

	if (ioctl(fd, FBIOGET_VSCREENINFO, &var) < 0)
	{
		perror("ioctl FBIOGET_VSCREENINFO");
		goto out;
	}

	lcd_width = var.xres;
	lcd_height = var.yres;
	lcd_bpp = var.bits_per_pixel;
#endif

	// Wait for VSYNC
	if(ioctl(fd, FBIO_WAITFORVSYNC, NULL))
	{
		printf("FBIO_WAITFORVSYNC call failed\n");
		goto out;
	}

	/* Determine refresh rate */
	gettimeofday(&tv1,NULL);
	ioctl(fd, FBIO_WAITFORVSYNC, NULL);	// Wait for VSYNC
	gettimeofday(&tv2,NULL);
	timersub( &tv2, &tv1, &tv3);
	lcd_refresh_us = tv3.tv_usec;

	close(fd);
	fd = 0;

	/* Read current LCD configuration directly from VDC6 driver sysfs interface */
	fd = open("/sys/bus/platform/drivers/vdc5fb/fcff7400.display/layer2", O_RDWR);
	if (fd < 0)
	{
		perror("can't open /sys/bus/platform/drivers/vdc5fb/fcff7400.display/layer2");
		goto out;
	}
	size = read(fd, buffer, 100);
	if (size < 0)
	{
		perror("read error of layer2");
		goto out;
	}

	/* find the line "base = " */
	for (i = 0; i < size; i++)
	{
		/* "base =" */
		if (buffer[i] == 'b') {
			/* hint, sizeof a string also includes the NULL */
			if (strncmp(buffer+i, "base = ", sizeof("base =" )) == 0)
			{
				if ( sscanf(buffer + i + sizeof("base ="), "0x%lX", &value) != 1)
					goto out;
				framebuffer = (void *)value;
				i += sizeof("base = 0x80000000");
			}
		}

#ifdef USE_SYSFS_ONLY
		/* "xres = 800" */
		if (buffer[i] == 'x') {
			/* hint, sizeof a string also includes the NULL */
			if (strncmp(buffer+i, "xres = ", sizeof("xres =" )) == 0)
			{
				if ( sscanf(buffer + i + sizeof("xres ="), "%d", &lcd_width) != 1)
					goto out;
				i += sizeof("xres = 800");
			}
		}

		/* "yres = 480" */
		if (buffer[i] == 'y') {
			/* hint, sizeof a string also includes the NULL */
			if (strncmp(buffer+i, "yres = ", sizeof("yres =" )) == 0)
			{
				if ( sscanf(buffer + i + sizeof("yres ="), "%d", &lcd_height) != 1)
					goto out;
				i += sizeof("yres = 800");
			}
		}

		/* "bpp = 16" */
		if (buffer[i] == 'b') {
			/* hint, sizeof a string also includes the NULL */
			if (strncmp(buffer+i, "bpp = ", sizeof("bpp =" )) == 0)
			{
				if ( sscanf(buffer + i + sizeof("bpp ="), "%d", &lcd_bpp) != 1)
					goto out;

				i += sizeof("bpp = 16");
			}
		}
#endif
	}

	if (!framebuffer || !lcd_width || !lcd_height || !lcd_bpp)
	{
		printf("Error, cannot detect current LCD configuration\n");
		printf("\tframebuffer = 0x%08lX\n", (unsigned long) framebuffer);
		printf("\tlcd_width = %d\n", lcd_width);
		printf("\tlcd_height = %d\n", lcd_height);
		printf("\tlcd_bpp = %d\n", lcd_bpp);
		goto out;
	}

	if (lcd_bpp != 16)
	{
		printf("ERROR: Can only display to a /dev/fb0 of RGB-565\n");
		goto out;
	}

	printf("Display LCD: %dx%d, %dbpp, %.1ffps, 0x%08lX\n",lcd_width, lcd_height, lcd_bpp,
							(double)1000000/tv3.tv_usec,
							(unsigned long)framebuffer);
	ret = 0;
out:

	if (fd > 0)
		close(fd);

	return ret;
}

int module_clock_enable(int mstp)
{
	int mem_fd;
	int reg;
	int bit;
	unsigned char *addr;
	void *stbcr_base;
#define STBCR_BASE	0xFCFE0000	/* mappings must always be on page boundary */
#define STBCR3_OFFSET	0x420		/* offset to STBCR0 */

	reg = mstp / 10;
	bit = mstp % 10;
	reg -= 3;		/* The MSTP values actually starts from 30, not 0 */

	mem_fd = open("/dev/mem", O_RDWR);
	if(mem_fd == -1) {
		perror("error: Failed to open /dev/mem!");
		return -1;
	}

	/* Map STBCR registers and memory */
	stbcr_base = mmap(NULL, 0x20,
			 PROT_READ | PROT_WRITE,
			 MAP_SHARED, mem_fd,
			 STBCR_BASE);
	if (stbcr_base == (void *) -1) {
		perror("error: Could not map STBCR registers!");
		return -1;
	}

	addr = stbcr_base + STBCR3_OFFSET;	/* Add in our offset from page boundary */
	addr += reg * 4;

	/* Clear the MSTP bit to allow the HW module to run */
	*addr &= ~(1 << bit);

	munmap(stbcr_base, 0x20);
	close(mem_fd);
	return 0;
}

/* Location of JCU registers */
#define JPEG_REG_PHYS	0xE8017000

/* Register access functions */

static inline uint32_t jpeg_getreg32(void *base, uint32_t address)
{
	return *(volatile uint32_t*)(base + address);
}

static inline uint8_t jpeg_getreg8(void *base, uint32_t address)
{
	return *(volatile uint8_t*)(base + address);
}

static inline void jpeg_setreg32(void *base, uint32_t address, uint32_t value)
{
	*(volatile uint32_t*)(base + address) = value;
}

static inline void jpeg_setreg8(void *base, uint32_t address, uint8_t value)
{
	*(volatile uint8_t*)(base + address) = value;
}


void decode_jpeg(uint32_t src_phys, uint32_t dst_phys, int dst_stride, int height, int bpp)
{
	static int first = 1;

	if (first)
	{
		/* Manually Enable the JCU HW Engine (cancel module stop mode) */
		if (module_clock_enable(61))	/* Clears bit MSTP61 in STBCR5 */
			exit(1);

		/* Set up JPU registers */
		jpeg_setreg8(jpeg_base, JPU_JINTS0, 0x00);
		jpeg_setreg32(jpeg_base, JPU_JINTS1, 0x00000000);
		jpeg_setreg8(jpeg_base, JPU_JCMOD, 0x08);	/* select decompression */
		jpeg_setreg8(jpeg_base, JPU_JINTE0, 0x00);

		first = 0;
	}

	/* Set Decompress Source Address (JPEG file location) */
	/* Remember, we are double buffering for speed purposes */
	jpeg_setreg32(jpeg_base, JPU_JIFDSA, src_phys);

	/* Set output image data format */
	if (bpp == 16)
		jpeg_setreg32(jpeg_base, JPU_JIFDCNT,
			(jpeg_getreg32(jpeg_base, JPU_JIFDCNT) & 0xFCFFF8F8) | 0x02000706);	/* RGB565 */
	else
		jpeg_setreg32(jpeg_base, JPU_JIFDCNT,
			(jpeg_getreg32(jpeg_base, JPU_JIFDCNT) & 0xFCFFF8F8) | 0x01000704);	/* RGBA888 */

	jpeg_setreg32(jpeg_base, JPU_JIFDDOFST, (bpp/8)*dst_stride);

	jpeg_setreg32(jpeg_base, JPU_JIFDDA, dst_phys);

	/* Start decode */
	jpeg_setreg8(jpeg_base, JPU_JCCMD, 0x01);

	/* Wait for JPEG unit to finish decode */
	while(!jpeg_getreg32(jpeg_base, JPU_JINTS1) && !jpeg_getreg8(jpeg_base, JPU_JINTS0))
	{
		usleep(100);
	}

	if(!(jpeg_getreg8(jpeg_base, JPU_JINTS0) & 0x40 || jpeg_getreg32(jpeg_base, JPU_JINTS1) & 0x00000004)) {
		printf("JPEG decode failed!\n");
	}
}

void decode_jpeg_wait(uint32_t src_phys, uint32_t dst_phys)
{
	while(!jpeg_getreg32(jpeg_base, JPU_JINTS1) && !jpeg_getreg8(jpeg_base, JPU_JINTS0))
	{
		usleep(100);
	}
}


int main(void)
{
	int i, moved;
	int loop_count;
	d1_device *d1_handle;
	int fb_fd;
	int max_time = 0;
	void *bg_buff;
	struct timeval tv1, tv2, tv3;
	int live_fb;
	struct ball tmp_ball;
	void *fb_phys_addr[2];
	d2_color bg_color = 0;

	uint8_t *jpeg_wb;

	int mem_fd;

	mem_fd = open("/dev/mem", O_RDWR);
	if( mem_fd == -1) {
		printf("Can't open device node: mem\n");
		return -1;
	}

	/* Map JPU registers and memory */
	jpeg_base = mmap(NULL, 0x400,
			 PROT_READ | PROT_WRITE,
			 MAP_SHARED, mem_fd,
			 JPEG_REG_PHYS);
	if (jpeg_base == (void *) -1) {
		perror("error: Could not map JPU registers!");
		return -1;
	}

	/* Map JPEG work buffer */
	jpeg_wb = (uint8_t *)mmap(NULL, JPEG_WB_SIZE,
			  	  PROT_READ | PROT_WRITE,
				  MAP_SHARED, mem_fd,
				  JPEG_WB_PHYS);
	if (jpeg_wb == (void *) -1)
	{
		perror("error: Could not map JPU work buffer!");
		return -1;
	}

	/* Copy in our JPEG image */
	memcpy(jpeg_wb, bg_jpeg, sizeof(bg_jpeg));

	/* Detect current screen resolution */
	if (detect_display())
		return -1;

	/* Create a handle to the fbdev device */
	fb_fd = open("/dev/fb0", O_RDWR);
	if( fb_fd == -1) {
		printf("Can't open device node: fb0\n");
		return -1;
	}

	/* Randomize the initial value of the balls */
	for (i = 0; i < TOTAL_BALLS; i++)
	{
		balls[i].x = rand() % (lcd_width * SUBPXL);
		balls[i].y = rand() % (lcd_height * SUBPXL);
		balls[i].z = (rand() % (MAX_Z - MIN_Z)) + MIN_Z;

		/* vectors can be positive or negative */
		balls[i].x_vector = MAX_SPEED/2 - (rand() % MAX_SPEED);
		balls[i].y_vector = MAX_SPEED/2 - (rand() % MAX_SPEED);
		balls[i].z_vector = rand() % 32;

		balls[i].color = rand() % 0xFFFFFF;
	}

	fb_phys_addr[0] = framebuffer;
	fb_phys_addr[1] = framebuffer + (lcd_width * lcd_height * 2);

	/* Create a device */
	d2_device *d2_handle = d2_opendevice ( 0 );

	/* Initialize the hardware */
	d2_inithw( d2_handle, 0 );

	/* Get lowlevel device handle */
	d1_handle = d2_level1interface(d2_handle);

	/* Allocate Video memory */
	bg_buff = d1_allocvidmem ( d1_handle, d1_mem_display, lcd_width * lcd_height * 2 );

	/* Use JPEG engine to draw the background in our video memory */
	decode_jpeg(JPEG_WB_PHYS, (uint32_t)bg_buff, lcd_width, lcd_height, lcd_bpp);

	/* Define address and memory organization of the next framebuffer */
//	d2_framebuffer( d2_handle, framebuffer, lcd_width, lcd_width, lcd_height, D2_FORMAT );
//	d2_framebuffer( d2_handle, bg_buff, lcd_width, lcd_width, lcd_height, D2_FORMAT );

	/* Select FB #0 */
	d2_framebuffer( d2_handle, framebuffer, lcd_width, lcd_width, lcd_height, D2_FORMAT );

	/* Clear the background */
	d2_startframe(d2_handle);
	d2_clear( d2_handle, bg_color );
	d2_endframe(d2_handle);
	ioctl(fb_fd, FBIO_WAITFORVSYNC, NULL);	// Wait for VSYNC

	/* Make sure frame buffer #0 is the live one */
	live_fb = 0;
	var.yoffset = var.xoffset = 0;
	if (ioctl(fb_fd, FBIOPAN_DISPLAY, &var) == -1)
		printf("IOCTL FBIOPAN_DISPLAY Failed %d (%s)\n", errno,strerror(errno));
	/* Wait for VSYNC (next screen becomes visible) */
	ioctl(fb_fd, FBIO_WAITFORVSYNC, NULL);

	loop_count = RUN_TIME * 1000000 / lcd_refresh_us;	/* 5 seconds */
	for ( ;  loop_count ;  loop_count--)
	{
		gettimeofday(&tv1,NULL);	/* record start time */

		/* New sequnce of rendering commands */
		/* This also triggers the HW to start rendering the previous buffer of commands */
		d2_startframe(d2_handle);

		/* Define address and memory organization of the next framebuffer */
		/* We select the current live becaues we will not start painting until it get swapped out */
		d2_framebuffer( d2_handle, fb_phys_addr[live_fb], lcd_width, lcd_width, lcd_height, D2_FORMAT );

#if 0
		/* Clear the background */
		d2_clear( d2_handle, bg_color );
#else
		/* Copy in our background image */
		d2_setblitsrc(d2_handle, bg_buff, lcd_width, lcd_width, lcd_height, D2_FORMAT);
		d2_blitcopy(d2_handle,
			lcd_width, lcd_height, 0, 0,	// source
			lcd_width * SUBPXL, lcd_height * SUBPXL, 0, 0,	// destination
			0);

#endif

		/* Draw each ball */
		for (i = 0; i < TOTAL_BALLS; i++)
		{
			/* draw a new ball */
			d2_setcolor( d2_handle, 0, balls[i].color );
			d2_rendercircle( d2_handle, balls[i].x, balls[i].y, balls[i].z, 0 );
		}

		/* End the sequnce of command */
		/* Also waits for HW to finish rendering the other command buffer that
		 * d2_startframe was started by */
		d2_endframe(d2_handle);

		/* determine how long it took to render the previous command buffer */
		gettimeofday(&tv2,NULL);	/* record stop time */
		timersub( &tv2, &tv1, &tv3);
		if (tv3.tv_usec > max_time)
			max_time = tv3.tv_usec;

		/* Now switch the frame buffers */
		live_fb ^= 1;	/* toggle */
		var.yoffset = lcd_height * live_fb;
		ioctl(fb_fd, FBIOPAN_DISPLAY, &var);

		/* FBIOPAN_DISPLAY returns immedialy, so we need to wait for the next
		 * VSYNC (next screen becomes visible) before we continue. */
		ioctl(fb_fd, FBIO_WAITFORVSYNC, NULL);

		/* Change background color */
		bg_color += 10;

		/* Advance all the balls for next time */
		for (i = 0; i < TOTAL_BALLS; i++)
		{
			balls[i].z += balls[i].z_vector;
			if (balls[i].z > MAX_Z) {
				balls[i].z = MAX_Z;
				balls[i].z_vector = 0 - balls[i].z_vector;
			}
			if (balls[i].z < MIN_Z) {
				balls[i].z = MIN_Z;
				balls[i].z_vector = 0 - balls[i].z_vector;
			}

			balls[i].x += balls[i].x_vector;
			if (balls[i].x > (MAX_X - balls[i].z)) {
				balls[i].x = MAX_X - balls[i].z;
				balls[i].x_vector = 0 - balls[i].x_vector;
			}
			if (balls[i].x < (MIN_X + balls[i].z)) {
				balls[i].x = MIN_X + balls[i].z;
				balls[i].x_vector = 0 - balls[i].x_vector;
			}

			balls[i].y += balls[i].y_vector;
			if (balls[i].y > (MAX_Y - balls[i].z)) {
				balls[i].y = MAX_Y - balls[i].z;
				balls[i].y_vector = 0 - balls[i].y_vector;
			}
			if (balls[i].y < (MIN_Y + balls[i].z)) {
				balls[i].y = MIN_Y + balls[i].z;
				balls[i].y_vector = 0 - balls[i].y_vector;
			}
		}

		/* Bubble sort based on Z order. Smallest balls get
		 * drawn first */
		moved = 1;
		while(moved)
		{
			moved = 0;
			for (i = 0; i < TOTAL_BALLS - 1; i++)
			{
				if (balls[i].z > balls[i+1].z)
				{
					tmp_ball = balls[i];
					balls[i] = balls[i+1];
					balls[i+1] = tmp_ball;
					moved = 1;
				}
			}
		}
	}

	/* Wait for current rendering to end */
	d2_flushframe(d2_handle);

	d1_freevidmem ( d1_handle, d1_mem_display, bg_buff );

	d2_closedevice(d2_handle);

	printf("Max processing time = %d us\n", max_time);

	return 0;
}


