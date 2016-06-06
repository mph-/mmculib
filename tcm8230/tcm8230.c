#include "pio.h"
#include "piobus.h"
#include "pwm.h"
#include "tc.h"
#include "delay.h"
#include "i2c_master.h"
#include "tcm8230.h"

/* This driver bit-bashes a TCM8230 image sensor.  

   It assumes that the data lines D0-D7 are contiguous on the same
   port.

   I2C/TWI is required to configure the TCM8230.  This can be
   bit-bashed with any two PIO lines.

   A TC or PWM output is required to drive EXTCLK.  It appears that
   the EXTCLK frequency needs to be at least 6.25 MHz when configuring
   the TCM8230 via I2C (TWI).  The frequency of EXTCLK can then be
   reduced to allow grabbing of the data by polling a PIO port.

   When the TCM8230 is correctly configured, it will start the DCLK,
   HD, and VD signals and put the pixel data on the D0-D7 data lines.

   The TCM8230 requires three power supplies.  If the 2.5 V supply is
   not provided, all the pixels return black (97, 8) in colour
   (RGB565) mode.


   DCLK has a frequency of EXTCLK / 4.  Thus with EXTCLK at 2 MHz,
   DCLK is 500 kHz.

   Two DCLKs are required per pixel.
   
   With EXTCLK at 2 MHz, SQCIF frame format (128 x 96 pixels), and
   low-power mode:

   HD is high for 128 * 2 / 500e3 = 512 us.
   HD is low for (1560 - 128 * 2) / 500e3 = 2.61 ms.
   The HD period is 1560 / 500e3 = 3.12 ms.

   VD is high for 254 lines, 254 * 1560 / 500e3 = 792.48 ms.
   VD is low for 9 lines, 9 * 1560 / 500e3 = 28.08 ms.
   The VD period is 263 lines, 263 * 1560 / 500e3 = 820.56 ms.

   This corresponds to a frame rate of 1.22 Hz.  To achieve 15 frames
   per second requires a 25 MHz EXTCLK.

   When debugging:
   1.  Check that all the regulators are enabled to power the image sensor
       before tcm8230_init is called.
   2.  Check that EXTCLK is driven my the MCU after tcm8230_init is called.
   3.  Check that the SDA and SCL lines toggle when tcm8230_init is called.
       Three bytes should be sent from the MCU and the image sensor should
       acknowledge each.
   4.  Check the DCLK, HD, and VD signals are driven by the image sensor
       after tcm8230_init is called.
*/   
 
#define TCM8230_TWI_ADDRESS 0x3C
 
#define TCM8230_CLOCK_INITIAL 6.25e6
 
#ifndef TCM8230_CLOCK
#define TCM8230_CLOCK 2e6
#endif

#define TCM8230_HSYNC_TIMEOUT_US 10000
 
 
/* Register 0x02 control fields.  */
enum {TCM8230_FPS_30 = 0,
      TCM8230_FPS_15 = 1 << 7};
 
enum {TCM8230_ACF_50 = 0,
      TCM8230_ACF_60 = 1 << 6};

/* DCLK polarity; normal is data valid on rising edge.  */ 
enum {TCM8230_DCLKP_NORMAL = 0,
      TCM8230_DCLKP_REVERSE = 1 << 1};
 
enum {TCM8230_ACFDET_AUTO = 0,
      TCM8230_ACFDET_MANUAL = 1};
 
/* Register 0x03 control fields.  */
enum {TCM8230_DOUTSW_ON = 0,
      TCM8230_DOUTSW_OFF = 1 << 7};
 
enum {TCM8230_DATAHZ_OUT = 0,
      TCM8230_DATAHZ_HIZ = 1 << 6};
 
enum {TCM8230_PICFMT_YUV422 = 0,
      TCM8230_PICFMT_RGB565 = 1 << 1};
 
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

enum {TCM8230_D_MASK1 = 1 << 6};
 
 
typedef struct tcm8230_mode_struct
{
    uint16_t width;
    uint16_t height;
} tcm8230_mode_t;


static const tcm8230_mode_t modes[] =
{
    {VGA_WIDTH, VGA_HEIGHT},
    {QVGA_WIDTH, QVGA_HEIGHT},
    {QVGA_WIDTH, QVGA_HEIGHT},
    {QQVGA_WIDTH, QQVGA_HEIGHT},    
    {QQVGA_WIDTH, QQVGA_HEIGHT},    
    {CIF_WIDTH, CIF_HEIGHT},    
    {QCIF_WIDTH, QCIF_HEIGHT},    
    {QCIF_WIDTH, QCIF_HEIGHT},    
    {SQCIF_WIDTH, SQCIF_HEIGHT},    
    {SQCIF_WIDTH, SQCIF_HEIGHT}
};
 

static const i2c_bus_cfg_t i2c_bus_cfg =
{
    .scl = TCM8230_SCL_PIO,
    .sda = TCM8230_SDA_PIO
};


static uint16_t width;
static uint16_t height;


static void
tcm8230_reg_write (i2c_t i2c, uint8_t addr, uint8_t value)
{
    i2c_master_addr_write (i2c, TCM8230_TWI_ADDRESS, addr, 1,
                           &value, sizeof (value));
}

#define TC_PRESCALE 2

static const tc_cfg_t tc_cfg =
{
    .pio = TCM8230_EXTCLK_PIO,
    .mode = TC_MODE_CLOCK,
    .frequency = TCM8230_CLOCK_INITIAL,
    .prescale = TC_PRESCALE
};


static const pwm_cfg_t pwm_cfg =
{
    .pio = TCM8230_EXTCLK_PIO,
    .period = PWM_PERIOD_DIVISOR (TCM8230_CLOCK_INITIAL),
    .duty = PWM_DUTY_DIVISOR (TCM8230_CLOCK_INITIAL, 50),
    .align = PWM_ALIGN_LEFT,
    .polarity = PWM_POLARITY_LOW
};


int tcm8230_init (const tcm8230_cfg_t *cfg)
{
    tc_t tc;
    pwm_t pwm;
    i2c_t i2c;
    uint8_t value;

    if (cfg->picsize > TCM8230_PICSIZE_SQCIF_ZOOM)
        return 0;

    width = modes[cfg->picsize].width;
    height = modes[cfg->picsize].height;

    /* Configure PIOs.  */
    pio_config_set (TCM8230_VD_PIO, PIO_INPUT);
    pio_config_set (TCM8230_HD_PIO, PIO_INPUT);
    pio_config_set (TCM8230_DCLK_PIO, PIO_INPUT);

#ifdef TCM8230_RESET_PIO
    pio_config_set (TCM8230_RESET_PIO, PIO_OUTPUT_LOW);
#endif
 
    piobus_config_set (TCM8230_DATA_PIOBUS, PIO_INPUT);
 

    /* Generate EXTCLK.  Try TC then fall back to PWM driver.  */
    pwm = 0;
    tc = tc_init (&tc_cfg);
    if (tc)
    {
        tc_start (tc);
    }
    else
    {
        pwm = pwm_init (&pwm_cfg);
        if (!pwm)
            return 0;
        pwm_start (pwm);
    }

#ifdef TCM8230_RESET_PIO
    /* Need to wait for 100 EXTCLK cycles.  */
    DELAY_US (100e6 / TCM8230_CLOCK_INITIAL);
    pio_output_high (TCM8230_RESET_PIO);
#endif


    /* CHECKME.  */
    DELAY_US (1000);


    /* Configure sensor using I2C.  */
    i2c = i2c_master_init (&i2c_bus_cfg);
 
    /* Set 15 fps.  */
    tcm8230_reg_write (i2c, 0x02, TCM8230_FPS_15 | TCM8230_ACF_50 
                       | TCM8230_DCLKP_NORMAL | TCM8230_ACFDET_AUTO);
    
 
    /* CHECKME.  */
    DELAY_US (10);
 
    /* Turn on data output, set picture size, and data format.  */
    value = TCM8230_DOUTSW_ON | TCM8230_DATAHZ_OUT 
        | (cfg->picsize << 2) | TCM8230_PICFMT_RGB565;

    if (0 && ! cfg->colour)
    {
        /* This does not seem to work properly.  FIXME.  */
        value |= TCM8230_CM_BW;
    }

    tcm8230_reg_write (i2c, 0x03, value);
 
    /* CHECKME.  */
    DELAY_US (10);
 
    /* Turn off codes and set HD to go low after 256 DCLKs.  */
    tcm8230_reg_write (i2c, 0x1E, TCM8230_D_MASK1 
                       | TCM8230_CODESW_OFF | TCM8230_CODESEL_ORIGINAL
                       | TCM8230_HSYNCSEL_ALT | TCM8230_TESTPIC_NOTOUT
                       | TCM8230_PICSEL_COLORBAR);

 
    /* Slow down clock; this will result in fewer than 15 fps but it makes
       it easier to poll.  */
    if (tc)
    {
        tc_frequency_set (tc, TCM8230_CLOCK);
    }
    else
    {
        pwm_period_set (pwm, PWM_PERIOD_DIVISOR (TCM8230_CLOCK));
        pwm_duty_set (pwm, PWM_DUTY_DIVISOR (TCM8230_CLOCK, 50));
    }
 
    return 1;
}


bool tcm8230_vsync_high_wait (uint32_t timeout_us)
{
  /* Wait for vertical sync. to go high.  */
    while (1)
    {
        if (pio_input_get (TCM8230_VD_PIO))
            return 1;
        if (!timeout_us)
            return 0;
        timeout_us--;
        DELAY_US (1);
    }
}


bool tcm8230_vsync_low_wait (uint32_t timeout_us)
{
  /* Wait for vertical sync. to go low.  */
    while (1)
    {
        if (! pio_input_get (TCM8230_VD_PIO))
            return 1;
        if (!timeout_us)
            return 0;
        timeout_us--;
        DELAY_US (1);
    }
}


bool tcm8230_hsync_high_wait (uint32_t timeout_us)
{
    /* Wait for horizontal sync. to go high.  */
    while (1)
    {
        if (pio_input_get (TCM8230_HD_PIO))
            return 1;
        if (!timeout_us)
            return 0;
        timeout_us--;
        DELAY_US (1);
    }
}


bool tcm8230_hsync_low_wait (uint32_t timeout_us)
{
    /* Wait for horizontal sync. to go low.  */
    while (1)
    {
        if (! pio_input_get (TCM8230_HD_PIO))
            return 1;
        if (!timeout_us)
            return 0;
        timeout_us--;
        DELAY_US (1);
    }
}


int16_t tcm8230_line_read (uint8_t *row, uint16_t cols)
{
    uint16_t col;
    uint8_t *buffer;

    buffer = row;

    if (! pio_input_get (TCM8230_HD_PIO))
        return TCM8230_LINE_NOT_READY;

    for (col = 0; col < cols * 2; col++)
    {
        /* TODO: should add timeout.  */
        while (! pio_input_get (TCM8230_DCLK_PIO))
            continue;
        
        *buffer++ = piobus_input_get (TCM8230_DATA_PIOBUS);
        
        /* TODO: should add timeout.  */
        while (pio_input_get (TCM8230_DCLK_PIO))
            continue;
    }
    
    return cols * 2;
}


/** This blocks until it captures a frame.  This may be up to nearly two image
    capture periods.  You should poll tcm8230_frame_ready_p first to see when
    VSYNC (VD) goes low.   If you call this function with VSYNC high you will
    miss the start of the image.  */
int32_t tcm8230_capture (uint8_t *image, uint32_t bytes, uint32_t timeout_us)
{
    uint16_t row;
    uint8_t *buffer;
    
    /* Check if user buffer large enough.  */
    if (bytes < 2u * height * width)
        return TCM8230_BUFFER_SMALL;

    buffer = image;

    /* It appears that the VD signal changes on the rising edge of DCLK
       but the HD signal changes on the falling edge of DCLK.  
       
       After VD goes high, there are 156 DCLKs before HD goes high to
       signify the first row (the data in this period is blanking).
       The time between HD going high again is 1560 DCLKs.

       In normal power mode a frame is 507 active lines and 18
       blanking lines (525 lines) total.

       In low power mode a frame is 254 active lines and 9
       blanking lines (263 lines) total.
    */

    if (! tcm8230_vsync_high_wait (timeout_us))
        return TCM8230_VSYNC_HIGH_TIMEOUT;

    for (row = 0; row < height; row++)
    {
        int16_t ret;

        /* Wait for horizontal sync. to go high.  Ideally should check for
           low to high transition.  */
        if (!tcm8230_hsync_high_wait (TCM8230_HSYNC_TIMEOUT_US))
            return TCM8230_HSYNC_HIGH_TIMEOUT;

        ret = tcm8230_line_read (buffer, width);
        if (ret < 0)
            return ret;
        buffer += ret;

        /* For small image formats there is plenty of spare time
           here...  */

        if (! tcm8230_hsync_low_wait (TCM8230_HSYNC_TIMEOUT_US))
            return TCM8230_HSYNC_LOW_TIMEOUT;
    }
    return buffer - image;
}


uint16_t tcm8230_width (void)
{
    return width;
}


uint16_t tcm8230_height (void)
{
    return height;
}


bool tcm8230_frame_ready_p (void)
{
    return tcm8230_vsync_high_wait (0);
}


bool tcm8230_line_ready_p (void)
{
    return tcm8230_hsync_high_wait (0);
}
