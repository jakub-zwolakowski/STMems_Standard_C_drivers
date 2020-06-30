/*
 ******************************************************************************
 * @file    test_self_test.c
 * @author  Sensors Software Solution Team
 * @brief   This file run selt test procedure
 *
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */

/*
 * This example was developed using the following STMicroelectronics
 * evaluation boards:
 *
 * - STEVAL_MKI109V3 + STEVAL-MKI206V1
 * - NUCLEO_F411RE + STEVAL-MKI206V1
 *
 * and STM32CubeMX tool with STM32CubeF4 MCU Package
 *
 * Used interfaces:
 *
 * STEVAL_MKI109V3    - Host side:   USB (Virtual COM)
 *                    - Sensor side: SPI(Default) / I2C(supported)
 *
 * NUCLEO_STM32F411RE - Host side: UART(COM) to USB bridge
 *                    - Sensor side: I2C(Default) / SPI(supported)
 *
 * If you need to run this example on a different hardware platform a
 * modification of the functions: `platform_write`, `platform_read`,
 * `tx_com` and 'platform_init' is required.
 *
 */

/* STMicroelectronics evaluation boards definition
 *
 * Please uncomment ONLY the evaluation boards in use.
 * If a different hardware is used please comment all
 * following target board and redefine yours.
 */
//#define STEVAL_MKI109V3
#define NUCLEO_F411RE

#if defined(__TRUSTINSOFT_ANALYZER__)
// We will not use this variable, but this is a placeholder for
// the communication interface.
static char tis_bus;
#define SENSOR_BUS tis_bus

#else
#if defined(STEVAL_MKI109V3)
/* MKI109V3: Define communication interface */
#define SENSOR_BUS hspi2

/* MKI109V3: Vdd and Vddio power supply values */
#define PWM_3V3 915

#elif defined(NUCLEO_F411RE)
/* NUCLEO_F411RE: Define communication interface */
#define SENSOR_BUS hi2c1

#endif
#endif

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>
#include "ais2dw12_reg.h"

#if defined(__TRUSTINSOFT_ANALYZER__)
// Nothing to include
// This function should probably not be called directly anyway.
#define HAL_Delay platform_delay

#else
#include "stm32f4xx_hal.h"
#include "gpio.h"
#include "i2c.h"
#if defined(STEVAL_MKI109V3)
#include "usbd_cdc_if.h"
#include "spi.h"
#elif defined(NUCLEO_F411RE)
#include "usart.h"
#endif
#endif

typedef union{
  int16_t i16bit[3];
  uint8_t u8bit[6];
} axis3bit16_t;

/* Private macro -------------------------------------------------------------*/
#define    BOOT_TIME      20 //ms

/* Self-test recommended samples */
#define SELF_TEST_SAMPLES  5

/* Self-test positive difference */
#define ST_MIN_POS      70.0f
#define ST_MAX_POS      1500.0f

/* Private variables ---------------------------------------------------------*/
static axis3bit16_t data_raw_acceleration[SELF_TEST_SAMPLES];
static float acceleration_mg[SELF_TEST_SAMPLES][3];
static uint8_t whoamI, rst;
static uint8_t tx_buffer[1000];

/* Extern variables ----------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
/*
 *   WARNING:
 *   Functions declare in this section are defined at the end of this file
 *   and are strictly related to the hardware platform used.
 *
 */
static int32_t platform_write(void *handle, uint8_t reg, uint8_t *bufp,
                              uint16_t len);
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
                             uint16_t len);
static void tx_com( uint8_t *tx_buffer, uint16_t len );
static void platform_delay(uint32_t ms);
static void platform_init(void);

/* Utility functions ---------------------------------------------------------*/
static inline float ABSF(float _x)
{
  return (_x < 0.0f) ? -(_x) : _x;
}

static int flush_samples(stmdev_ctx_t *dev_ctx)
{
  ais2dw12_reg_t reg;
  axis3bit16_t dummy;
  int samples = 0;

  /*
   * Discard old samples
   */
  ais2dw12_status_reg_get(dev_ctx, &reg.status);
  if (reg.status.drdy)
  {
    ais2dw12_acceleration_raw_get(dev_ctx, dummy.u8bit);
    samples++;
  }

  return samples;
}

static void test_self_test_ais2dw12(stmdev_ctx_t *dev_ctx)
{
  ais2dw12_reg_t reg;
  float media[3] = { 0.0f, 0.0f, 0.0f };
  float mediast[3] = { 0.0f, 0.0f, 0.0f };
  uint8_t match[3] = { 0, 0, 0 };
  uint8_t j = 0;
  uint16_t i = 0;
  uint8_t k = 0;
  uint8_t axis;

  /* Restore default configuration */
  ais2dw12_reset_set(dev_ctx, PROPERTY_ENABLE);
  do
  {
    ais2dw12_reset_get(dev_ctx, &rst);
  } while (rst);

  ais2dw12_block_data_update_set(dev_ctx, PROPERTY_ENABLE);
  ais2dw12_full_scale_set(dev_ctx, AIS2DW12_4g);
  ais2dw12_power_mode_set(dev_ctx, AIS2DW12_PWR_MD_4);
  ais2dw12_data_rate_set(dev_ctx, AIS2DW12_XL_ODR_50Hz);
  HAL_Delay(100);

  /* Flush old samples */
  flush_samples(dev_ctx);

  do
  {
    ais2dw12_status_reg_get(dev_ctx, &reg.status);
    if (reg.status.drdy)
    {
      /* Read accelerometer data */
      memset(data_raw_acceleration[i].u8bit, 0x00, 3 * sizeof(int16_t));
      ais2dw12_acceleration_raw_get(dev_ctx, data_raw_acceleration[i].u8bit);
      for (axis = 0; axis < 3; axis++)
        acceleration_mg[i][axis] =
          ais2dw12_from_fs4_to_mg(data_raw_acceleration[i].i16bit[axis]);

        i++;
      }
  } while (i < SELF_TEST_SAMPLES);

  for (k = 0; k < 3; k++)
  {
    for (j = 0; j < SELF_TEST_SAMPLES; j++)
    {
      media[k] += acceleration_mg[j][k];
    }

    media[k] = (media[k] / j);
  }

  /* Enable self test mode */
  ais2dw12_self_test_set(dev_ctx, AIS2DW12_XL_ST_POSITIVE);
  HAL_Delay(100);
  i = 0;

  /* Flush old samples */
  flush_samples(dev_ctx);

  do
  {
    ais2dw12_status_reg_get(dev_ctx, &reg.status);
    if (reg.status.drdy)
    {
      /* Read accelerometer data */
      memset(data_raw_acceleration[i].u8bit, 0x00, 3 * sizeof(int16_t));
      ais2dw12_acceleration_raw_get(dev_ctx, data_raw_acceleration[i].u8bit);
      for (axis = 0; axis < 3; axis++)
        acceleration_mg[i][axis] =
          ais2dw12_from_fs4_to_mg(data_raw_acceleration[i].i16bit[axis]);

      i++;
    }
  } while (i < SELF_TEST_SAMPLES);

  for (k = 0; k < 3; k++)
  {
      for (j = 0; j < SELF_TEST_SAMPLES; j++)
      {
        mediast[k] += acceleration_mg[j][k];
      }

    mediast[k] = (mediast[k] / j);
  }

  /* Check for all axis self test value range */
  for (k = 0; k < 3; k++)
  {
    if ((ABSF(mediast[k] - media[k]) >= ST_MIN_POS) &&
        (ABSF(mediast[k] - media[k]) <= ST_MAX_POS))
    {
      match[k] = 1;
    }

#if defined(__TRUSTINSOFT_ANALYZER__)
    // sprintf is an avoidable security risk
    snprintf((char*)tx_buffer, 1000,
#else
    sprintf((char*)tx_buffer,
#endif
            "%d: |%f| <= |%f| <= |%f| %s\r\n", k,
            ST_MIN_POS, ABSF(mediast[k] - media[k]), ST_MAX_POS,
            match[k] == 1 ? "PASSED" : "FAILED");
    tx_com(tx_buffer, strlen((char const*)tx_buffer));
  }

  /* Disable self test mode */
  ais2dw12_data_rate_set(dev_ctx, AIS2DW12_XL_ODR_OFF);
  ais2dw12_self_test_set(dev_ctx, AIS2DW12_XL_ST_DISABLE);
}

/* Main Example --------------------------------------------------------------*/
void ais2dw12_self_test(void)
{
  /* Initialize mems driver interface */
  stmdev_ctx_t dev_ctx;

  dev_ctx.write_reg = platform_write;
  dev_ctx.read_reg = platform_read;
  dev_ctx.handle = &SENSOR_BUS;

  /* Initialize platform specific hardware */
  platform_init();

  /* Wait sensor boot time */
  platform_delay(BOOT_TIME);

  /* Check device ID */
  ais2dw12_device_id_get(&dev_ctx, &whoamI);
  if (whoamI != AIS2DW12_ID)
  while(1)
  {
    /* manage here device not found */
  }

  /* Start self test */
  while(1)
  {
    test_self_test_ais2dw12(&dev_ctx);
  }
}

#if defined(__TRUSTINSOFT_ANALYZER__)
#include "tis_platform.c"
#else
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
  if (handle == &hi2c1)
  {
    HAL_I2C_Mem_Write(handle, AIS2DW12_I2C_ADD_L, reg,
                      I2C_MEMADD_SIZE_8BIT, bufp, len, 1000);
  }
#ifdef STEVAL_MKI109V3
  else if (handle == &hspi2)
  {
    HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(handle, &reg, 1, 1000);
    HAL_SPI_Transmit(handle, bufp, len, 1000);
    HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_SET);
  }
#endif
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
  if (handle == &hi2c1)
  {
    HAL_I2C_Mem_Read(handle, AIS2DW12_I2C_ADD_L, reg,
                     I2C_MEMADD_SIZE_8BIT, bufp, len, 1000);
  }
#ifdef STEVAL_MKI109V3
  else if (handle == &hspi2)
  {
  /* Read command */
  reg |= 0x80;
    HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(handle, &reg, 1, 1000);
    HAL_SPI_Receive(handle, bufp, len, 1000);
    HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_SET);
  }
#endif
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
  #ifdef NUCLEO_F411RE
  HAL_UART_Transmit(&huart2, tx_buffer, len, 1000);
  #endif
  #ifdef STEVAL_MKI109V3
  CDC_Transmit_FS(tx_buffer, len);
  #endif
}

/*
 * @brief  platform specific delay (platform dependent)
 *
 * @param  ms        delay in ms
 *
 */
static void platform_delay(uint32_t ms)
{
  HAL_Delay(ms);
}

/*
 * @brief  platform specific initialization (platform dependent)
 */
static void platform_init(void)
{
#if defined(STEVAL_MKI109V3)
  TIM3->CCR1 = PWM_3V3;
  TIM3->CCR2 = PWM_3V3;
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
  HAL_Delay(1000);
#endif
}

#endif
