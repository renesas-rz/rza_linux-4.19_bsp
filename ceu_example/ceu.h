#define CAPSR  0x00 /* Capture start register */
#define CAPCR  0x04 /* Capture control register */
#define CAMCR  0x08 /* Capture interface control register */
#define CMCYR  0x0c /* Capture interface cycle  register */
#define CAMOR  0x10 /* Capture interface offset register */
#define CAPWR  0x14 /* Capture interface width register */
#define CAIFR  0x18 /* Capture interface input format register */
#define CSTCR  0x20 /* Camera strobe control register (<= sh7722) */
#define CSECR  0x24 /* Camera strobe emission count register (<= sh7722) */
#define CRCNTR 0x28 /* CEU register control register */
#define CRCMPR 0x2c /* CEU register forcible control register */
#define CFLCR  0x30 /* Capture filter control register */
#define CFSZR  0x34 /* Capture filter size clip register */
#define CDWDR  0x38 /* Capture destination width register */
#define CDAYR  0x3c /* Capture data address Y register */
#define CDACR  0x40 /* Capture data address C register */
#define CDBYR  0x44 /* Capture data bottom-field address Y register */
#define CDBCR  0x48 /* Capture data bottom-field address C register */
#define CBDSR  0x4c /* Capture bundle destination size register */
#define CFWCR  0x5c /* Firewall operation control register */
#define CLFCR  0x60 /* Capture low-pass filter control register */
#define CDOCR  0x64 /* Capture data output control register */
#define CDDCR  0x68 /* Capture data complexity level register */
#define CDDAR  0x6c /* Capture data complexity level address register */
#define CEIER  0x70 /* Capture event interrupt enable register */
#define CETCR  0x74 /* Capture event flag clear register */
#define CSTSR  0x7c /* Capture status register */
#define CSRTR  0x80 /* Capture software reset register */
#define CDSSR  0x84 /* Capture data size register */
#define CDAYR2 0x90 /* Capture data address Y register 2 */
#define CDACR2 0x94 /* Capture data address C register 2 */
#define CDBYR2 0x98 /* Capture data bottom-field address Y register 2 */
#define CDBCR2 0x9c /* Capture data bottom-field address C register 2 */

void ceu_set_base_addr(void *base_addr);
void ceu_print_register(void);
void ceu_start_cap(void);
int ceu_is_capturing(void);
int ceu_init(int width, int height, int mode);
void ceu_set_buffer_addr(uint32_t phys_addr);

