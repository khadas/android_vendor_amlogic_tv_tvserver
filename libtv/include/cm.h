#ifndef _TVOUT_CM_H
#define _TVOUT_CM_H

#define REGS_MAX_NUMBER 900

// ***************************************************************************
// *** enum definitions *********************************************
// ***************************************************************************

typedef enum cm_region_idx_e {
    CM_REGION_IDX_0 = 0,
    CM_REGION_IDX_1,
    CM_REGION_IDX_2,
    CM_REGION_IDX_3,
    CM_REGION_IDX_4,
    CM_REGION_IDX_5,
    CM_REGION_IDX_6,
    CM_REGION_IDX_7,
} cm_region_idx_t;

typedef enum cm_sat_shape_e {
    CM_SAT_SHAPE_RIGHT_BIGGEST = 0,
    CM_SAT_SHAPE_LEFT_BIGGEST,
} cm_sat_shape_t;

typedef enum cm_hue_shape_e {
    CM_HUE_SHAPE_LEFT_MORE = 0,
    CM_HUE_SHAPE_RIGHT_MORE,
} cm_hue_shape_t;

typedef enum cm_demo_pos_e {
    CM_DEMO_POS_TOP = 0,
    CM_DEMO_POS_BOTTOM,
    CM_DEMO_POS_LEFT,
    CM_DEMO_POS_RIGHT,
} cm_demo_pos_t;

typedef enum cm_sat_sel_e {
    CM_SAT_SEL_U2_V2 = 0,
    CM_SAT_SEL_UV_MAX,
} cm_sat_sel_t;

typedef enum cm_csc_e {
    CM_CSC_601 = 0,
    CM_CSC_709,
    CM_CSC_FULL_601,
    CM_CSC_FULL_709,
} cm_csc_t;

// ***************************************************************************
// *** struct definitions *********************************************
// ***************************************************************************

typedef struct cm_region_s {
    enum cm_region_idx_e region_idx;
    // sym
    unsigned char       sym_en;
    // sat - top
    unsigned char       sat_en;
    unsigned char       sat_central_en;
    enum cm_sat_shape_e sat_shape;
    unsigned char       sat_gain;
    unsigned char       sat_inc;
    // sat - lum
    unsigned char       sat_lum_h_slope;
    unsigned char       sat_lum_l_slope;
    unsigned char       sat_lum_h;
    unsigned char       sat_lum_l;
    // sat - sat
    unsigned char       sat_sat_h_slope;
    unsigned char       sat_sat_l_slope;
    unsigned char       sat_sat_h;
    unsigned char       sat_sat_l;
    // hue - top
    unsigned char       hue_en;
    unsigned char       hue_central_en;
    enum cm_hue_shape_e hue_shape;
    unsigned char       hue_gain;
    unsigned char       hue_clockwise;
    unsigned char       hue_shf_ran;
    unsigned short      hue_shf_sta;
    // hue - lum
    unsigned char       hue_lum_h_slope;
    unsigned char       hue_lum_l_slope;
    unsigned char       hue_lum_h;
    unsigned char       hue_lum_l;
    // hue - sat
    unsigned char       hue_sat_h_slope;
    unsigned char       hue_sat_l_slope;
    unsigned char       hue_sat_h;
    unsigned char       hue_sat_l;
} cm_region_t;

typedef struct cm_top_s {
    unsigned char       chroma_en;
    enum cm_sat_sel_e   sat_sel;
    unsigned char       uv_adj_en;
    unsigned char       rgb_to_hue_en;
    enum cm_csc_e       csc_sel;
} cm_top_t;

typedef struct cm_cbar_s {
    unsigned char en;
    unsigned char wid;
    unsigned char cr;
    unsigned char cb;
    unsigned char y;
} cm_cbar_t;
typedef struct cm_demo_s {
    unsigned char       en;
    enum cm_demo_pos_e  pos;
    unsigned char       hlight_adj;
    unsigned short      wid;
    struct cm_cbar_s   cbar;
} cm_demo_t;

typedef struct cm_regmap_s {
    unsigned long reg[50];
} cm_regmap_t;

typedef enum reg_bus_type_e {
    REG_TYPE_PHY = 0,
    REG_TYPE_CBUS,
    REG_TYPE_APB,
    REG_TYPE_AXI,
    REG_TYPE_AHB,
    REG_TYPE_MPEG,
    REG_TYPE_INDEX_VPPCHROMA,
    REG_TYPE_INDEX_GAMMA,
    VALUE_TYPE_CONTRAST_BRIGHTNESS,
    REG_TYPE_INDEX_VPP_COEF,
    REG_TYPE_MAX,
} reg_bus_type_t;

typedef enum pq_table_name_e {
    TABLE_NAME_SHARPNESS0 = 0x1,/*in vpp*/
    TABLE_NAME_SHARPNESS1 = 0x2,/*in vpp*/
    TABLE_NAME_DNLP = 0x4,      /*in vpp*/
    TABLE_NAME_CM = 0x8,        /*in vpp*/
    TABLE_NAME_BLK_BLUE_EXT = 0x10,/*in vpp*/
    TABLE_NAME_BRIGHTNESS = 0x20,/*in vpp*/
    TABLE_NAME_CONTRAST = 0x40, /*in vpp*/
    TABLE_NAME_SATURATION_HUE = 0x80,/*in vpp*/
    TABLE_NAME_CVD2 = 0x100,        /*in tvafe*/
    TABLE_NAME_DI = 0x200,      /*in di*/
    TABLE_NAME_NR = 0x400,      /*in di*/
    TABLE_NAME_MCDI = 0x800,    /*in di*/
    TABLE_NAME_DEBLOCK = 0x1000,    /*in di*/
    TABLE_NAME_DEMOSQUITO = 0x2000,/*in di*/
    TABLE_NAME_WB = 0X4000,     /*in vpp*/
    TABLE_NAME_GAMMA = 0X8000,  /*in vpp*/
    TABLE_NAME_XVYCC = 0x10000, /*in vpp*/
    TABLE_NAME_HDR = 0x20000,   /*in vpp*/
    TABLE_NAME_DOLBY_VISION = 0x40000,/*in vpp*/
    TABLE_NAME_RESERVED1 = 0x80000,
    TABLE_NAME_RESERVED2 = 0x100000,
    TABLE_NAME_RESERVED3 = 0x200000,
    TABLE_NAME_RESERVED4 = 0x400000,
    TABLE_NAME_RESERVED5 = 0x800000,
    TABLE_NAME_MAX,
} pq_table_name_t;

/* Register table structure */
typedef struct am_reg_s {
    unsigned int type; //32-bits; 0: CBUS; 1: APB BUS...
    unsigned int addr; //32-bits; Register address
    unsigned int mask; //32-bits; Valid bits
    unsigned int  val; //32-bits; Register Value
} am_reg_t;

#ifdef AMVIDEO_REG_TABLE_DYNAMIC
typedef struct am_regs_s {
    unsigned int    length; // Length of total am_reg
    struct am_reg_s *am_reg;
} am_regs_t;
#else
typedef struct am_regs_s {
    unsigned int    length; // Length of total am_reg
    struct am_reg_s am_reg[REGS_MAX_NUMBER];
} am_regs_t;

typedef struct am_pq_param_s {
    unsigned int table_name;
    unsigned int table_len;
    void *table_ptr;
    void *reserved0;
    void *reserved1;
    void *reserved2;
} am_pq_param_t;

#endif

#endif  // _TVOUT_CM_H
