#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include "ov7670.h"

//#define DEBUG

/*
 * The default register settings, as obtained from OmniVision.  There
 * is really no making sense of most of these - lots of "reserved" values
 * and such.
 *
 * These settings give VGA YUYV.
 */

#define u32 uint32_t

#define OV7670_ADDR	0x21

static int ov7670_fd;
static int current_format;


struct regval_list {
	unsigned char reg_num;
	unsigned char value;
};

static struct regval_list ov7670_default_regs[] = {
//	{ REG_COM7, COM7_RESET },
/*
 * Clock scale: 3 = 15fps
 *              2 = 20fps
 *              1 = 30fps
 */
	{ REG_CLKRC, 0x1 },	/* OV: clock scale (30 fps) */
	{ REG_TSLB,  0x00 },	/* Set YUV orders (in combination with REG_COM13) */

	{REG_COM13, COM13_GAMMA | COM13_UVSAT}, /* Gamma enable, UV sat enable, UYVY or VYUY (see TSLB_REG) */

	{ REG_COM7, 0 },	/* VGA */


	/*
	 * Set the hardware window.  These values from OV don't entirely
	 * make sense - hstop is less than hstart.  But they work...
	 */
	{ REG_HSTART, 0x13 },	{ REG_HSTOP, 0x01 },
	{ REG_HREF, 0xb6 },	{ REG_VSTART, 0x02 },
	{ REG_VSTOP, 0x7a },	{ REG_VREF, 0x0a },

	{ REG_COM3, 0 },	{ REG_COM14, 0 },
	/* Mystery scaling numbers */
	{ 0x70, 0x3a },		{ 0x71, 0x35 },
	{ 0x72, 0x11 },		{ 0x73, 0xf0 },
	{ 0xa2, 0x02 },		{ REG_COM10, 0 },

	/* Gamma curve values */
	{ 0x7a, 0x20 },		{ 0x7b, 0x10 },
	{ 0x7c, 0x1e },		{ 0x7d, 0x35 },
	{ 0x7e, 0x5a },		{ 0x7f, 0x69 },
	{ 0x80, 0x76 },		{ 0x81, 0x80 },
	{ 0x82, 0x88 },		{ 0x83, 0x8f },
	{ 0x84, 0x96 },		{ 0x85, 0xa3 },
	{ 0x86, 0xaf },		{ 0x87, 0xc4 },
	{ 0x88, 0xd7 },		{ 0x89, 0xe8 },

	/* AGC and AEC parameters.  Note we start by disabling those features,
	   then turn them only after tweaking the values. */
	{ REG_COM8, COM8_FASTAEC | COM8_AECSTEP | COM8_BFILT },
	{ REG_GAIN, 0 },	{ REG_AECH, 0 },
	{ REG_COM4, 0x40 }, /* magic reserved bit */
	{ REG_COM9, 0x18 }, /* 4x gain + magic rsvd bit */
	{ REG_BD50MAX, 0x05 },	{ REG_BD60MAX, 0x07 },
	{ REG_AEW, 0x95 },	{ REG_AEB, 0x33 },
	{ REG_VPT, 0xe3 },	{ REG_HAECC1, 0x78 },
	{ REG_HAECC2, 0x68 },	{ 0xa1, 0x03 }, /* magic */
	{ REG_HAECC3, 0xd8 },	{ REG_HAECC4, 0xd8 },
	{ REG_HAECC5, 0xf0 },	{ REG_HAECC6, 0x90 },
	{ REG_HAECC7, 0x94 },
	{ REG_COM8, COM8_FASTAEC|COM8_AECSTEP|COM8_BFILT|COM8_AGC|COM8_AEC },

	/* Almost all of these are magic "reserved" values.  */
	{ REG_COM5, 0x61 },	{ REG_COM6, 0x4b },
	{ 0x16, 0x02 },		{ REG_MVFP, 0x07 },
	{ 0x21, 0x02 },		{ 0x22, 0x91 },
	{ 0x29, 0x07 },		{ 0x33, 0x0b },
	{ 0x35, 0x0b },		{ 0x37, 0x1d },
	{ 0x38, 0x71 },		{ 0x39, 0x2a },
	{ REG_COM12, 0x78 },	{ 0x4d, 0x40 },
	{ 0x4e, 0x20 },		{ REG_GFIX, 0 },
	{ 0x6b, 0x4a },		{ 0x74, 0x10 },
	{ 0x8d, 0x4f },		{ 0x8e, 0 },
	{ 0x8f, 0 },		{ 0x90, 0 },
	{ 0x91, 0 },		{ 0x96, 0 },
	{ 0x9a, 0 },		{ 0xb0, 0x84 },
	{ 0xb1, 0x0c },		{ 0xb2, 0x0e },
	{ 0xb3, 0x82 },		{ 0xb8, 0x0a },

	/* More reserved magic, some of which tweaks white balance */
	{ 0x43, 0x0a },		{ 0x44, 0xf0 },
	{ 0x45, 0x34 },		{ 0x46, 0x58 },
	{ 0x47, 0x28 },		{ 0x48, 0x3a },
	{ 0x59, 0x88 },		{ 0x5a, 0x88 },
	{ 0x5b, 0x44 },		{ 0x5c, 0x67 },
	{ 0x5d, 0x49 },		{ 0x5e, 0x0e },
	{ 0x6c, 0x0a },		{ 0x6d, 0x55 },
	{ 0x6e, 0x11 },		{ 0x6f, 0x9f }, /* "9e for advance AWB" */
	{ 0x6a, 0x40 },		{ REG_BLUE, 0x40 },
	{ REG_RED, 0x60 },
	{ REG_COM8, COM8_FASTAEC|COM8_AECSTEP|COM8_BFILT|COM8_AGC|COM8_AEC|COM8_AWB },

	/* Matrix coefficients */
	{ 0x4f, 0x80 },		{ 0x50, 0x80 },
	{ 0x51, 0 },		{ 0x52, 0x22 },
	{ 0x53, 0x5e },		{ 0x54, 0x80 },
	{ 0x58, 0x9e },

	{ REG_COM16, COM16_AWBGAIN },	{ REG_EDGE, 0 },
	{ 0x75, 0x05 },		{ 0x76, 0xe1 },
	{ 0x4c, 0 },		{ 0x77, 0x01 },
	{ REG_COM13, 0xc3 },	{ 0x4b, 0x09 },
	{ 0xc9, 0x60 },		{ REG_COM16, 0x38 },
	{ 0x56, 0x40 },

	{ 0x34, 0x11 },		{ REG_COM11, COM11_EXP|COM11_HZAUTO },
	{ 0xa4, 0x88 },		{ 0x96, 0 },
	{ 0x97, 0x30 },		{ 0x98, 0x20 },
	{ 0x99, 0x30 },		{ 0x9a, 0x84 },
	{ 0x9b, 0x29 },		{ 0x9c, 0x03 },
	{ 0x9d, 0x4c },		{ 0x9e, 0x3f },
	{ 0x78, 0x04 },

	/* Extra-weird stuff.  Some sort of multiplexor register */
	{ 0x79, 0x01 },		{ 0xc8, 0xf0 },
	{ 0x79, 0x0f },		{ 0xc8, 0x00 },
	{ 0x79, 0x10 },		{ 0xc8, 0x7e },
	{ 0x79, 0x0a },		{ 0xc8, 0x80 },
	{ 0x79, 0x0b },		{ 0xc8, 0x01 },
	{ 0x79, 0x0c },		{ 0xc8, 0x0f },
	{ 0x79, 0x0d },		{ 0xc8, 0x20 },
	{ 0x79, 0x09 },		{ 0xc8, 0x80 },
	{ 0x79, 0x02 },		{ 0xc8, 0xc0 },
	{ 0x79, 0x03 },		{ 0xc8, 0x40 },
	{ 0x79, 0x05 },		{ 0xc8, 0x30 },
	{ 0x79, 0x26 },

	{ 0xff, 0xff },	/* END MARKER */
};

/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 *
 * RGB656 and YUV422 come from OV; RGB444 is homebrewed.
 *
 * IMPORTANT RULE: the first entry must be for COM7, see ov7670_s_fmt for why.
 */


static struct regval_list ov7670_fmt_yuv422[] = {
	{ REG_COM7, 0x0 },  /* Selects YUV mode */
	{ REG_RGB444, 0 },	/* No RGB444 please */
	{ REG_COM1, 0 },	/* CCIR601 */
	{ REG_COM15, COM15_R00FF },
#if 0 //chris
	{ REG_COM9, 0x48 }, /* 32x gain ceiling; 0x8 is reserved bit */
#else
	{ REG_COM9, 0x18 }, /* 4x gain ceiling; 0x8 is reserved bit */
#endif
	{ 0x4f, 0x80 }, 	/* "matrix coefficient 1" */
	{ 0x50, 0x80 }, 	/* "matrix coefficient 2" */
	{ 0x51, 0    },		/* vb */
	{ 0x52, 0x22 }, 	/* "matrix coefficient 4" */
	{ 0x53, 0x5e }, 	/* "matrix coefficient 5" */
	{ 0x54, 0x80 }, 	/* "matrix coefficient 6" */
	{ REG_COM13, COM13_GAMMA|COM13_UVSAT },
	{ 0xff, 0xff },
};

static struct regval_list ov7670_fmt_rgb565[] = {
	{ REG_COM7, COM7_RGB },	/* Selects RGB mode */
	{ REG_RGB444, 0 },	/* No RGB444 please */
	{ REG_COM1, 0x0 },	/* CCIR601 */
	{ REG_COM15, COM15_RGB565 },
	{ REG_COM9, 0x38 }, 	/* 16x gain ceiling; 0x8 is reserved bit */
	{ 0x4f, 0xb3 }, 	/* "matrix coefficient 1" */
	{ 0x50, 0xb3 }, 	/* "matrix coefficient 2" */
	{ 0x51, 0    },		/* vb */
	{ 0x52, 0x3d }, 	/* "matrix coefficient 4" */
	{ 0x53, 0xa7 }, 	/* "matrix coefficient 5" */
	{ 0x54, 0xe4 }, 	/* "matrix coefficient 6" */
	{ REG_COM13, COM13_GAMMA|COM13_UVSAT },
	{ 0xff, 0xff },
};

static struct regval_list ov7670_fmt_rgb444[] = {
	{ REG_COM7, COM7_RGB },	/* Selects RGB mode */
	{ REG_RGB444, R444_ENABLE },	/* Enable xxxxrrrr ggggbbbb */
	{ REG_COM1, 0x0 },	/* CCIR601 */
	{ REG_COM15, COM15_R01FE|COM15_RGB565 }, /* Data range needed? */
	{ REG_COM9, 0x38 }, 	/* 16x gain ceiling; 0x8 is reserved bit */
	{ 0x4f, 0xb3 }, 	/* "matrix coefficient 1" */
	{ 0x50, 0xb3 }, 	/* "matrix coefficient 2" */
	{ 0x51, 0    },		/* vb */
	{ 0x52, 0x3d }, 	/* "matrix coefficient 4" */
	{ 0x53, 0xa7 }, 	/* "matrix coefficient 5" */
	{ 0x54, 0xe4 }, 	/* "matrix coefficient 6" */
	{ REG_COM13, COM13_GAMMA|COM13_UVSAT|0x2 },  /* Magic rsvd bit */
	{ 0xff, 0xff },
};

static struct regval_list ov7670_fmt_raw[] = {
	{ REG_COM7, COM7_BAYER },
	{ REG_COM13, 0x08 }, /* No gamma, magic rsvd bit */
	{ REG_COM16, 0x3d }, /* Edge enhancement, denoise */
	{ REG_REG76, 0xe1 }, /* Pix correction, magic rsvd */
	{ 0xff, 0xff },
};


static int ov7670_read(unsigned char reg, unsigned char *value)
{
	int ret;

	if( write(ov7670_fd, &reg, 1) != 1) {
#ifdef DEBUG
		printf("ov7670_read error (setting reg=0x%X)\n",reg);
#endif
		return -1;
	}
	if( read(ov7670_fd, value, 1) != 1) {
#ifdef DEBUG
		printf("ov7670_read error (read reg=0x%X)\n",reg);
		return -1;
#endif
	}
	return 0;	
}

static int ov7670_write(unsigned char reg, unsigned char value)
{
	uint8_t data[2];

	data[0] = reg;
	data[1] = value;

	if( write(ov7670_fd, data, 2) != 2) {
#ifdef DEBUG
		printf("ov7670_write (reg=0x%X)\n",reg);
#endif
		return -1;
	}

	return 0;
}


/*
 * Write a list of register settings; ff/ff stops the process.
 */
static int ov7670_write_array(struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != 0xff || vals->value != 0xff) {
		ret = ov7670_write(vals->reg_num, vals->value);
		if (ret < 0)
			return ret;
		vals++;
	}
	return 0;
}

static int ov7670_reset(u32 val)
{
	ov7670_write(REG_COM7, COM7_RESET);
	usleep(1000);

	/* Release reset */
	ov7670_write(REG_COM7, COM7_FMT_VGA);
	usleep(1000);

	return 0;
}

static int ov7670_detect(void)
{
	unsigned char v;
	int ret;

	ret = ov7670_read(REG_MIDH, &v);
	if (ret < 0)
		return ret;
	if (v != 0x7f) { /* OV manuf. id. */
		//printf("REG_MIDH = 0x02X\n", v);
		return -1;
	}
	ret = ov7670_read(REG_MIDL, &v);
	if (ret < 0)
		return ret;
	if (v != 0xa2) {
		//printf("REG_MIDL = 0x02X\n", v);
		return -1;
	}
	/*
	 * OK, we know we have an OmniVision chip...but which one?
	 */
	ret = ov7670_read(REG_PID, &v);
	if (ret < 0)
		return ret;
	if (v != 0x76) {  /* PID + VER = 0x76 / 0x73 */
		//printf("REG_PID = 0x02X\n", v);
		return -1;
	}
	ret = ov7670_read(REG_VER, &v);
	if (ret < 0)
		return ret;
	if (v != 0x73) { /* PID + VER = 0x76 / 0x73 */
		//printf("REG_PID = 0x02X\n", v);
		return -1;
	}

	return 0;
}

/*
 * QCIF mode is done (by OV) in a very strange way - it actually looks like
 * VGA with weird scaling options - they do *not* use the canned QCIF mode
 * which is allegedly provided by the sensor.  So here's the weird register
 * settings.
 */
static struct regval_list ov7670_qcif_regs[] = {
	{ REG_COM3, COM3_SCALEEN|COM3_DCWEN },
	{ REG_COM3, COM3_DCWEN },
	{ REG_COM14, COM14_DCWEN | 0x01},
	{ 0x73, 0xf1 },
	{ 0xa2, 0x52 },
	{ 0x7b, 0x1c },
	{ 0x7c, 0x28 },
	{ 0x7d, 0x3c },
	{ 0x7f, 0x69 },
	{ REG_COM9, 0x38 },
	{ 0xa1, 0x0b },
	{ 0x74, 0x19 },
	{ 0x9a, 0x80 },
	{ 0x43, 0x14 },
	{ REG_COM13, 0xc0 },
	{ 0xff, 0xff },
};

/*
 * Store information about the video data format.  The color matrix
 * is deeply tied into the format, so keep the relevant values here.
 * The magic matrix numbers come from OmniVision.
 */
static struct ov7670_format_struct {
	struct regval_list *regs;
	int cmatrix[CMATRIX_LEN];
} ov7670_formats[] = {
	{
		//.mbus_code	= V4L2_MBUS_FMT_YUYV8_2X8,
		//.colorspace	= V4L2_COLORSPACE_JPEG,
		.regs 		= ov7670_fmt_yuv422,
		.cmatrix	= { 128, -128, 0, -34, -94, 128 },
	},
	{
		//.mbus_code	= V4L2_MBUS_FMT_RGB444_2X8_PADHI_LE,
		//.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs		= ov7670_fmt_rgb444,
		.cmatrix	= { 179, -179, 0, -61, -176, 228 },
	},
	{
		//.mbus_code	= V4L2_MBUS_FMT_RGB565_2X8_LE,
		//.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs		= ov7670_fmt_rgb565,
		.cmatrix	= { 179, -179, 0, -61, -176, 228 },
	},
	{
		//.mbus_code	= V4L2_MBUS_FMT_SBGGR8_1X8,
		//.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= ov7670_fmt_raw,
		.cmatrix	= { 0, 0, 0, 0, 0, 0 },
	},
};
#define N_OV7670_FMTS ARRAY_SIZE(ov7670_formats)


struct ov7670_win_size {
	int	width;
	int	height;
	unsigned char com7_bit;
	int	hstart;		/* Start/stop values for the camera.  Note */
	int	hstop;		/* that they do not always make complete */
	int	vstart;		/* sense to humans, but evidently the sensor */
	int	vstop;		/* will do the right thing... */
	struct regval_list *regs; /* Regs to tweak */
};


static struct ov7670_win_size ov7670_win_sizes[] = {
	/* VGA */
	{
		.width		= VGA_WIDTH,
		.height		= VGA_HEIGHT,
		.com7_bit	= COM7_FMT_VGA,
		.hstart		= 158,	/* These values from */
		.hstop		=  14,	/* Omnivision */
		.vstart		=  10,
		.vstop		= 490,
		.regs		= NULL,
	},
	/* CIF */
	{
		.width		= CIF_WIDTH,
		.height		= CIF_HEIGHT,
		.com7_bit	= COM7_FMT_CIF,
		.hstart		= 170,	/* Empirically determined */
		.hstop		=  90,
		.vstart		=  14,
		.vstop		= 494,
		.regs		= NULL,
	},
	/* QVGA */
	{
		.width		= QVGA_WIDTH,
		.height		= QVGA_HEIGHT,
		.com7_bit	= COM7_FMT_QVGA,
		.hstart		= 168,	/* Empirically determined */
		.hstop		=  24,
		.vstart		=  12,
		.vstop		= 492,
		.regs		= NULL,
	},
	/* QCIF */
	{
		.width		= QCIF_WIDTH,
		.height		= QCIF_HEIGHT,
		.com7_bit	= COM7_FMT_VGA, /* see comment above */
		.hstart		= 456,	/* Empirically determined */
		.hstop		=  24,
		.vstart		=  14,
		.vstop		= 494,
		.regs		= ov7670_qcif_regs,
	}
};

static struct ov7670_win_size ov7675_win_sizes[] = {
	/*
	 * Currently, only VGA is supported. Theoretically it could be possible
	 * to support CIF, QVGA and QCIF too. Taking values for ov7670 as a
	 * base and tweak them empirically could be required.
	 */
	{
		.width		= VGA_WIDTH,
		.height		= VGA_HEIGHT,
		.com7_bit	= COM7_FMT_VGA,
		.hstart		= 158,	/* These values from */
		.hstop		=  14,	/* Omnivision */
		.vstart		=  14,  /* Empirically determined */
		.vstop		= 494,
		.regs		= NULL,
	}
};

/*
 * Store a set of start/stop values into the camera.
 */
static int ov7670_set_hw(int hstart, int hstop,	int vstart, int vstop)
{
	int ret;
	unsigned char v;
/*
 * Horizontal: 11 bits, top 8 live in hstart and hstop.  Bottom 3 of
 * hstart are in href[2:0], bottom 3 of hstop in href[5:3].  There is
 * a mystery "edge offset" value in the top two bits of href.
 */
	ret =  ov7670_write(REG_HSTART, (hstart >> 3) & 0xff);
	ret += ov7670_write(REG_HSTOP, (hstop >> 3) & 0xff);
	ret += ov7670_read(REG_HREF, &v);
	v = (v & 0xc0) | ((hstop & 0x7) << 3) | (hstart & 0x7);
	usleep(1000);
	ret += ov7670_write(REG_HREF, v);
/*
 * Vertical: similar arrangement, but only 10 bits.
 */
	ret += ov7670_write(REG_VSTART, (vstart >> 2) & 0xff);
	ret += ov7670_write(REG_VSTOP, (vstop >> 2) & 0xff);
	ret += ov7670_read(REG_VREF, &v);
	v = (v & 0xf0) | ((vstop & 0x3) << 2) | (vstart & 0x3);
	usleep(1000);
	ret += ov7670_write(REG_VREF, v);
	return ret;
}

/* image_format:
 *  0 = YUV422
 *  1 = RGB444
 *  2 = RGB565
 *  3 = RAW
 * 
 * resolution:
 *  0 = VGA (640x480)
 *  1 = CIF (352x288)
 *  2 = QVGA (320x240)
 *  3 = QCIF (176x144)
 *
 * out_format 
 *  0 = YUYV (Y/Cb/Y/Cr)
 *  1 = YVYU (Y/Cr/Y/Cb)
 *  2 = UYVY (Cb/Y/Cr/Y)
 *  3 = VYUY (Cr/Y/Cb/Y)
 */
int ov7670_set_format( int image_format, int resolution, int out_format)
{
	struct ov7670_win_size *wsize;
	unsigned char com7;
	int ret;
	struct regval_list *regs;
	uint8_t com13, tslb;

	regs = ov7670_formats[image_format].regs;
	current_format = image_format;

	wsize = ov7670_win_sizes + resolution;

	/*
	 * COM7 is a pain in the ass, it doesn't like to be read then
	 * quickly written afterward.  But we have everything we need
	 * to set it absolutely here, as long as the format-specific
	 * register sets list it first.
	 */
	com7 = regs[0].value;
	com7 |= wsize->com7_bit;
	ov7670_write(REG_COM7, com7);
	/*
	 * Now write the rest of the array.  Also store start/stops
	 */
	ov7670_write_array(regs + 1);
	ov7670_set_hw(wsize->hstart, wsize->hstop, wsize->vstart,
			wsize->vstop);
	ret = 0;
	if (wsize->regs)
		ret = ov7670_write_array(wsize->regs);

	ov7670_read( REG_COM13, &com13);
	ov7670_read( REG_TSLB, &tslb);
	com13 &= ~2;	// clear bit 1
	tslb &= ~8;	// clear bit 3

	// TSLB[3] COM13[1]
	//   0       0      Y U Y V (Y/Cb/Y/Cr)
	//   0       1      Y V Y U (Y/Cr/Y/Cb)
	//   1       0      U Y V Y (Cb/Y/Cr/Y)
	//   1       1      V Y U Y (Cr/Y/Cb/Y)
	switch (out_format) 
	{
		case 0:
			break;
		case 1:	com13 |= 2;
			break;
		case 2:	tslb |= 8;
			break;
		case 3:	com13 |= 2;
			tslb |= 8;
			break;
		default:
			printf("%s: bad out_format\n", __func__);
			return -1;
			break;

	}
	ov7670_write( REG_COM13, com13);
	ov7670_write( REG_TSLB, tslb);

	/*
	 * If we're running RGB565, we must rewrite clkrc after setting
	 * the other parameters or the image looks poor.  If we're *not*
	 * doing RGB565, we must not rewrite clkrc or the image looks
	 * *really* poor.
	 *
	 * (Update) Now that we retain clkrc state, we should be able
	 * to write it unconditionally, and that will make the frame
	 * rate persistent too.
	 */
	if (ret == 0)
		//ret = ov7670_write(REG_CLKRC, info->clkrc);
		ret = ov7670_write(REG_CLKRC, 1);

	return 0;
}

static unsigned char ov7670_abs_to_sm(unsigned char v)
{
	if (v > 127)
		return v & 0x7f;
	return (128 - v) | 0x80;
}

static int ov7670_s_brightness(int value)
{
	unsigned char com8 = 0, v;
	int ret;

	ov7670_read(REG_COM8, &com8);
	com8 &= ~COM8_AEC;
	ov7670_write(REG_COM8, com8);
	v = ov7670_abs_to_sm(value);
	ret = ov7670_write(REG_BRIGHT, v);
	return ret;
}

static int ov7670_s_contrast(int value)
{
	return ov7670_write(REG_CONTRAS, (unsigned char) value);
}
static int ov7670_s_hflip(int value)
{
	unsigned char v = 0;
	int ret;

	ret = ov7670_read(REG_MVFP, &v);
	if (value)
		v |= MVFP_MIRROR;
	else
		v &= ~MVFP_MIRROR;
	usleep(1000);  /* FIXME */
	ret += ov7670_write(REG_MVFP, v);
	return ret;
}

static int ov7670_s_vflip(int value)
{
	unsigned char v = 0;
	int ret;

	ret = ov7670_read(REG_MVFP, &v);
	if (value)
		v |= MVFP_FLIP;
	else
		v &= ~MVFP_FLIP;
	usleep(1000);  /* FIXME */
	ret += ov7670_write(REG_MVFP, v);
	return ret;
}

static int ov7670_store_cmatrix(int matrix[CMATRIX_LEN])
{
	int i, ret;
	unsigned char signbits = 0;

	/*
	 * Weird crap seems to exist in the upper part of
	 * the sign bits register, so let's preserve it.
	 */
	ret = ov7670_read(REG_CMATRIX_SIGN, &signbits);
	signbits &= 0xc0;

	for (i = 0; i < CMATRIX_LEN; i++) {
		unsigned char raw;

		if (matrix[i] < 0) {
			signbits |= (1 << i);
			if (matrix[i] < -255)
				raw = 0xff;
			else
				raw = (-1 * matrix[i]) & 0xff;
		}
		else {
			if (matrix[i] > 255)
				raw = 0xff;
			else
				raw = matrix[i] & 0xff;
		}
		ret += ov7670_write(REG_CMATRIX_BASE + i, raw);
	}
	ret += ov7670_write(REG_CMATRIX_SIGN, signbits);
	return ret;
}

/*
 * Hue also requires messing with the color matrix.  It also requires
 * trig functions, which tend not to be well supported in the kernel.
 * So here is a simple table of sine values, 0-90 degrees, in steps
 * of five degrees.  Values are multiplied by 1000.
 *
 * The following naive approximate trig functions require an argument
 * carefully limited to -180 <= theta <= 180.
 */
#define SIN_STEP 5
static const int ov7670_sin_table[] = {
	   0,	 87,   173,   258,   342,   422,
	 499,	573,   642,   707,   766,   819,
	 866,	906,   939,   965,   984,   996,
	1000
};

static int ov7670_sine(int theta)
{
	int chs = 1;
	int sine;

	if (theta < 0) {
		theta = -theta;
		chs = -1;
	}
	if (theta <= 90)
		sine = ov7670_sin_table[theta/SIN_STEP];
	else {
		theta -= 90;
		sine = 1000 - ov7670_sin_table[theta/SIN_STEP];
	}
	return sine*chs;
}

static int ov7670_cosine(int theta)
{
	theta = 90 - theta;
	if (theta > 180)
		theta -= 360;
	else if (theta < -180)
		theta += 360;
	return ov7670_sine(theta);
}

static void ov7670_calc_cmatrix(int matrix[CMATRIX_LEN], int sat, int hue)
{
	int i;
	/*
	 * Apply the current saturation setting first.
	 */
	for (i = 0; i < CMATRIX_LEN; i++)
		matrix[i] = (ov7670_formats[current_format].cmatrix[i] * sat) >> 7;


	/*
	 * Then, if need be, rotate the hue value.
	 */
	if (hue != 0) {
		int sinth, costh, tmpmatrix[CMATRIX_LEN];

		memcpy(tmpmatrix, matrix, CMATRIX_LEN*sizeof(int));
		sinth = ov7670_sine(hue);
		costh = ov7670_cosine(hue);

		matrix[0] = (matrix[3]*sinth + matrix[0]*costh)/1000;
		matrix[1] = (matrix[4]*sinth + matrix[1]*costh)/1000;
		matrix[2] = (matrix[5]*sinth + matrix[2]*costh)/1000;
		matrix[3] = (matrix[3]*costh - matrix[0]*sinth)/1000;
		matrix[4] = (matrix[4]*costh - matrix[1]*sinth)/1000;
		matrix[5] = (matrix[5]*costh - matrix[2]*sinth)/1000;
	}
}


// SATURATION: 0 to 256, def=128
// SATURATION: -180 to 180, def=0

static int ov7670_s_sat_hue(int sat, int hue)
{
	int matrix[CMATRIX_LEN];
	int ret;

	ov7670_calc_cmatrix(matrix, sat, hue);
	ret = ov7670_store_cmatrix(matrix);
	return ret;
}

static int ov7670_s_autoexp(int value)
{
	int ret;
	unsigned char com8;

	ret = ov7670_read(REG_COM8, &com8);
	if (ret == 0) {
		if (value)
			com8 |= COM8_AEC;
		else
			com8 &= ~COM8_AEC;
		ret = ov7670_write(REG_COM8, com8);
	}
	return ret;
}

void ov7670_print_registers(void)
{
	int i;
	uint8_t value;

	printf("Camera Registers\n",i, value);
	for (i=0;i<=0x9C;i++) {
		ov7670_read(i, &value);
		printf("0x%02X = 0x%02X\n",i, value);
	}
}

int ov7670_open(char *i2c_dev_path)
{
	int ret;
	int i;
	uint8_t value;

	ov7670_fd = open(i2c_dev_path, O_RDWR);
	if (ov7670_fd < 0) {
		printf("ov7670_open");
		return -1;
	}

	if (ioctl(ov7670_fd, I2C_SLAVE, OV7670_ADDR) < 0) {
		printf("%s: I2C_SLAVE failed\n", __func__);
		return -1;
	}

	/* Make sure it's an ov7670 */
	ret = ov7670_detect();
	if (ret) {
#ifdef DEBUG
		printf("chip found, but is not an ov7670 chip.\n");
#endif
		return ret;
	}

	ret = ov7670_write_array(ov7670_default_regs);
	if (ret < 0) {
		printf("%s: register init failed\n", __func__);
		return ret;
	}

	ov7670_s_brightness(128);	/* 0 to 255 (128) */
	ov7670_s_contrast(64);		/* 0 to 127 (64) */

	ov7670_s_hflip(0);	/* 0 to 1 (0) */
	ov7670_s_vflip(0);	/* 0 to 1 (0) */

	ov7670_s_sat_hue(128, 0);
	
	ov7670_s_autoexp(0);
	ov7670_s_autoexp(1);

	return 0;
}
int ov7670_close(void)
{
	close(ov7670_fd);
	ov7670_fd = 0;
	
	return 0;
}
