/*
 * ceu_omni.c
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

#include "jpeglib.h"

#include "ceu.h"
#include "ov7670.h"
#include "ov7740.h"

//#define DEBUG

/* Options */

/* Throw away this number frames at the begining of capturing */
/* The reason is that it takes a couple captures until the auto gain
   adjusts correclty */
#define THROW_AWAY_FRAMES 10

//#define IMAGE_CAPTURE_MODE	/* Use 'Image Capture' Mode of the CEU (not recomended) */

/* The CEU HW can capture a 4:2:2 stream and convert it to 4:2:0 when writing
   to the capture buffers. This reduces the memory requirements by 25% for each buffer */
//#define CAPTURE_YCBCR420	/* Convert YCbCr 4:2:2 to 4:2:0 in HW */

/* globals */
struct mem_map {
	char name[20];
	void *base;
	uint32_t addr;
	uint32_t size;
};
struct mem_map map_ceu, map_ram;

/* For JPEG Compression */
struct jpeg_compress_struct cinfo;
struct jpeg_error_mgr jerr;

int mem_fd;	/* file descriptor for /dev/mem */
int fb0_fd;	/* file descriptor for /dev/fb0 */
void *fb0_base;	/* mmap pointer for /dev/fb0 */

int vga_capture = 1;		/* 1=VGA, 0=QVGA */
int continuous_stream = 0;
int displayfb0 = 0;
int savejpg = 1;
int displayjpg = 0;
int displayyuv = 0;
int quiet = 0;
char * ycbcr_fb;		/* A separate frame buffer for LCD LVDS display */
static char i2c_dev_path[] = "/dev/i2c-#";
unsigned int cap_buf_addr;			/* Location to capture image to */
unsigned int cap_buf_size = 640*480*2;		/* Capture buffer size */

int cap_width = 640;
int cap_height = 480;

int lcd_width;
int lcd_height;

/* Supported Boards */
#define RZA1_RSKRZA1 0
#define RZA1_STREAMIT 1
#define RZA2_RZA2MEVB 2
int board;


/*
 * kbhit()
 * check if a keyboard key has been hit
 */
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

#define max(a,b) \
	({ __typeof__ (a) _a = (a); \
	__typeof__ (b) _b = (b); \
	_a > _b ? _a : _b; })
#define min(a,b) \
	({ __typeof__ (a) _a = (a); \
	__typeof__ (b) _b = (b); \
	_a < _b ? _a : _b; })

/*
 * YCbCr_to_RGB565()
 * convert a YCbCr pixel to RGB565
 */
uint16_t YCbCr_to_RGB565(int y, int cb, int cr)
{
	double Y = (double) y;
	double Cb = (double) cb;
	double Cr = (double) cr;
	uint16_t rgb565;

	int r = (int) (Y + 1.40200 * (Cr - 128));
	int g = (int) (Y - 0.34414 * (Cb - 128) - 0.71414 * (Cr - 128));
	int b = (int) (Y + 1.77200 * (Cb - 128));

	r = max(0, min(255, r));
	g = max(0, min(255, g));
	b = max(0, min(255, b));

#define RED(a)      ((((a) & 0xf800) >> 11) << 3)
#define GREEN(a)    ((((a) & 0x07e0) >> 5) << 2)
#define BLUE(a)     (((a) & 0x001f) << 3)

	r = r >> 3;
	g = g >> 2;
	b = b >> 3;
	rgb565 = (r << 11) | (g << 5) | b;

	return rgb565;
}


/*
 * display_on_fb0()
 *
 * mode 0: 8-bit Y to RGB565
 * mode 1: 8-bit Y to RGB565 (but every other bit, to convert VGA to QVGA)
 * mode 2: 16-bit YCbCr (Y0/Cb0/Y1/Cr0) to RGB565
 * mode 3: 16-bit YCbCr (Cb0/Y0/Cr0/Y1) to RGB565
 */
/* OV7670 output is: Y0, Cb0, Y1, and Cr0 */
void display_on_fb0(char *image_buff, int mode)
{
	int i;
	int row, col;
	char *fb;
	uint16_t pixel;
	char y,cb,cr;

	/* For converting 8-bit B&W to 16-bit RGB565 */
	#define BW_TO_RGB565(a) ((a & 0xF8) << 8) | ((a & 0xFC) << 3) | (a >> 3);

	for(row=0; row < cap_height; row++)
	{
		fb = fb0_base + row * lcd_width * 2;
		for(col=0; col < cap_width; col++)
		{
			if (mode == 0 | mode == 1)
			{
				pixel = *image_buff++;
				pixel = BW_TO_RGB565(pixel);
				*(uint16_t *)fb = pixel;
				fb += 2;
			}
			if (mode == 1)
			{
				image_buff++;	// skip
			}
			if (mode == 2) /* (Y0/Cb0/Y1/Cr0) */
			{
				y = *image_buff++;
				if( !(col & 1) ) {
					cb = *image_buff;
					cr = *(image_buff + 2);
				}
				image_buff++;	// skip over Cb/Cr

				pixel = YCbCr_to_RGB565(y,cb,cr);
				*(uint16_t *)fb = pixel;
				fb += 2;
			}
			if (mode == 3)	/* (Cb0/Y0/Cr0/Y1) */
			{
				if( !(col & 1) ) {
					cb = *image_buff;
					cr = *(image_buff + 2);
				}
				image_buff++;	// skip over Cb/Cr
				y = *image_buff++;

				pixel = YCbCr_to_RGB565(y,cb,cr);
				*(uint16_t *)fb = pixel;
				fb += 2;
			}
		}
	}
}

/* 
  planar_to_interleave()

  When the CEU read in the images it separates the YCbCr422 into separate Y Cb/Cr buffers
  automatically (and there's no way to turn that off by the way).

  The JPEG library wants pixel data to come in interleaved where each Y will have
  its own Cb and Cr. Therefore our buffer that we pass to libjpeg has to be bigger
  and we have to duplicate the color data.

  OV7670 output (YCbCr422) = Cb0,Y0,Cr0,Y1,Cb2,Y3,Cr2, etc...

  CEU buffer = Y0,Y1,Y2...........YN
               Cb0,Cr0,Cb1,Cr1,..CbN
  
When 4:2:0 conversion is enabled:
    CEU buffer = Y0,Y1,Y2...........YN
                 Cb0,Cr0,Cb1,Cr1,..CbN (only half as much data as Y

  libjpeg input = Y0,Cb0,Cr0,Y1,Cb0,Cr0,Y2,Cb2,Cr2,Y3,Cb2,Cr2,Y4,etc..

*/
static char *ibuf;
static char *planar_to_interleave(char *pBuf)
{
	char *y = pBuf;
	char *fb = ibuf;
	char *cbcr = y + cap_width * cap_height ;
	int col, row;

	if (!ibuf)
		ibuf = malloc(cap_width * cap_height * 3);

	for(row=0; row < cap_height; row++)
	{
#ifdef CAPTURE_YCBCR420
		/* YCbCr420 to YCbCr422 */
		/* Duplicate color data over 2 rows */
		if( row & 1)
			cbcr -= cap_width;
#endif
		/* Y0/Cb/Cr/Y1/Cb/Cr */
		for(col = 0; col < cap_width; col++)
		{
			*fb++ = *y++;
			if( col & 1)
			{
				/* on old colums, duplicate previous color data */
				*fb++ = *(cbcr - 2);
				*fb++ = *(cbcr - 1);
			}
			else
			{
				*fb++ = *cbcr++;
				*fb++ = *cbcr++;
			}
		}
	}
	return ibuf;
}


/*
  YUV422_to_libjpegYCbCr()

  The JPEG library wants pixel data to come in interleaved where each Y will have
  its own Cb and Cr. Therefore our buffer that we pass to libjpeg has to be bigger
  and we have to duplicate the color data.

  OV7670 output (YCbCr422) = Cb0,Y0,Cr0,Y1,Cb2,Y3,Cr2, etc...

  cam_fmt: What the current 422 format is
	0 = YUYV (Y/Cb/Y/Cr) Y0,Cb0,Y1,Cr0,Y3,Cb2, etc...
	1 = YVYU (Y/Cr/Y/Cb) Y0,Cr0,Y1,Cb0,Y3,Cr2, etc...
	2 = UYVY (Cb/Y/Cr/Y) Cb0,Y0,Cr0,Y1,Cb2,Y3,Cr2, etc...
	3 = VYUY (Cr/Y/Cb/Y) Cr0,Y0,Cb0,Y1,Cr2,Y3,Cb2, etc...

  libjpeg input = Y0,Cb0,Cr0,Y1,Cb0,Cr0,Y2,Cb2,Cr2,Y3,Cb2,Cr2,Y4,etc..

*/
static char *jpegbuf;
static char *YUV422_to_libjpegYCbCr(char *pBuf, int cam_fmt)
{
	char *fb;
	int col, row;
	char cb = *pBuf;
	char cr = *(pBuf + 2);

	if (!jpegbuf)
	{
		jpegbuf = malloc(cap_width * cap_height * 3);
	}

	fb = jpegbuf;

	for(row = 0; row < cap_height; row++)
	{
		for(col = 0; col < cap_width; col++)
		{
			/*  IN=Y0/Cb/Y1/Cr */
			/* OUT=Y0/Cb/Cr/Y1/Cb/Cr */
			if (cam_fmt == 0)
			{
				/* Cb or Cr */
				*fb++ = *pBuf++;	/* the Y is before C */
				if( col & 1)
					cr = *pBuf++;	/* old colums */
				else
					cb = *pBuf++;	/* even old colums */
			}

			/*  IN=Cb/Y0/Cr/Y1 */
			/* OUT=Y0/Cb/Cr/Y1/Cb/Cr */
			if (cam_fmt == 2)
			{
				/* Cb or Cr */
				if( col & 1)
					cr = *pBuf++;	/* old colums */
				else
					cb = *pBuf++;	/* even old colums */
				*fb++ = *pBuf++;	/* the Y is after C */
			}

			/* Now add the Cb and Cr */
			*fb++ = cb;
			*fb++ = cr;
		}
	}

	return jpegbuf;
}


int close_resources(void)
{
	if( map_ceu.base ) {
		munmap(map_ceu.base, map_ceu.size);
		map_ceu.base = 0;
	}
	if( map_ram.base ) {
		munmap(map_ram.base, map_ram.size);
		map_ram.base = 0;
	}


	if (ibuf)
		free(ibuf);

	if (jpegbuf)
		free(jpegbuf);

	if( fb0_base )
		munmap(fb0_base, lcd_width * 2 * lcd_height);	/* RGB565 */
	if( fb0_fd )
		close(fb0_fd);
}

int open_resources(void)
{
	int fd;
	char buffer[20];
	int count;
	FILE *fin;
	uint32_t size;
	uint32_t addr;
	int ret = 0;
	struct fb_fix_screeninfo fix;
	struct fb_var_screeninfo var;

	if (displayfb0 || displayyuv)
	{
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

#ifdef DEBUG
		printf("Display LCD: %dx%d,%dbpp\n",var.xres, var.yres,var.bits_per_pixel);
#endif
		lcd_width = var.xres;
		lcd_height = var.yres;

		if ( (cap_width > lcd_width) && displayfb0 )
		{
			printf("ERROR: You can only display the image on the LCD directly (by writting RGB565 directly to the frame buffer) if your camera image size is smaller than your LCD display. Try using the -q option to capture in QVGA.\n");
			return -1;
		}
	}

	if (displayfb0)
	{
		if (var.bits_per_pixel != 16)
		{
			printf("ERROR: Can only display to a /deb/fb0 of RGB-565\n");
			return -1;
		}
		fb0_base = mmap(NULL, lcd_width * 2 * lcd_height, /* RGB565 */
				  PROT_READ | PROT_WRITE,
				  MAP_SHARED, fb0_fd, 
				  0 );
		if (fb0_base == MAP_FAILED) {
			fb0_base = NULL;
			perror("Could not map /dev/fb0");
			close(fb0_fd);
			fb0_fd = 0;
		}
	}

	map_ceu.addr = 0xE8210000;
	map_ceu.size = 0x100;

	/* Open a handle to all memory */
	mem_fd = open("/dev/mem", O_RDWR);
	if (mem_fd < 0) {
		perror("Can't open /dev/mem");
		return -1;
	}

	/* Map our CEU registers */
	map_ceu.base = mmap(NULL, map_ceu.size,
			  PROT_READ | PROT_WRITE,
			  MAP_SHARED, mem_fd, 
			  map_ceu.addr );

	if (map_ceu.base == MAP_FAILED) {
		map_ceu.base = NULL;
		perror("Could not map CEU registers");
		close(mem_fd);
		return -1;
	}
	ceu_set_base_addr(map_ceu.base);

	/* Map RAM used for capturing frames */
	map_ram.addr = cap_buf_addr;
	map_ram.size = cap_buf_size;

	map_ram.base = mmap(NULL, map_ram.size,
			  PROT_READ | PROT_WRITE,
			  MAP_SHARED, mem_fd, 
			  map_ram.addr );
	if (map_ram.base == MAP_FAILED)
	{
		map_ram.base = NULL;
		perror("Could not map CEU registers");
		close(mem_fd);
		return -1;
	}

#ifdef DEBUG
	printf("map_ceu.size=0x%lx\n", map_ceu.size);
	printf("map_ceu.addr=0x%lx\n", map_ceu.addr);
	printf("map_ceu.base=%p\n", map_ceu.base);

	printf("map_ram.size=0x%lx\n", map_ram.size);
	printf("map_ram.addr=0x%lx\n", map_ram.addr);
	printf("map_ram.base=%p\n", map_ram.base);
#endif

	return 0;
}

static void save_jpeg(char *buf)
{
	static int inited = 0;
	static int fileno = 0;
	int row_stride;
	JSAMPROW row_pointer[1];
	char *image_buf;
	FILE * outfile;
	char filename[32];

	if(!inited)
	{
		memset(&cinfo, 0, sizeof(cinfo));
		memset(&jerr, 0, sizeof(jerr));

		cinfo.err = jpeg_std_error(&jerr);
		jpeg_create_compress(&cinfo);

		cinfo.image_width = cap_width; //image width and height, in pixels
		cinfo.image_height = cap_height;
		cinfo.input_components = 3;	// must be 3
		cinfo.in_color_space = JCS_YCbCr;
		jpeg_set_defaults(&cinfo);

		inited = 1;
	}

	if( continuous_stream )
		fileno = 0;	/* same filename each time */

	sprintf(filename, "capture%04d.jpg", fileno++);

	if ((outfile = fopen(filename, "wb")) == NULL)
	{
		fprintf(stderr, "can't open %s\n", filename);
		exit(1);
	}
	jpeg_stdio_dest(&cinfo, outfile);

	jpeg_start_compress(&cinfo, TRUE);

	row_stride = cap_width * 3;

#ifdef IMAGE_CAPTURE_MODE
	image_buf = planar_to_interleave(buf);
#else
	image_buf = YUV422_to_libjpegYCbCr(buf, CAM_OUT_YUYV);
#endif
	while (cinfo.next_scanline < cinfo.image_height)
	{
		row_pointer[0] = &image_buf[cinfo.next_scanline * row_stride];
		(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
	fclose(outfile);
}

/* Subtract the ‘struct timeval’ values X and Y,
storing the result in RESULT.
Return 1 if the difference is negative, otherwise 0. */
int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y)
{
	/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_usec < y->tv_usec) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000) {
		int nsec = (x->tv_usec - y->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}

	/* Compute the time remaining to wait.
	tv_usec is certainly positive. */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;

	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}

static int stream(char *buf, int nFrames)
{
	int err;
	static int count = 0;
	struct timeval tv1,tv2,tv3;

	/* Look till we hit our number of frames, or use hits the keyboard */
	while(nFrames-- || continuous_stream)
	{
		gettimeofday(&tv1, NULL);	/* record start time */

		/* Wait for a buffer */
		ceu_start_cap();

		while( ceu_is_capturing() )
		{
			//printf(".");
			usleep(1000);
		}

		gettimeofday(&tv2, NULL);	/* record end time */
		timeval_subtract(&tv3,&tv2,&tv1);
#ifdef DEBUG
		printf("capture time = %ld:%06ld sec\n", (long)tv3.tv_sec, (long)tv3.tv_usec);
#endif

		count++;
		/* Throw away the first 10or so frames because the camera needs
		   a couple captures to adjust exposure and such */

		if((count > THROW_AWAY_FRAMES) && savejpg)
			save_jpeg(buf);
		else
			nFrames++;

		if (displayfb0)
			display_on_fb0(buf, 2);

		/* Display JPEG iamge on LCD */
		if((count > THROW_AWAY_FRAMES) && savejpg && displayjpg)
		{
			if ( cap_width > lcd_width )
				system("fbv -f --noclear --noinfo --delay -1 capture0000.jpg");	// scale down
			else
				system("fbv --noclear --noinfo --delay -1 capture0000.jpg");
		}

		/* Stop capturing? */
		if (continuous_stream)
		{
			if ( kbhit() )
				break;
		}
	}

	jpeg_destroy_compress(&cinfo);

	return 0;
}

int main(int argc, char *argv[])
{
	int cap_cnt = 10;	/* default number of times to capture */
	int i;
	int resolution;
	int ret;

	if( argc < 2)
	{
		printf(
		"ov76_test: Capture images from a OV7670 camera and save a JPEG.\n"\
		"Usage: ceu_omni [-c] [-d] [-n] [-q] [count]  \n"\
		"  -v: Capture VGA (640x480) images (default).\n"\
		"  -q: Capture QVGA (320x240) images.\n"\
		"  -c: Continuous mode. Output file will always be capture0000.jpg\n"\
		"  -f: Display saved jpeg image on the LCD using fbv (will scale the image to fit the screen).\n"\
		"  -b: Display captured YCbCr422 on the LCD (/dev/fb0) by converting to RGB565 \n"\
		"  -y: Display captured YCbCr422 on the LCD directly (disables /dev/fb0, creates layers 0 as YCbCr)\n"\
		"  -n: Don't save JPEG images.\n"\
		"  -t: Quiet mode.\n"\
		" count: the number of images to caputre and save.\n"\
		);
		return -1;
	}

	i = 1;
	for( i=1; i < argc; i++)
	{
		if( !strcmp(argv[i],"-c") )
		{
			continuous_stream = 1;
		}
		else if( !strcmp(argv[i],"-v") )
		{
			cap_width = 640;
			cap_height = 480;
			vga_capture = 1;
		}
		else if( !strcmp(argv[i],"-q") )
		{
			cap_width = 320;
			cap_height = 240;
			vga_capture = 0;
		}
		else if( !strcmp(argv[i],"-b") )
		{
			displayfb0 = 1;
		}
		else if( !strcmp(argv[i],"-y") )
		{
			displayyuv = 1;
		}
		else if( !strcmp(argv[i],"-n") )
		{
			savejpg = 0;
		}
		else if( !strcmp(argv[i],"-f") )
		{
			displayjpg = 1;
		}
		else if( !strcmp(argv[i],"-t") )
		{
			quiet = 1;
		}
		else
		{
			// Must be a count
			sscanf(argv[i], "%u", &cap_cnt);
		}
	}

	if (displayfb0 && displayjpg)
	{
		printf("ERROR: You can't specify both -b and -f at the same time.\n");
		goto app_error;
	
	}

	/* Determine the board we are running on by looking at the model name in the device tree */
	if ( system("grep RSKRZA1 /sys/firmware/devicetree/base/model") == 0)
	{
		board = RZA1_RSKRZA1;
		cap_buf_addr = 0x60900000;	/* 9MB offset in internal RAM */
		i2c_dev_path[9] = '0';		/* "/dev/i2c-0" on CN40 */
	}
	else if ( system("grep STREAMIT /sys/firmware/devicetree/base/model") == 0)
	{
		board = RZA1_STREAMIT;
		cap_buf_addr = 0x60040000;	/* Directly after LCD frame buffer */
		i2c_dev_path[9] = '1';		/* "/dev/i2c-1 on CN12 */
	}
	else if ( system("grep RZA2MEVB /sys/firmware/devicetree/base/model") == 0)
	{
		board = RZA2_RZA2MEVB;
		cap_buf_addr = 0x80200000;	/* 2MB offset in internal RAM */
		i2c_dev_path[9] = '3';		/* "/dev/i2c-3 on CN17 */
	}
	else {
		printf("ERROR: Cannot detect what board we are running on.\n");
		goto app_error;
	}

	if (open_resources())
		goto app_error;

	if (cap_width == 640)
		resolution = CAM_RES_VGA; /* 0 = VGA */
	else if (cap_width == 320)
		resolution = CAM_RES_QVGA; /* 2 = QVGA */
	else
		printf("resolution not supported\n");


	/* Look for OV7670 */
	if( ov7670_open(i2c_dev_path) == 0 )
	{
		/* NOTE: 'CAM_OUT_YUYV' and 'CAM_OUT_YVYU' output the exact format. I have no idea why. */
		if( ov7670_set_format(CAM_FMT_YUV422, resolution, CAM_OUT_YUYV) )
			goto app_error;
	}
	/* Look for OV7740 */
	else if ( ov7740_open(i2c_dev_path) == 0)
	{
		if( ov7740_set_format(CAM_FMT_YUV422, resolution, CAM_OUT_YUYV) )
			goto app_error;
	}
	else
	{
		printf("no cammera found\n");
		goto app_error;
	}

	/* Set CEU registers for YUV422 capturing usign 'Data Synchronous Fetch Mode' */
	if( ceu_init(cap_width, cap_height, 1) )
		goto app_error;

	/* Set the address of our capture buffer */
	ceu_set_buffer_addr(map_ram.addr);

#ifdef DEBUG
	ceu_print_register();
#endif

	if (displayyuv)
	{
		FILE *layer0_fd, *layer2_fd, *restore_fd;

		layer0_fd = fopen("/sys/devices/platform/fcff7400.display/layer0", "r+");
		layer2_fd = fopen("/sys/devices/platform/fcff7400.display/layer2", "r+");

		if ( !layer0_fd || !layer2_fd)
		{
			printf("ERROR: Can't access /sys/devices/platform/fcff7400.display/layer2.\n");
			goto app_error;
		}

		/* Make Layer 2 (/dev/fb0) only 1x1 pixels */
		// $ echo xres = 1 , yres = 1 , blend = 1 > /sys/devices/platform/fcff7400.display/layer2
		fprintf(layer2_fd, "xres = 1 , yres = 1 , blend = 1\n");


		/* Enable Layer 0 as YCbCr with YCC_SWAP=0 (Cb/Y0/Cr/Y1) */
		/* Set Layer 0 data format to "Cb/Y0/Cr/Y1" to match OV7670 output */
		/* Set Layer 0 to swap data as it reads in "Swapped in 32-bit units + 16-bit units" to match OV7670 output */
		// $ echo xres = 640 , yres = 480 , base = 0x80200000 , bpp = 16 , format = YCbCr422_0 , read_swap = swap_32_16 > /sys/devices/platform/fcff7400.display/layer0
		fprintf(layer0_fd, "xres = %d , yres = %d , base = 0x%08X , bpp = 16 , format = YCbCr422_0 , read_swap = swap_32_16\n",
						cap_width, cap_height, cap_buf_addr);

		fclose(layer0_fd);
		fclose(layer2_fd);

		/* Make a file to restore Layer 2 back to normal */
		/* TO RESTORE: $ source /tmp/restore_layer2.txt */
		restore_fd = fopen("/tmp/restore_layer2.txt", "w");
		if (!restore_fd)
		{
			printf("ERROR: Can't create /tmp/restore_layer2.txt\n");
			goto app_error;
		}
		fprintf(restore_fd, "echo xres = %d , yres = %d , blend = 0 > /sys/devices/platform/fcff7400.display/layer2\n",
				lcd_width, lcd_height);

		fclose(restore_fd);
	}

	if( continuous_stream )
	{
		printf("***********************************************\n");
		printf("Continuous Stream Mode.\n");
		printf("Press ENTER at any time to exit.\n");
		printf("***********************************************\n");
		setbuf(stdout, NULL);	/* disable stdout buffering */
	}

	if (stream(map_ram.base, cap_cnt))
		goto app_error;

	/* successfull ending */
	ret = 0;
	goto app_exit;

	/* error ending */
app_error:
	ret = 1;


app_exit:
	ov7670_close();
	ov7740_close();
	close_resources();
	return ret;

}

