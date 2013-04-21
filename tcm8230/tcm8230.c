#include "pio.h"
#include "piobus.h"
#include "tc.h"
#include "delay.h"
#include "i2c_master.h"

 
#define TCM8230_TWI_ADDRESS 0x30
 
#define TCM8230_CLOCK_INITIAL 6.25e6
 
#ifndef TCM8230_CLOCK
#define TCM8230_CLOCK 2e6
#endif
 
 
/* Register 0x02 control fields.  */
enum {TCM8230_FPS_30 = 0,
      TCM8230_FPS_15 = 1 << 7};
 
enum {TCM8230_ACF_50 = 0,
      TCM8230_ACF_60 = 1 << 6};
 
enum {TCM8230_DCLKP_NORMAL = 0,
      TCM8230_DCLKP_REVERSE = 1 << 1};
 
enum {TCM8230_ACFDET_AUTO = 0,
      TCM8230_ACFDET_MANUAL = 1};
 
/* Register 0x03 control fields.  */
enum {TCM8230_DOUTSW_ON = 0,
      TCM8230_DOUTSW_OFF = 1 << 7};
 
enum {TCM8230_DATAHZ_OUT = 0,
      TCM8230_DATAHZ_HIZ = 1 << 6};
 
enum {TCM8230_PICSIZ_VGA = 0,
      TCM8230_PICSIZ_QVGA = 1 << 2,
      TCM8230_PICSIZ_QVGA_ZOOM = 2 << 2,
      TCM8230_PICSIZ_QQVGA = 3 << 2,
      TCM8230_PICSIZ_QQGA_ZOOM = 4 << 2,
      TCM8230_PICSIZ_CIF = 5 << 2,
      TCM8230_PICSIZ_QCIF = 6 << 2,
      TCM8230_PICSIZ_QCIF_ZOOM = 7 << 2,
      TCM8230_PICSIZ_SQCIF = 8 << 2,
      TCM8230_PICSIZ_SQCIF_ZOOM = 9 << 2};
 
enum {TCM8230_PICFMT_YUV = 0,
      TCM8230_PICFMT_RGB = 1 << 1};
 
enum {TCM8230_CM_COLOR = 0,
      TCM8230_CM_BW = 1};
 
 
/* Register 0x1E control fields.  */
enum {TCM8230_CODESW_OFF = 0,
      TCM8230_CODESW_OUT = 1 << 5};
 
enum {TCM8230_CODESEL_ORIGINAL = 0,
      TCM8230_CODESEL_ITU656 = 1 << 4};
 
enum {TCM8230_HSYNCSEL_NORMAL = 0,
      TCM8230_HSYNCSEL_ALT = 1 << 3};
 
enum {TCM8230_TESTPIC_NOTOUT = 0,
      TCM8230_TESTPIC_OUT = 1 << 2};
 
enum {TCM8230_PICSEL_COLORBAR = 0,
      TCM8230_PICSEL_RAMP1 = 1,
      TCM8230_PICSEL_RAMP2 = 2};
 
 
enum {VGA_HEIGHT = 480,
      CIF_HEIGHT = 288,
      QVGA_HEIGHT = 240,
      QCIF_HEIGHT = 144,
      QQVGA_HEIGHT = 120,
      SQCIF_HEIGHT = 96};
 
enum {VGA_WIDTH = 640,
      CIF_WIDTH = 352,
      QVGA_WIDTH = 320,
      QCIF_WIDTH = 176,
      QQVGA_WIDTH = 160,
      SQCIF_WIDTH = 128};
 


static const i2c_bus_cfg_t i2c_bus_cfg =
{
    .scl = TCM8230_SCL_PIO,
    .sda = TCM8230_SDA_PIO
};


static const i2c_slave_cfg_t i2c_cfg =
{
    .id = TCM8230_TWI_ADDRESS
};


static void
tcm8230_reg_write (i2c_t i2c, uint8_t addr, uint8_t value)
{
    i2c_master_addr_write (i2c, addr, 1, &value, sizeof (value));
}


static const tc_cfg_t tc_cfg =
{
    .pio = TCM8230_EXTCLK_PIO
};

 
int tcm8230_init (void)
{
    tc_t tc;
    i2c_t i2c;
    
    pio_config_set (TCM8230_VD_PIO, PIO_INPUT);
    pio_config_set (TCM8230_HD_PIO, PIO_INPUT);
    pio_config_set (TCM8230_DCLK_PIO, PIO_INPUT);
 
    piobus_config_set (TCM8230_DATA_PIOBUS, PIO_INPUT);
 
    tc = tc_init (&tc_cfg);

    tc_squarewave_config (tc, TC_PERIOD_DIVISOR (TCM8230_CLOCK_INITIAL));
 
    tc_start (tc);
 
    /* CHECKME.  */
    DELAY_US (1000);
 

    i2c = i2c_master_init (&i2c_bus_cfg, &i2c_cfg);
 
    /* Turn on data output, set picture size, and black and white operation.  */
    tcm8230_reg_write (i2c, 0x03, TCM8230_DOUTSW_ON | TCM8230_DATAHZ_OUT 
                       | TCM8230_PICSIZ_SQCIF | TCM8230_PICFMT_RGB | TCM8230_CM_BW);

 
    /* CHECKME.  */
    DELAY_US (10);
 
    /* Set 15 fps.  */
    tcm8230_reg_write (i2c, 0x02, TCM8230_FPS_15 | TCM8230_ACF_50 
                       | TCM8230_DCLKP_NORMAL | TCM8230_ACFDET_AUTO);
    
 
    /* CHECKME.  */
    DELAY_US (10);
 
    /* Turn off codes and set HD to go low after 256 DCLKs.  */
    tcm8230_reg_write (i2c, 0x1E, TCM8230_CODESW_OFF | TCM8230_CODESEL_ORIGINAL
                       | TCM8230_HSYNCSEL_ALT | TCM8230_TESTPIC_NOTOUT
                       | TCM8230_PICSEL_COLORBAR);

 
    /* Slow down clock; this will result in fewer than 15 fps.  */
    tc_squarewave_config (tc, TC_PERIOD_DIVISOR (TCM8230_CLOCK));
 
    return 1;
}
