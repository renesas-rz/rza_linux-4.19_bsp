int ov7740_open(char *i2c_dev_path);
int ov7740_close(void);
int ov7740_set_format( int image_format, int resolution, int out_format );
void ov7740_print_registers(void);

#ifndef CAM_FMT_YUV422
 #define CAM_FMT_YUV422 0
 #define CAM_FMT_RGB444 1 /* (not supported) */
 #define CAM_FMT_RGB565 2 /* (not supported) */
 #define CAM_FMT_RAW 3 /* (not supported) */
#endif

#ifndef CAM_RES_VGA
 #define CAM_RES_VGA 0 /* (640x480) */
 #define CAM_RES_CIF 1 /* (352x288) */
 #define CAM_RES_QVGA 2 /* (320x240) */
 #define CAM_RES_QCIF 3 /* (176x144) */
#endif

#ifndef CAM_OUT_YUYV
 #define CAM_OUT_YUYV 0	/* (Y/Cb/Y/Cr) */
 #define CAM_OUT_YVYU 1	/*  (Y/Cr/Y/Cb) */
 #define CAM_OUT_UYVY 2	/*  (Cb/Y/Cr/Y) */
 #define CAM_OUT_VYUY 3	/*  (Cr/Y/Cb/Y) */
#endif
