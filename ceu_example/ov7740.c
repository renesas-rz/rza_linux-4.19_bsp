#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include "ov7740.h"

//#define DEBUG


//#define OV7740_ADDR (0x42) /* The 7740 sits on i2c with ID 0x42 */
#define OV7740_ADDR 0x21

#define VGA_WIDTH              (640)
#define VGA_HEIGHT             (480)

#define TOTAL_LINE_WIDTH    (784)

#define Y_SIZE                 (320*240)
#define UV_SIZE             (Y_SIZE/2)

typedef struct
{
    uint8_t addr;
    uint8_t val;
} omni_register_t;

int ov7740_fd;

static int ov7740_read(unsigned char reg, unsigned char *value)
{
	int ret;

	if( write(ov7740_fd, &reg, 1) != 1) {
#ifdef DEBUG
		printf("ov7740_read error (setting reg=0x%X)\n",reg);
#endif
		return -1;
	}
	if( read(ov7740_fd, value, 1) != 1) {
#ifdef DEBUG
		printf("ov7740_read error (read reg=0x%X)\n",reg);
		return -1;
#endif
	}
	return 0;	
}

static int ov7740_write(unsigned char reg, unsigned char value)
{
	uint8_t data[2];

	data[0] = reg;
	data[1] = value;

	if( write(ov7740_fd, data, 2) != 2) {
#ifdef DEBUG
		printf("ov7740_write (reg=0x%X)\n",reg);
#endif
		return -1;
	}

	return 0;
}

#define REG_PIDH		0x0a	/* Product ID MSB */
#define REG_PIDL		0x0b	/* Product ID LSB */
#define REG_MIDH	0x1c	/* Manuf. ID high */
#define REG_MIDL	0x1d	/* Manuf. ID low */

static int ov7740_detect(void)
{
	unsigned char v;
	int ret;

	ret = ov7740_read(REG_MIDH, &v);
	if (ret < 0)
		return ret;
	if (v != 0x7f) { /* OV manuf. id. */
		printf("REG_MIDH = 0x%02X\n", v);
		return -1;
	}
	ret = ov7740_read(REG_MIDL, &v);
	if (ret < 0)
		return ret;
	if (v != 0xa2) {
		printf("REG_MIDL = 0x%02X\n", v);
		return -1;
	}
	/*
	 * OK, we know we have an OmniVision chip...but which one?
	 */
	ret = ov7740_read(REG_PIDH, &v);
	if (ret < 0)
		return ret;
	if (v != 0x77) {  /* PID + VER = 0x76 / 0x73 */
		printf("REG_PIDH = 0x%02X\n", v);
		return -1;
	}
	ret = ov7740_read(REG_PIDL, &v);
	if (ret < 0)
		return ret;
	if (v != 0x42) { /* PID + VER = 0x76 / 0x73 */
		printf("REG_PIDL = 0x%02X\n", v);
		return -1;
	}

	return 0;
}

int ov7740_open(char *i2c_dev_path)
{
	int ret;
	int i;
	uint8_t value;

	ov7740_fd = open(i2c_dev_path, O_RDWR);
	if (ov7740_fd < 0) {
		return -1;
	}

	if (ioctl(ov7740_fd, I2C_SLAVE, OV7740_ADDR) < 0) {
		printf("%s: I2C_SLAVE failed\n", __func__);
		return -1;
	}

	/* Make sure it's an ov7740 */
	ret = ov7740_detect();
	if (ret) {
#ifdef DEBUG
		printf("OV7740 not found on bus %s\n",i2c_dev_path);
#endif
		return ret;
	}

	return 0;
}


/* For more information on the OV7740 camera, please visit the Omnivision web site */
/* http://www.ovt.com */

/* Initialisation sequence for VGA resolution (640x480) */
static const omni_register_t ov7740_reset_regs_vga[] =
{
    /* {register, value} */
    {0x04, 0x60},       // "reserved"
    {0x0c, 0x12},       // REG0C (YUV swap) (the spec is wrong)
//    {0x0c, 0x02},       // REG0C
    {0x0d, 0x34},       // "reserved"
    {0x11, 0x03},       // CLK
    {0x12, 0x00},       // REG12
    {0x13, 0xff},       // REG13
    {0x14, 0x38},       // REG14
    {0x17, 0x24},       // AHSTART
    {0x18, 0xa0},       // AHSIZE
    {0x19, 0x03},       // AVSTART
    {0x1a, 0xf0},       // AVSIZE
    {0x1b, 0x85},       // PSHFT
    {0x1e, 0x13},       // REG1E
    {0x20, 0x00},       // "reserved"
    {0x21, 0x23},       // "reserved"
    {0x22, 0x03},       // "reserved"
    {0x24, 0x3c},       // WPT
    {0x25, 0x30},       // BPT
    {0x26, 0x72},       // VPT
    {0x27, 0x80},       // REG27
    {0x29, 0x17},       // REG29
    {0x2b, 0xf8},       // REG2B
    {0x2c, 0x01},       // REG2C
    {0x31, 0xa0},       // HOUTSIZE
    {0x32, 0xf0},       // VOUTSIZE
    {0x33, 0xc4},       // "reserved"
    {0x36, 0x3f},       // "reserved"
    {0x38, 0x11},       // REG38 - see OV7740 data sheet
    {0x3a, 0xb4},       // "reserved"
    {0x3d, 0x0f},       // "reserved"
    {0x3e, 0x82},       // "reserved"
    {0x3f, 0x40},       // "reserved"
    {0x40, 0x7f},       // "reserved"
    {0x41, 0x6a},       // "reserved"
    {0x42, 0x29},       // "reserved"
    {0x44, 0xe5},       // "reserved"
    {0x45, 0x41},       // "reserved"
    {0x47, 0x42},       // "reserved"
    {0x48, 0x00},       // "reserved"
    {0x49, 0x61},       // "reserved"
    {0x4a, 0xa1},       // "reserved"
    {0x4b, 0x46},       // "reserved"
    {0x4c, 0x18},       // "reserved"
    {0x4d, 0x50},       // "reserved"
    {0x4e, 0x13},       // "reserved"
    {0x50, 0x97},       // REG50
    {0x51, 0x7e},       // REG51
    {0x52, 0x00},       // REG52
    {0x53, 0x00},       // "reserved"
    {0x55, 0x42},       // "reserved"
    {0x56, 0x55},       // REG56
    {0x57, 0xff},       // REG57
    {0x58, 0xff},       // REG58
    {0x59, 0xff},       // REG59
    {0x5a, 0x24},       // "reserved"
    {0x5b, 0x1f},       // "reserved"
    {0x5c, 0x88},       // "reserved"
    {0x5d, 0x60},       // "reserved"
    {0x5f, 0x04},       // "reserved"
    {0x64, 0x00},       // "reserved"
    {0x67, 0x88},       // REG67
    {0x68, 0x1a},       // "reserved"
    {0x69, 0x08},       // REG69
    {0x70, 0x00},       // "reserved"
    {0x71, 0x34},       // "reserved"
    {0x74, 0x28},       // "reserved"
    {0x75, 0x98},       // "reserved"
    {0x76, 0x00},       // "reserved"
    {0x77, 0x08},       // "reserved"
    {0x78, 0x01},       // "reserved"
    {0x79, 0xc2},       // "reserved"
    {0x7a, 0x9c},       // "reserved"
    {0x7b, 0x40},       // "reserved"
    {0x7c, 0x0c},       // "reserved"
    {0x7d, 0x02},       // "reserved"
    {0x80, 0x7f},       // ISP CTRL00
    {0x81, 0x3f},       // ISP CTRL01
    {0x82, 0x32},       // ISP CTRL02
    {0x83, 0x03},       // ISP CTRL03
    {0x84, 0x70},       // REG84 - see OV7740 data sheet
    {0x85, 0x00},       // AGC OFFSET
    {0x86, 0x03},       // AGC BASE1
    {0x87, 0x01},       // AGC BASE2
    {0x88, 0x05},       // AGC CTRL
    {0x89, 0x30},       // LENC CTRL
    {0x8d, 0x40},       // LENC RED A1
    {0x8e, 0x00},       // LENC RED B1
    {0x8f, 0x33},       // LENC RED AB2
    {0x93, 0x28},       // LENC GREEN A1
    {0x94, 0x20},       // LENC GREEN B1
    {0x95, 0x33},       // LENC GREEN AB2
    {0x99, 0x30},       // LENC BLUE A1
    {0x9a, 0x14},       // LENC BLUE B1
    {0x9b, 0x33},       // LENC BLUE AB2
    {0x9c, 0x08},       // GMA YST01
    {0x9d, 0x12},       // GMA YST02
    {0x9e, 0x23},       // GMA YST03
    {0x9f, 0x45},       // GMA YST04
    {0xa0, 0x55},       // GMA YST05
    {0xa1, 0x64},       // GMA YST06
    {0xa2, 0x72},       // GMA YST07
    {0xa3, 0x7f},       // GMA YST08
    {0xa4, 0x8b},       // GMA YST09
    {0xa5, 0x95},       // GMA YST10
    {0xa6, 0xa7},       // GMA YST11
    {0xa7, 0xb5},       // GMA YST12
    {0xa8, 0xcb},       // GMA YST13
    {0xa9, 0xdd},       // GMA YST14
    {0xaa, 0xec},       // GMA YST15
    {0xab, 0x1a},       // GMA YSLP
    {0xac, 0x6e},       // "reserved"
    {0xbe, 0xff},       // "reserved"
    {0xbf, 0x00},       // "reserved"
    {0xce, 0x78},       // CMX M1
    {0xcf, 0x6e},       // CMX M2
    {0xd0, 0x0a},       // CMX M3
    {0xd1, 0x0c},       // CMX M4
    {0xd2, 0x84},       // CMX M5
    {0xd3, 0x90},       // CMX M6
    {0xd4, 0x1e},       // CMX CTRL
    {0xd5, 0x10},       // SCALE SMTH CTRL
    {0xda, 0x04},       // SDE CTRL
    {0xe9, 0x00},       // YAVG BLK THRE
    {0xec, 0x02},       // REGEC

    {0xff, 0xff}        // structure end marker
};

/* Initialisation sequence for QVGA resolution (320x240) */
static const omni_register_t ov7740_reset_regs_qvga[] =
{
    /* {register, value} */
    {0x04, 0x60},       // "reserved"
    {0x0c, 0x12},       // REG0C (YUV swap) the spec is wrong
//    {0x0c, 0x02},       // REG0C
    {0x0d, 0x34},       // "reserved"
    {0x11, 0x03},       // CLK
    {0x12, 0x00},       // REG12
    {0x13, 0xff},       // REG13
    {0x14, 0x38},       // REG14
    {0x17, 0x24},       // AHSTART
    {0x18, 0xa0},       // AHSIZE
    {0x19, 0x03},       // AVSTART
    {0x1a, 0xf0},       // AVSIZE
    {0x1b, 0x85},       // PSHFT
    {0x1e, 0x13},       // REG1E
    {0x20, 0x00},       // "reserved"
    {0x21, 0x23},       // "reserved"
    {0x22, 0x03},       // "reserved"
    {0x24, 0x3c},       // WPT
    {0x25, 0x30},       // BPT
    {0x26, 0x72},       // VPT
    {0x27, 0x80},       // REG27
    {0x29, 0x17},       // REG29
    {0x2b, 0xf8},       // REG2B
    {0x2c, 0x01},       // REG2C
    {0x31, 0x50},       // HOUTSIZE
    {0x32, 0x78},       // VOUTSIZE
    {0x33, 0xc4},       // "reserved"
    {0x36, 0x3f},       // "reserved"
    {0x38, 0x11},       // REG38 - see OV7740 data sheet
    {0x3a, 0xb4},       // "reserved"
    {0x3d, 0x0f},       // "reserved"
    {0x3e, 0x82},       // "reserved"
    {0x3f, 0x40},       // "reserved"
    {0x40, 0x7f},       // "reserved"
    {0x41, 0x6a},       // "reserved"
    {0x42, 0x29},       // "reserved"
    {0x44, 0xe5},       // "reserved"
    {0x45, 0x41},       // "reserved"
    {0x47, 0x42},       // "reserved"
    {0x48, 0x00},       // "reserved"
    {0x49, 0x61},       // "reserved"
    {0x4a, 0xa1},       // "reserved"
    {0x4b, 0x46},       // "reserved"
    {0x4c, 0x18},       // "reserved"
    {0x4d, 0x50},       // "reserved"
    {0x4e, 0x13},       // "reserved"
    {0x50, 0x97},       // REG50
    {0x51, 0x7e},       // REG51
    {0x52, 0x00},       // REG52
    {0x53, 0x00},       // "reserved"
    {0x55, 0x42},       // "reserved"
    {0x56, 0x55},       // REG56
    {0x57, 0xff},       // REG57
    {0x58, 0xff},       // REG58
    {0x59, 0xff},       // REG59
    {0x5a, 0x24},       // "reserved"
    {0x5b, 0x1f},       // "reserved"
    {0x5c, 0x88},       // "reserved"
    {0x5d, 0x60},       // "reserved"
    {0x5f, 0x04},       // "reserved"
    {0x64, 0x00},       // "reserved"
    {0x67, 0x88},       // REG67
    {0x68, 0x1a},       // "reserved"
    {0x69, 0x08},       // REG69
    {0x70, 0x00},       // "reserved"
    {0x71, 0x34},       // "reserved"
    {0x74, 0x28},       // "reserved"
    {0x75, 0x98},       // "reserved"
    {0x76, 0x00},       // "reserved"
    {0x77, 0x08},       // "reserved"
    {0x78, 0x01},       // "reserved"
    {0x79, 0xc2},       // "reserved"
    {0x7a, 0x9c},       // "reserved"
    {0x7b, 0x40},       // "reserved"
    {0x7c, 0x0c},       // "reserved"
    {0x7d, 0x02},       // "reserved"
    {0x80, 0x7f},       // ISP CTRL00
    {0x81, 0x3f},       // ISP CTRL01
    {0x82, 0x3f},       // ISP CTRL02
    {0x83, 0x03},       // ISP CTRL03
    {0x84, 0x70},       // REG84 - see OV7740 data sheet
    {0x85, 0x00},       // AGC OFFSET
    {0x86, 0x03},       // AGC BASE1
    {0x87, 0x01},       // AGC BASE2
    {0x88, 0x05},       // AGC CTRL
    {0x89, 0x30},       // LENC CTRL
    {0x8d, 0x40},       // LENC RED A1
    {0x8e, 0x00},       // LENC RED B1
    {0x8f, 0x33},       // LENC RED AB2
    {0x93, 0x28},       // LENC GREEN A1
    {0x94, 0x20},       // LENC GREEN B1
    {0x95, 0x33},       // LENC GREEN AB2
    {0x99, 0x30},       // LENC BLUE A1
    {0x9a, 0x14},       // LENC BLUE B1
    {0x9b, 0x33},       // LENC BLUE AB2
    {0x9c, 0x08},       // GMA YST01
    {0x9d, 0x12},       // GMA YST02
    {0x9e, 0x23},       // GMA YST03
    {0x9f, 0x45},       // GMA YST04
    {0xa0, 0x55},       // GMA YST05
    {0xa1, 0x64},       // GMA YST06
    {0xa2, 0x72},       // GMA YST07
    {0xa3, 0x7f},       // GMA YST08
    {0xa4, 0x8b},       // GMA YST09
    {0xa5, 0x95},       // GMA YST10
    {0xa6, 0xa7},       // GMA YST11
    {0xa7, 0xb5},       // GMA YST12
    {0xa8, 0xcb},       // GMA YST13
    {0xa9, 0xdd},       // GMA YST14
    {0xaa, 0xec},       // GMA YST15
    {0xab, 0x1a},       // GMA YSLP
    {0xac, 0x6e},       // "reserved"
    {0xbe, 0xff},       // "reserved"
    {0xbf, 0x00},       // "reserved"
    {0xce, 0x78},       // CMX M1
    {0xcf, 0x6e},       // CMX M2
    {0xd0, 0x0a},       // CMX M3
    {0xd1, 0x0c},       // CMX M4
    {0xd2, 0x84},       // CMX M5
    {0xd3, 0x90},       // CMX M6
    {0xd4, 0x1e},       // CMX CTRL
    {0xd5, 0x10},       // SCALE SMTH CTRL
    {0xda, 0x04},       // SDE CTRL
    {0xe9, 0x00},       // YAVG BLK THRE
    {0xec, 0x02},       // REGEC

    {0xff, 0xff}        // structure end marker
};

static const omni_register_t ov7740_test[] =
{
    {0x28, 0x02},       // VSYNC Negative Value
    {0xda, 0x00},       // Fixed YU disable

    {0xff, 0xff}        // structure end marker
};

static void write_omni_table (const omni_register_t Table[])
{
    uint16_t i;

    for (i = 0; ((0xff != Table[i].addr) || (0xff != Table[i].val)); i++)
    {
        ov7740_write(Table[i].addr, Table[i].val);
    }
}

/* image_format:
 *  0 = YUV422
 *  1 = RGB565
 *  2 = RGB444
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
 *  2 = UYVY (Cb/Y/Cr/Y)
 */
int ov7740_set_format( int image_format, int resolution, int out_format)
{
	uint8_t reg0c;

	// TODO: reset via GPIO pins
	// usleep(10000);                  // The Omnivision chip needs some wait time after reset

	switch (resolution)
	{
		case 0 :
			write_omni_table(ov7740_reset_regs_vga);
			break;

		case 2 :
			write_omni_table(ov7740_reset_regs_qvga);
			break;

		default:
			printf("%s: illegal resolution\n",__func__);
			return -1;
	}

	ov7740_read( 0x0C, &reg0c);
	reg0c &= ~0x10;	// clear bit 4

	switch (out_format)
	{
		/* NOTE: According to the OV7740 spec, 0=YUVU, and 1=UYVY. But,
                         it seems those values are backwards. */
		case 2 :
			break;

		case 0 :
			reg0c |= 0x10;
			break;

		default:
			printf("%s: out_format\n",__func__);
			return -1;
	}
	ov7740_write( 0x0C, reg0c);

	write_omni_table(ov7740_test);

	return 0;
}
int ov7740_close(void)
{
	close(ov7740_fd);
	ov7740_fd = 0;
	
	return 0;
}



