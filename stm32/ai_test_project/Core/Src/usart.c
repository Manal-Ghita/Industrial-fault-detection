/**
  ******************************************************************************
  * @file    app_x-cube-ai.c
  * @author  X-CUBE-AI C code generator
  * @brief   AI program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#ifdef __cplusplus
 extern "C" {
#endif

#if defined ( __ICCARM__ )
#elif defined ( __CC_ARM ) || ( __GNUC__ )
#endif

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>

#include "app_x-cube-ai.h"
#include "main.h"
#include "ai_datatypes_defines.h"
#include "network.h"
#include "network_data.h"

/* USER CODE BEGIN includes */
#include <stdarg.h>
#include "usart.h"

extern UART_HandleTypeDef huart2;

#define BYTES_IN_FLOATS  (6 * 4)
#define TIMEOUT          5000
#define SYNCHRONISATION  0xAB
#define ACKNOWLEDGE      0xCD
#define CLASS_NUMBER     5

static void uart_printf(const char *fmt, ...);
/* USER CODE END includes */

/* IO buffers ----------------------------------------------------------------*/
#if !defined(AI_NETWORK_INPUTS_IN_ACTIVATIONS)
AI_ALIGNED(4) ai_i8 data_in_1[AI_NETWORK_IN_1_SIZE_BYTES];
ai_i8* data_ins[AI_NETWORK_IN_NUM] = { data_in_1 };
#else
ai_i8* data_ins[AI_NETWORK_IN_NUM] = { NULL };
#endif

#if !defined(AI_NETWORK_OUTPUTS_IN_ACTIVATIONS)
AI_ALIGNED(4) ai_i8 data_out_1[AI_NETWORK_OUT_1_SIZE_BYTES];
ai_i8* data_outs[AI_NETWORK_OUT_NUM] = { data_out_1 };
#else
ai_i8* data_outs[AI_NETWORK_OUT_NUM] = { NULL };
#endif

AI_ALIGNED(32)
static uint8_t pool0[AI_NETWORK_DATA_ACTIVATION_1_SIZE];
ai_handle data_activations0[] = {pool0};

static ai_handle  network  = AI_HANDLE_NULL;
static ai_buffer *ai_input;
static ai_buffer *ai_output;

static void ai_log_err(const ai_error err, const char *fct)
{
  /* USER CODE BEGIN log */
  (void)err; (void)fct;
  /* USER CODE END log */
}

static int ai_boostrap(ai_handle *act_addr)
{
  ai_error err = ai_network_create_and_init(&network, act_addr, NULL);
  if (err.type != AI_ERROR_NONE) { ai_log_err(err, "create_and_init"); return -1; }
  ai_input  = ai_network_inputs_get(network,  NULL);
  ai_output = ai_network_outputs_get(network, NULL);
#if defined(AI_NETWORK_INPUTS_IN_ACTIVATIONS)
  for (int idx = 0; idx < AI_NETWORK_IN_NUM;  idx++) data_ins[idx]  = ai_input[idx].data;
#else
  for (int idx = 0; idx < AI_NETWORK_IN_NUM;  idx++) ai_input[idx].data  = data_ins[idx];
#endif
#if defined(AI_NETWORK_OUTPUTS_IN_ACTIVATIONS)
  for (int idx = 0; idx < AI_NETWORK_OUT_NUM; idx++) data_outs[idx] = ai_output[idx].data;
#else
  for (int idx = 0; idx < AI_NETWORK_OUT_NUM; idx++) ai_output[idx].data = data_outs[idx];
#endif
  return 0;
}

static int ai_run(void)
{
  ai_i32 batch = ai_network_run(network, ai_input, ai_output);
  if (batch != 1) { return -1; }
  return 0;
}

/* USER CODE BEGIN 2 */

static void uart_print(const char *s)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)s, (uint16_t)strlen(s), HAL_MAX_DELAY);
}

static void uart_printf(const char *fmt, ...)
{
  char buf[200];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  uart_print(buf);
}

static int synchronize_UART(void)
{
  unsigned char rx[2] = {0};
  unsigned char tx[2] = {ACKNOWLEDGE, 0};
  while (1)
  {
    HAL_StatusTypeDef s = HAL_UART_Receive(&huart2, (uint8_t *)rx, 2, TIMEOUT);
    if (s != HAL_OK) return 1;
    if (rx[0] == SYNCHRONISATION)
    {
      HAL_UART_Transmit(&huart2, (uint8_t *)tx, 2, TIMEOUT);
      return 0;
    }
  }
}

static int acquire_and_process_data(ai_i8 *data[])
{
  uint8_t tmp[BYTES_IN_FLOATS] = {0};
  HAL_StatusTypeDef s = HAL_UART_Receive(&huart2, tmp, sizeof(tmp), TIMEOUT);
  if (s != HAL_OK) return 1;
  memcpy(data[0], tmp, sizeof(tmp));
  return 0;
}

static int post_process(ai_i8 *data[])
{
  if (data == NULL || data[0] == NULL) return 1;
  uint8_t  outs[CLASS_NUMBER] = {0};
  uint8_t *output = (uint8_t *)data[0];
  for (size_t i = 0; i < CLASS_NUMBER; i++)
  {
    float val;
    memcpy(&val, &output[i * 4], sizeof(float));
    outs[i] = (uint8_t)(val * 255.0f);
  }
  HAL_StatusTypeDef s = HAL_UART_Transmit(&huart2, outs, sizeof(outs), TIMEOUT);
  if (s != HAL_OK) return 1;
  return 0;
}

/* USER CODE END 2 */

void MX_X_CUBE_AI_Init(void)
{
    /* USER CODE BEGIN 5 */
  HAL_Delay(3000);
  uart_printf("\r\n=== Predictive Maintenance AI4I ===\r\n");
  uart_printf("Input : 6 features | Output: 5 classes\r\n");
  uart_printf("HDF / No Failure / OSF / PWF / TWF\r\n");

  if (ai_boostrap(data_activations0) != 0)
  {
    uart_printf("Bootstrap FAILED\r\n");
    Error_Handler();
  }
  uart_printf("Network OK. Waiting for data...\r\n");
    /* USER CODE END 5 */
}

void MX_X_CUBE_AI_Process(void)
{
    /* USER CODE BEGIN 6 */
  if (network)
  {
    while (1)
    {
      if (synchronize_UART() != 0) continue;
      if (acquire_and_process_data(data_ins) != 0) continue;
      if (ai_run() != 0) continue;
      post_process(data_outs);
    }
  }
    /* USER CODE END 6 */
}

#ifdef __cplusplus
}
#endif
