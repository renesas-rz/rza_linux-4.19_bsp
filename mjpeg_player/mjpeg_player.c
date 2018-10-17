#define _LARGEFILE64_SOURCE

/* Optional Defines */
//#define TSLIB

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/types.h> 
#include <sys/mman.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include "jpeg_regs.h"

#ifdef TSLIB
#include "tslib.h"
#endif

// booted with "=> run xsa_boot"
/* JCU Source Image RAM */
#define	JPEG_WB_PHYS	lcd_fb_phys + 0x200000	// Physical RAM address to hold JPEG file to decompress
#define JPEG_IMG_SIZE	(128*1024)		// The max size of a .jpg file to decode
#define JPEG_WB_SIZE	(JPEG_IMG_SIZE*2)	// Size of total work buffer area (double buffered)

/* Globals */
uint32_t lcd_fb_phys;		/* physical RAM address of LCD frame buffer */
int lcd_width;			/* Width of the LCD in pixels */
int lcd_height;			/* Height of the LCD in pixels */
int lcd_bytes_per_pixel;	/* Bytes per pixel of current frame buffer */

int x_offset;			/* offset from top left of screen */
int y_offset;			/* offset from top left of screen */
float custom_fps;		/* override the framerate in the file */

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

int detect_video(void)
{
	struct fb_fix_screeninfo fix;
	struct fb_var_screeninfo var;
	int fb0_fd;
	int ret;

	fb0_fd = open("/dev/fb0", O_RDWR);
	if (fb0_fd < 0) {
		perror("Can't open /dev/fb0");
		fb0_fd = 0;
		return -1;
	}

	if (ioctl(fb0_fd, FBIOGET_FSCREENINFO, &fix) < 0) {
		perror("ioctl FBIOGET_FSCREENINFO");
		close(fb0_fd);
		return -1;
	}

	if (ioctl(fb0_fd, FBIOGET_VSCREENINFO, &var) < 0) {
		perror("ioctl FBIOGET_VSCREENINFO");
		close(fb0_fd);
		return -1;
	}

	lcd_width = var.xres;
	lcd_height = var.yres;
	lcd_bytes_per_pixel = var.bits_per_pixel / 8;

	/* Determine physical address of LCD frame buffer by looking at
	 * the vmalloc table (MMU remapping) */
	system("cat /proc/vmallocinfo | grep vdc5 > /tmp/vdc5.txt");
	{
		FILE * fp;
		char * line = NULL;
		size_t len = 0;
		ssize_t read;
		char *index;
		uint32_t address = 0;

		fp = fopen("/tmp/vdc5.txt", "r");
		if (fp == NULL) {
			perror("Can't open /tmp/vdc5.txt");
			return -1;
		}

		while ((read = getline(&line, &len, fp)) != -1)
		{
			index = strstr(line,"phys=");
			if (index)
			{
				/* extract the address (after 'phys=') */
				sscanf(index+5,"%x\n",&address);
				/* The address will be either 0x2??????? or 0x6??????? */
				if (((address & 0xF0000000) == 0x20000000) ||
				    ((address & 0xF0000000) == 0x60000000))
				{
					lcd_fb_phys = address;
					break;
				}
			}
		}
		fclose(fp);
		if (line)
			free(line);

		if (lcd_fb_phys == 0)
		{
			perror("Can't determine physical address of LCD frame buffer");
			return -1;
		}
	}

	/* Print Screen Statistics */
	printf("Screen Resolution: %dx%d (%d bpp)\n",lcd_width,lcd_height, lcd_bytes_per_pixel*8);
	printf("Frame Buffer Physical Address: %p\n",lcd_fb_phys);
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


/* main */
int main(int argc, char *argv[])
{

	int mem_fd, jpeg_fd;
	void *jpeg_base;
	uint8_t *jpeg_wb;
	uint8_t fb_num = 0, first_run = 0;
	uint32_t time_decode_start = 0,
		 size, max_size = 0, frame_count = 0;
	float fps;
	struct timespec curtime;
#ifdef TSLIB
	struct tsdev *ts;
	char *tsdevice = NULL;
	struct ts_sample samp;
#endif
	int ret;
	struct stat file_stat;
	int i;
	int single_file = 0;
	char *input_file;


	if (argc < 2) {
		printf("Usage:\n%s [-x] [-y] filename\n", argv[0]);
		printf("      [-x N] Horizontal screen offset by N pixels \n");
		printf("      [-y N] Vertical screen offset by N pixels\n");
		printf("      [-f N] Custom framerate (decimal number)\n");
		return 1;
	}

	i = 1;
	for( i=1; i < argc; i++) {
		if( !strcmp(argv[i],"-x") )
		{
			sscanf(argv[i+1], "%d", &x_offset);
			i++;
		}
		else if( !strcmp(argv[i],"-y") )
		{
			sscanf(argv[i+1], "%d", &y_offset);
			i++;
		}
		else if( !strcmp(argv[i],"-f") )
		{
			sscanf(argv[i+1], "%f", &custom_fps);
			i++;
		}
		else
		{
			// Must be input file
			input_file = argv[i];
		}
	}

	/* Manually Enable the JCU HW Engine (cancel module stop mode) */
	if (module_clock_enable(61))	/* Clears bit MSTP61 in STBCR5 */
		exit(1);

	/* Determine screen resolution and frame buffer physical address */
	if (detect_video())
		exit(1);

	/* Print additional information */
	printf("JPEG Buffer Physical Address: %p\n",JPEG_WB_PHYS);
	printf("JPEG Buffer size:  %d (%d x 2)\n",JPEG_WB_SIZE, JPEG_IMG_SIZE);

	mem_fd = open("/dev/mem", O_RDWR);
	if(mem_fd == -1) {
		perror("error: Failed to open /dev/mem!");
		return 1;
	}

	/* Map JPU registers and memory */
	jpeg_base = mmap(NULL, 0x400,
			 PROT_READ | PROT_WRITE,
			 MAP_SHARED, mem_fd,
			 JPEG_REG_PHYS);
	if (jpeg_base == (void *) -1) {
		perror("error: Could not map JPU registers!");
		return 1;
	}

	/* Map JPEG work buffer */
	jpeg_wb = (uint8_t *)mmap(NULL, JPEG_WB_SIZE,
			  	  PROT_READ | PROT_WRITE,
				  MAP_SHARED, mem_fd,
				  JPEG_WB_PHYS);
	if (jpeg_wb == (void *) -1) {
		perror("error: Could not map JPU work buffer!");
		return 1;
	}


#ifdef TSLIB
	/* Touchscreen stuff */
	if( (tsdevice = getenv("TSLIB_TSDEVICE")) != NULL ) {
		ts = ts_open(tsdevice,1);
	} else {
		if (!(ts = ts_open("/dev/input/event0", 1)))
			ts = ts_open("/dev/touchscreen/ucb1x00", 1);
	}

	if (!ts) {
		perror("ts_open");
		exit(1);
	}

	if (ts_config(ts)) {
		perror("ts_config");
		exit(1);
	}
#endif

	/* Open motion JPEG and read header */
	jpeg_fd = open64(input_file, O_RDONLY | O_RSYNC | O_SYNC);
	if (jpeg_fd <= 0) { 
		printf("%s: file %s not found\n", argv[0], input_file);
		return 1;
	}
	read(jpeg_fd, &fps, 4);


	/* Determine if the file passed was a single JPEG or a MJPEG by looking
	   at the first 2 bytes (magic number)*/
	if ((*(uint32_t *)&fps & 0xFFFF) == 0xD8FF) {
		single_file = 1;
		fps = 1;	// dummy
		lseek(jpeg_fd, 0, SEEK_SET);	// rewind file pointer to beginning of file
		printf("displaying single image\n");
	}

	if (custom_fps)
		fps = custom_fps;

	if (fps < 90.0 && fps > 0.1) {
		printf("fps=%f\n",fps);
	} else {
		printf("ERROR: FPS value out of range (%f) (corrupt MJPEG?)\n",fps);
		goto out;
	}

	while(1) {
		if (single_file) {
			/* Determine the file size */
			stat(input_file,&file_stat);
			size = file_stat.st_size;

		} else {	/* motion JPEG */

			/* Get size of next JPEG and read to BG buffer */
			read(jpeg_fd, &size, 4);
			if (size == 0xffffffff)
				break;
		}

		frame_count++;

		/* Keep track of the maximum size of the JPEGs (so you know how big of a RAM buffer you really needed */
		if (size > max_size)
			max_size = size;

		/* Read in JPEG file */
		read(jpeg_fd, jpeg_wb + (fb_num ? (JPEG_WB_SIZE/2) : 0), size);

		if (first_run != 0) {

			/* Wait for JPEG unit to finish decode */
			while(!jpeg_getreg32(jpeg_base, JPU_JINTS1) && !jpeg_getreg8(jpeg_base, JPU_JINTS0));

			if(!(jpeg_getreg8(jpeg_base, JPU_JINTS0) & 0x40 || jpeg_getreg32(jpeg_base, JPU_JINTS1) & 0x00000004)) {
				printf("JPEG decode failed!\n");
			}

#ifdef TSLIB
			/* Pause/unpause on touch/touch */
			if(ts_read(ts, &samp, 1) == 1) {
				while(1) {
					if(ts_read(ts, &samp, 1) == 1)
						if(samp.pressure == 0) break;
					usleep(100);
				}
				while(1) {
					if(ts_read(ts,&samp, 1) == 1)
						if(samp.pressure == 1) break;
					usleep(100);
				}
				while(1) {
					if(ts_read(ts,&samp, 1) == 1)
						if(samp.pressure == 0) break;
					usleep(100);
				}	
			}
#endif
			/* Sleep until it's time for a new frame */
			do {

				usleep(100);
				clock_gettime(CLOCK_MONOTONIC, &curtime);
			} while((float)(curtime.tv_nsec - time_decode_start) < (822000000.0/fps));

		} else {
			first_run = 1;
		}

		/* Set up JPU registers */
		jpeg_setreg8(jpeg_base, JPU_JINTS0, 0x00);
		jpeg_setreg32(jpeg_base, JPU_JINTS1, 0x00000000);
		jpeg_setreg8(jpeg_base, JPU_JCMOD, 0x08);	/* select decompression */
		jpeg_setreg8(jpeg_base, JPU_JINTE0, 0x00);

		/* Set Decompress Source Address (JPEG file location) */
		/* Remember, we are double buffering for speed purposes */
		jpeg_setreg32(jpeg_base, JPU_JIFDSA, JPEG_WB_PHYS + (fb_num ? (JPEG_WB_SIZE/2) : 0));

		/* Set output image data format */
		if (lcd_bytes_per_pixel == 2)
			jpeg_setreg32(jpeg_base, JPU_JIFDCNT, (jpeg_getreg32(jpeg_base, JPU_JIFDCNT) & 0xFCFFF8F8) | 0x02000706);	/* RGB565 */
		else
			jpeg_setreg32(jpeg_base, JPU_JIFDCNT, (jpeg_getreg32(jpeg_base, JPU_JIFDCNT) & 0xFCFFF8F8) | 0x01000704);	/* RGBA888 */

		/* JCU will decode directly into the live LCD frame buffer */
		jpeg_setreg32(jpeg_base, JPU_JIFDDOFST, lcd_bytes_per_pixel*lcd_width);
		jpeg_setreg32(jpeg_base, JPU_JIFDDA, lcd_fb_phys + ((x_offset + y_offset*lcd_width) * lcd_bytes_per_pixel));

		/* Start decode */
		jpeg_setreg8(jpeg_base, JPU_JCCMD, 0x01);
		time_decode_start = curtime.tv_nsec;

		/* Swap JPEG buffers */
		fb_num = fb_num ? 0 : 1;

		if (single_file)
			break;
	}

out:
	close(jpeg_fd);

	/* Unmap JPU registers and memory */
	if(munmap(jpeg_base, 0x400) == -1) {
		perror("error: Could not unmap JPU registers!");
		return 1;
	}
	if(munmap((void *)jpeg_wb, JPEG_WB_SIZE) == -1) {
		perror("error: Could not unmap JPU work buffer!");
		return 1;
	}
	if(close(mem_fd) == -1) {
		perror("error: Could not close /dev/mem!");
		return 1;
	}

	printf("\n%d JPEG images displayed\n", frame_count);
	printf("Max JPEG image size was %d bytes\n", max_size);
	return 0;
}

