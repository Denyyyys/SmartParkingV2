/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "crc.h"
#include "i2c.h"
#include "icache.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "hash/cmox_hash_retvals.h"
#include "hash/cmox_sha256.h"
#include "cmox_init.h"
#include "hash/cmox_hash.h"
#include "cmox_crypto.h"
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
bool gateOpened = true;
bool gateClosed = false;
bool gateMoves = false;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void SystemPower_Config(void);
/* USER CODE BEGIN PFP */

void openGate(){
	if (gateMoves || gateOpened) {
		return;
	}
	gateMoves = true;
	__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 18);
	gateMoves = false;
	gateOpened = true;
	gateClosed = false;
}

void closeGate() {
	if (gateMoves || gateClosed) {
		return;
	}

	gateMoves = true;
	__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 40);
	gateMoves = false;
	gateClosed = true;
	gateOpened = false;
}
void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == B3_Pin)
	{
		openGate();
		printf("yo3");
//		HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
	}

	if (GPIO_Pin == B2_Pin)
	{
		closeGate();
		printf("yo2");
//		HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
	}
}

int __io_putchar(int ch) {
	HAL_UART_Transmit(&huart1, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
	return 1;
}

void btox(uint8_t *hexbuf, const uint8_t *binbuf, int n) {
	n *= 2;
	hexbuf[n] = 0x00;
	const char hex[]= "0123456789abcdef";
	while ( -- n >= 0)
		hexbuf[n] = hex[(binbuf[n>>1] >> ((1-(n&1)) << 2)) & 0xF];
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
//void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
//{
//	if (GPIO_Pin == GPIO_PIN_13)
//	{
////		HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
//	}
//}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the System Power */
  SystemPower_Config();

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ICACHE_Init();
  MX_USART1_UART_Init();
  MX_I2C1_Init();
  MX_RTC_Init();
  MX_SPI1_Init();
  MX_TIM2_Init();
  MX_CRC_Init();
  MX_TIM1_Init();
  MX_TIM16_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);

//  lcd_init();
//  lcd_backlight(1);
//  char* msg = "Swieci";
//  RTC_TimeTypeDef sTime;
//  RTC_DateTypeDef sDate;
//  char msg[100];

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

//
// =========================== HASH BEGIN ===========================
//
//  cmox_hash_retval_t retval;
//  uint8_t data[] = "Alice has a cat.";
//  uint8_t hash[CMOX_SHA256_SIZE];
//  size_t computed_size;
//  uint8_t buffer[2*CMOX_SHA256_SIZE+1];
//  if (cmox_initialize(NULL) != CMOX_INIT_SUCCESS)
//	  Error_Handler();
//
//  retval = cmox_hash_compute(CMOX_SHA256_ALGO, data, strlen((char*)data), hash, CMOX_SHA256_SIZE, &computed_size);
//
//  if (retval != CMOX_HASH_SUCCESS)
//	  Error_Handler();
//  printf("Input data (ASCII): %s (length=%d) \n\r", data, strlen((char*)data));
//  btox(buffer, data, strlen((char*)data));
//  printf("Input data (hex) : %s\n\r", buffer);
// =========================== HASH END ===========================
// =========================== AES BEGIN ===========================
//  cmox_cbc_handle_t Cbc_Ctx;
//  #define CHUNK_SIZE  48u
//
//  const uint8_t Key[] =
//  {
//    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
//  };
//  const uint8_t IV[] =
//  {
//    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
//  };
//
//  const uint8_t Plaintext[] =
//  {
//    0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
//    0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c, 0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
//    0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11, 0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
//    0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17, 0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
//  };
//  const uint8_t Expected_Ciphertext[] =
//  {
//    0x76, 0x49, 0xab, 0xac, 0x81, 0x19, 0xb2, 0x46, 0xce, 0xe9, 0x8e, 0x9b, 0x12, 0xe9, 0x19, 0x7d,
//    0x50, 0x86, 0xcb, 0x9b, 0x50, 0x72, 0x19, 0xee, 0x95, 0xdb, 0x11, 0x3a, 0x91, 0x76, 0x78, 0xb2,
//    0x73, 0xbe, 0xd6, 0xb8, 0xe3, 0xc1, 0x74, 0x3b, 0x71, 0x16, 0xe6, 0x9e, 0x22, 0x22, 0x95, 0x16,
//    0x3f, 0xf1, 0xca, 0xa1, 0x68, 0x1f, 0xac, 0x09, 0x12, 0x0e, 0xca, 0x30, 0x75, 0x86, 0xe1, 0xa7
//  };
//
//  uint8_t Computed_Ciphertext[sizeof(Expected_Ciphertext)];
//  uint8_t Computed_Plaintext[sizeof(Plaintext)];
//
//  cmox_cipher_retval_t retval;
//  size_t computed_size;
//
//  cmox_cipher_handle_t *cipher_ctx;
//
//  uint32_t index;
//
//
//  cmox_init_arg_t init_target = {CMOX_INIT_TARGET_AUTO, NULL};
//
//  if (cmox_initialize(&init_target) != CMOX_INIT_SUCCESS)
//  {
//    Error_Handler();
//  }
//
//  cipher_ctx = cmox_cbc_construct(&Cbc_Ctx, CMOX_AES_CBC_ENC);
//  if (cipher_ctx == NULL)
//  {
//    Error_Handler();
//  }
//
//  retval = cmox_cipher_init(cipher_ctx);
//  if (retval != CMOX_CIPHER_SUCCESS)
//  {
//    Error_Handler();
//  }
//
//  retval = cmox_cipher_setKey(cipher_ctx, Key, sizeof(Key));  /* AES key to use */
//  if (retval != CMOX_CIPHER_SUCCESS)
//  {
//    Error_Handler();
//  }
//
//  retval = cmox_cipher_setIV(cipher_ctx, IV, sizeof(IV));     /* Initialization vector */
//	if (retval != CMOX_CIPHER_SUCCESS)
//	{
//	  Error_Handler();
//	}
//
//  for (index = 0; index < (sizeof(Plaintext) - CHUNK_SIZE); index += CHUNK_SIZE)
//  {
//	    retval = cmox_cipher_append(cipher_ctx,
//	                                &Plaintext[index], CHUNK_SIZE,        /* Chunk of plaintext to encrypt */
//	                                Computed_Ciphertext, &computed_size); /* Data buffer to receive generated chunk
//	                                                                         of ciphertext */
//
//	    if (retval != CMOX_CIPHER_SUCCESS)
//	    {
//	      Error_Handler();
//	    }
//
//	    /* Verify generated data size is the expected one */
//	    if (computed_size != CHUNK_SIZE)
//	    {
//	      Error_Handler();
//	    }
//
//
//	    if (memcmp(&Expected_Ciphertext[index], Computed_Ciphertext, computed_size) != 0)
//	    {
//	      Error_Handler();
//	    }
//  }
//
//  if (index < sizeof(Plaintext))
//	{
//		retval = cmox_cipher_append(cipher_ctx,
//										&Plaintext[index],
//										sizeof(Plaintext) - index,              /* Last part of plaintext to encrypt */
//										Computed_Ciphertext, &computed_size);   /* Data buffer to receive generated last
//																				   part of ciphertext */
//		if (retval != CMOX_CIPHER_SUCCESS)
//		{
//		  Error_Handler();
//		}
//
//		if (computed_size != (sizeof(Plaintext) - index))
//		{
//		  Error_Handler();
//		}
//
//		if (memcmp(&Expected_Ciphertext[index], Computed_Ciphertext, computed_size) != 0)
//		{
//		  Error_Handler();
//		}
//	}
//
// 	    retval = cmox_cipher_cleanup(cipher_ctx);
// 	    if (retval != CMOX_CIPHER_SUCCESS)
// 	    {
// 	      Error_Handler();
// 	    }
// =========================== AES END ===========================
//  char* msg = "sometext\r\n";
//  app_main();


  while (1)
  {
//	  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 2000);
//	  HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
//	  HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
//
//	  snprintf(msg, sizeof(msg),
//	          "Time:%02d:%02d:%02d\r\nDate:20%02d-%02d-%02d (WD:%d)\r\n",
//	          sTime.Hours,
//	          sTime.Minutes,
//	          sTime.Seconds,
//	          sDate.Year,
//	          sDate.Month,
//	          sDate.Date,
//	          sDate.WeekDay);
//
//	 HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
//	 printf("Dioda sie swieci %s\r\n", msg);
//	 lcd_clear();
//	 lcd_set_cursor(0, 0);
//	 lcd_write_string(msg);
//	 HAL_Delay(1500);


    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_0;
  RCC_OscInitStruct.LSIDiv = RCC_LSI_DIV1;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV4;
  RCC_OscInitStruct.PLL.PLLM = 3;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 1;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLLVCIRANGE_1;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Power Configuration
  * @retval None
  */
static void SystemPower_Config(void)
{

  /*
   * Switch to SMPS regulator instead of LDO
   */
  if (HAL_PWREx_ConfigSupply(PWR_SMPS_SUPPLY) != HAL_OK)
  {
    Error_Handler();
  }
/* USER CODE BEGIN PWR */
/* USER CODE END PWR */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
