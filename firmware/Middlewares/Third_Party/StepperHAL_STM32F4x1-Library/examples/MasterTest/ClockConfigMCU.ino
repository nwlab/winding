 void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

  memset(&RCC_OscInitStruct, 0, sizeof(RCC_OscInitTypeDef));
  memset(&RCC_ClkInitStruct, 0, sizeof(RCC_ClkInitTypeDef));

  /* Abilita il clock PWR e imposta la scala di tensione */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* Configura HSE e PLL
     Assunzione: HSE = 25 MHz (da PLLM = 25)
     PLL input clock = HSE / PLLM = 1 MHz
     VCO = PLLN * PLL input = 200 MHz
     SYSCLK = VCO / PLLP = 200 / 2 = 100 MHz
     PLLQ = 4 (per USB/OTG, se usi)
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM       = 25;   /* HSE / 25 -> 1 MHz PLL input */
  RCC_OscInitStruct.PLL.PLLN       = 200;  /* VCO = 200 MHz */
  RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV2; /* SYSCLK = 100 MHz */
  RCC_OscInitStruct.PLL.PLLQ       = 4;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /* Configure SYSCLK, HCLK, PCLK1 and PCLK2
     - SYSCLK source = PLLCLK (100 MHz)
     - AHB prescaler = 1 -> HCLK = 100 MHz
     - APB1 prescaler = 2 -> PCLK1 = 50 MHz (max APB1 for F4)
     - APB2 prescaler = 1 -> PCLK2 = 100 MHz (max APB2 for F4)
  */
  RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  /* FLASH latency per 100 MHz su F4: 3 WS */
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK) {
    Error_Handler();
  }

  /* Optional: aggiornamento del SysTick se necessario (HAL lo fa di solito) */
  /* HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);
     HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
  */
}