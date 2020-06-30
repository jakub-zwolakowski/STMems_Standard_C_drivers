#include<tis_builtin.h>

/*
 * @brief  Write generic device register (platform dependent)
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to write
 * @param  bufp      pointer to data to write in register reg
 * @param  len       number of consecutive register to write
 *
 */
static int32_t platform_write(void *handle, uint8_t reg, uint8_t *bufp,
                              uint16_t len)
{
  // We do not maintain an internal state for the hardware, so
  // we just log the write.
  printf("[TIS] Write register 0x%hhx (len: %hu)\n", reg, len);
  return 0;
}

/*
 * @brief  Read generic device register (platform dependent)
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to read
 * @param  bufp      pointer to buffer that store the data read
 * @param  len       number of consecutive register to read
 *
 */
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
                             uint16_t len)
{
  switch(reg) {
  case AIS2DW12_WHO_AM_I:
    // Assuming only this ID can happen
    *bufp = AIS2DW12_ID;
    break;
  default:
    printf("[TIS] Read register 0x%hhx (len: %hu)\n", reg, len);

    // The platform needs to write len bytes to bufp: this means
    // that bufp must point to a valid and large enough buffer.
    // This is what we specify here:

    //@ assert UB: \valid(bufp+(0 .. len -1));

    // Let's assume anything may be read from the device 
    tis_make_unknown(bufp, len);
    break;
  }
  return 0;
}

/*
 * @brief  Write generic device register (platform dependent)
 *
 * @param  tx_buffer     buffer to trasmit
 * @param  len           number of byte to send
 *
 */
static void tx_com(uint8_t *tx_buffer, uint16_t len)
{
}

/*
 * @brief  platform specific delay (platform dependent)
 *
 * @param  ms        delay in ms
 *
 */
static void platform_delay(uint32_t ms)
{
}

/*
 * @brief  platform specific initialization (platform dependent)
 */
static void platform_init(void)
{
}
