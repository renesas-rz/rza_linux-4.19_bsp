#ifndef __rzjpu_regs_h__
#define __rzjpu_regs_h__

/*
 * JPU Registers
 */

#define STBCR6			0x042C	/* (8)standby control register6 */
#define SWRSTCR2		0x0464	/* (8)software reset control register2 */

#define JPU_JCMOD		0x0000	/* (8)JPEG code mode register */
#define JPU_JCCMD		0x0001	/* (8)JPEG code command register */
#define JPU_JCQTN		0x0003	/* (8)JPEG code quantization table no. register */
#define JPU_JCHTN		0x0004	/* (8)JPEG code Huffman table number register */
#define JPU_JCDRIU		0x0005	/* (8)JPEG code DRI upper register */
#define JPU_JCDRID		0x0006	/* (8)JPEG code DRI lower register */
#define JPU_JCVSZU		0x0007	/* (8)JPEG code vertical size upper register */
#define JPU_JCVSZD		0x0008	/* (8)JPEG code vertical size lower register */
#define JPU_JCHSZU		0x0009	/* (8)JPEG code horizontal size upper register */
#define JPU_JCHSZD		0x000A	/* (8)JPEG code horizontal size lower register */
#define JPU_JCDTCU		0x000B	/* (8)JPEG code data count upper register */
#define JPU_JCDTCM		0x000C	/* (8)JPEG code data count middle register */
#define JPU_JCDTCD		0x000D	/* (8)JPEG code data count lower register */

#define JPU_JINTE0		0x000E	/* (8)JPEG interrupt enable register0 */
#define JPU_JINTS0		0x000F	/* (8)JPEG interrupt status register0 */

#define JPU_JCDERR		0x0010	/* (8)JPEG code decode error register */
#define JPU_JCRST		0x0011	/* (8)JPEG code reset register */

#define JPU_JIFECNT		0x0040	/* (32)JPEG I/F encode interface control register */
#define JPU_JIFESA		0x0044	/* (32)JPEG I/F encode src address register */
#define JPU_JIFESOFST	0x0048	/* (32)JPEG I/F encode line offset address register */
#define JPU_JIFEDA		0x004C	/* (32)JPEG I/F encode dst address register */
#define JPU_JIFESLC		0x0050	/* (32)JPEG I/F encode src line count register */
#define JPU_JIFEDDC		0x0054	/* (32)JPEG I/F encode dst register */

#define JPU_JIFDCNT		0x0058	/* (32)JPEG I/F decode interface control register */
#define JPU_JIFDSA		0x005C	/* (32)JPEG I/F decode src address register */
#define JPU_JIFDDOFST	0x0060	/* (32)JPEG I/F decode line offset address register */
#define JPU_JIFDDA		0x0064	/* (32)JPEG I/F decode dst address register */
#define JPU_JIFDSDC		0x0068	/* (32)JPEG I/F decode src line count register */
#define JPU_JIFDDLC		0x006C	/* (32)JPEG I/F decode dst register */
#define JPU_JIFDADT		0x0070	/* (32)JPEG I/F decode alpha setting register */

#define JPU_JINTE1		0x008C	/* (32)JPEG interrupt enable register1 */
#define JPU_JINTS1		0x0090	/* (32)JPEG interrupt status register1 */

#define JPU_JIFESVSZ	0x0094	/* (32)JPEG input image  data CbCr setting register */
#define JPU_JIFESHSZ	0x0098	/* (32)JPEG input output data CbCr setting register */

/* (8)JPEG code quantization table 0 register */
#define JPU_JCQTBL0(n)	(0x100 + (n & 0x3F))	/* to 0x13F */

/* (8)JPEG code quantization table 1 register */
#define JPU_JCQTBL1(n)	(0x140 + (n & 0x3F))	/* to 0x17F */

/* (8)JPEG code quantization table 2 register */
#define JPU_JCQTBL2(n)	(0x180 + (n & 0x3F))	/* to 0x1BF */

/* (8)JPEG code quantization table 3 register */
#define JPU_JCQTBL3(n)	(0x1C0 + (n & 0x3F))	/* to 0x1FF */

/* (8)JPEG code Huffman table DC0 register */
#define JPU_JCHTBD0(n)	(0x200 + (n & 0x1B))	/* to 0x21B */

/* (8)JPEG code Huffman table AC0 register */
#define JPU_JCHTBA0(n)	(0x220 + (n & 0xA1))	/* to 0x2D1 */

/* (8)JPEG code Huffman table DC1 register */
#define JPU_JCHTBD1(n)	(0x300 + (n & 0x1B))	/* to 0x31B */

/* (8)JPEG code Huffman table AC1 register */
#define JPU_JCHTBA1(n)	(0x320 + (n & 0xA1))	/* to 0x3D1 */


#endif /* !__rzjpu_regs_h__ */

