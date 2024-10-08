/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "abuzz.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

enum event {
    ev_none = 0,
    ev_button_push,
    ev_state_timeout,
	ev_error = -99
};

#define EVQ_SIZE 10

enum event evq[ EVQ_SIZE];
int 	evq_count		= 0;
int 	evq_front_ix	= 0;
int 	evq_rear_ix		= 0;



typedef enum state {
    s_init,
    s_car_go,
    s_pushed_wait,
    s_cars_stopping,
    s_car_stopped,
    s_go_people,
    s_stop_people,
    s_cars_wait
} State_Type;

// Global variables for state and events
State_Type current_state = s_init;
enum event current_event = ev_none;

// Switch case state machine
void set_traffic_lights(enum state current_state) {
	HAL_GPIO_WritePin(P_GREEN_GPIO_Port, P_GREEN_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(P_RED_GPIO_Port,   P_RED_Pin,   GPIO_PIN_RESET);
	HAL_GPIO_WritePin(C_GREEN_GPIO_Port, C_GREEN_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(C_RED_GPIO_Port, C_RED_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(C_YELLOW_GPIO_Port, C_YELLOW_Pin, GPIO_PIN_RESET);
    switch (current_state) {
        case s_init:
        	HAL_GPIO_WritePin(P_GREEN_GPIO_Port, P_GREEN_Pin, GPIO_PIN_SET);
        	HAL_GPIO_WritePin(P_RED_GPIO_Port,   P_RED_Pin,   GPIO_PIN_SET);
        	HAL_GPIO_WritePin(C_GREEN_GPIO_Port, C_GREEN_Pin, GPIO_PIN_SET);
        	HAL_GPIO_WritePin(C_RED_GPIO_Port, C_RED_Pin, GPIO_PIN_SET);
        	HAL_GPIO_WritePin(C_YELLOW_GPIO_Port, C_YELLOW_Pin, GPIO_PIN_SET);
        	break;
        case s_car_go:
        	HAL_GPIO_WritePin(C_GREEN_GPIO_Port, C_GREEN_Pin, GPIO_PIN_SET);
        	HAL_GPIO_WritePin(P_RED_GPIO_Port,   P_RED_Pin,   GPIO_PIN_SET);
            break;
        case s_pushed_wait:
        	HAL_GPIO_WritePin(C_GREEN_GPIO_Port, C_GREEN_Pin, GPIO_PIN_SET);
        	HAL_GPIO_WritePin(P_RED_GPIO_Port,   P_RED_Pin,   GPIO_PIN_SET);
            break;
        case s_cars_stopping:
        	HAL_GPIO_WritePin(C_YELLOW_GPIO_Port, C_YELLOW_Pin, GPIO_PIN_SET);
        	HAL_GPIO_WritePin(P_RED_GPIO_Port,   P_RED_Pin,   GPIO_PIN_SET);
            break;
        case s_car_stopped:
        	HAL_GPIO_WritePin(C_RED_GPIO_Port, C_RED_Pin, GPIO_PIN_SET);
        	HAL_GPIO_WritePin(P_RED_GPIO_Port,   P_RED_Pin,   GPIO_PIN_SET);
            break;
        case s_go_people:
        	HAL_GPIO_WritePin(P_GREEN_GPIO_Port, P_GREEN_Pin, GPIO_PIN_SET);
        	HAL_GPIO_WritePin(C_RED_GPIO_Port, C_RED_Pin, GPIO_PIN_SET);
            break;
        case s_stop_people:
        	HAL_GPIO_WritePin(P_RED_GPIO_Port,   P_RED_Pin,   GPIO_PIN_SET);
        	HAL_GPIO_WritePin(C_RED_GPIO_Port, C_RED_Pin, GPIO_PIN_SET);
            break;
        case s_cars_wait:
        	HAL_GPIO_WritePin(P_RED_GPIO_Port,   P_RED_Pin,   GPIO_PIN_SET);
        	HAL_GPIO_WritePin(C_YELLOW_GPIO_Port, C_YELLOW_Pin, GPIO_PIN_SET);
            break;
        default:
            break;
    }
}



/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
int  ticks_left_in_state = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */
//HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int is_button_pressed()
{
    uint32_t reg_reading = GPIOC->IDR;
    uint16_t blue_button = 1 << 13;

    int bitValue = (reg_reading & blue_button) ? 1 : 0;
    if(bitValue)
    {
        return 1;
    }
    else
    {

    	return 0;

    }

}


void evq_init()
{
	for (int i=0; i < EVQ_SIZE; i++)
	{
		evq[i] = ev_error;
	}

}


void evq_push_back(enum event e)
{
	// if queue is flll ignroe e
	if(evq_count < EVQ_SIZE)
	{
		evq[evq_rear_ix] = e;
		evq_rear_ix++;
		evq_rear_ix %= EVQ_SIZE;
		evq_count++;
	}
}

enum event evq_pop_front()
{
	enum event e = ev_none;
	if (evq_count > 0)
	{
		e = evq[evq_front_ix];
		evq[evq_front_ix] = ev_error;
		evq_front_ix++;
		evq_front_ix %= EVQ_SIZE;
		evq_count--;
	}
		return e;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    static uint32_t last_interrupt_time = 0;
    uint32_t current_time = HAL_GetTick(); // Get current tick

    if (GPIO_Pin == B1_Pin && (current_time - last_interrupt_time) > 200) // 200 ms debounce
    {
        last_interrupt_time = current_time;  // Update last interrupt time
        evq_push_back(ev_button_push);
    }
}


int systick_count = 0;

void my_systick_handler()
{
	enum event eve;


		  if(ticks_left_in_state == 1)
			 {
			    eve = ev_state_timeout;
			    evq_push_back(eve);
			    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
		  	}
			if(ticks_left_in_state > 0)
		  	{
			   ticks_left_in_state--;
			}




}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

	uint32_t last_tick = 0;
	int 	 last_button_press;
	int 	 current_button_press;

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM3_Init();
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  /* USER CODE BEGIN 2 */
  enum state current_state = s_init;
  //current_button_press = is_button_pressed();
  set_traffic_lights(s_init);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  enum event ev = ev_none;

	  	  //current_button_press = is_button_pressed();

	  	  uint32_t current_tick = HAL_GetTick();

//	  	  if(current_tick != last_tick)
//	  	  {
//	  		if(ticks_left_in_state == 1)
//	  		 {
//	  			  ev = ev_state_timeout;
//	  		 }
//	  		if(ticks_left_in_state > 0)
//	  		{
//	  		 ticks_left_in_state--;
//	  		}
//	  	  }
//
//	  	  if((current_button_press == 0) && (last_button_press != current_button_press))
//	  	  {
//	  		ev = ev_button_push;
//	  	  }
	  	  ev = evq_pop_front();




	        switch (current_state) {
	                case s_init:
	              	  if(ev == ev_button_push)
	              	  {
	              		ev = ev_none;
	              		ticks_left_in_state = 1500;
	              		current_state = s_car_stopped;
	              		set_traffic_lights(s_car_stopped);
	              	  }
	                	break;
	                case s_car_go:
	              	  if(ev == ev_button_push)
	              	  {
	              		  ev 				   = ev_none;
	              		  current_state 	   = s_pushed_wait;
	              		  ticks_left_in_state = 2000;
	              		  set_traffic_lights(s_pushed_wait);
	              		  abuzz_start();
	              		  abuzz_p_long();
	              	  }
	                	  break;
	                case s_pushed_wait:
	              	  if (ev == ev_state_timeout)
	              	  {
	              		  ev = ev_none;
	              		  current_state = s_cars_stopping;
	              		  ticks_left_in_state = 1000;
	              		  set_traffic_lights(s_cars_stopping);
	              	  }
	                	  break;
	                case s_cars_stopping:
	              	  if(ev == ev_state_timeout)
	              	  {
	              		  ev = ev_none;
	              		  current_state = s_car_stopped;
	              		  ticks_left_in_state = 1000;
	              		  set_traffic_lights(s_car_stopped);
	              		  abuzz_stop();
	              	  }
	                	  break;
	                case s_car_stopped:
	                    if ( ev == ev_state_timeout )
	                    {
	                        ev = ev_none; // clear event
	                        current_state = s_go_people; // set next state
	                        ticks_left_in_state = 2500; // set next timeout
	                        set_traffic_lights(s_go_people);
	                        abuzz_start();
	                        abuzz_p_short(); // set output
	                    }
	                	 break;
	                case s_go_people:
	              	  if(ev == ev_state_timeout)
	              	  {
	              		  ev = ev_none;
	              		  current_state = s_stop_people;
	              		  ticks_left_in_state = 5000;
	              		  set_traffic_lights(s_stop_people);
	              		  abuzz_stop();
	              	  }
	                	break;
	                case s_stop_people:
	              	  if(ev == ev_state_timeout)
	              	  {
	              		  ev 		= ev_none;
	              		  current_state    	= s_cars_wait;
	              		  ticks_left_in_state = 2000;
	              		  set_traffic_lights(s_cars_wait);
	              	  }
	                	 break;
	                case s_cars_wait:
	  			  	  if(ev == ev_state_timeout)
	  			  	  {
	  			  		  ev = ev_none;
	  			  		  current_state= s_car_go;
	  			  		  //ticks_left_in_state = 2000;
	  			  		  set_traffic_lights(s_car_go);
	  			  	  }
	                	break;
	                default:
	                    break;
	            }

	        // Here you can change the event to test state transitions
	        //current_event = ev_button_push;
	        last_tick = current_tick;
	        last_button_press = current_button_press;
  }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, C_YELLOW_Pin|C_GREEN_Pin|LD2_Pin|P_GREEN_Pin
                          |P_RED_Pin|C_RED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : C_YELLOW_Pin C_GREEN_Pin LD2_Pin P_GREEN_Pin
                           P_RED_Pin C_RED_Pin */
  GPIO_InitStruct.Pin = C_YELLOW_Pin|C_GREEN_Pin|LD2_Pin|P_GREEN_Pin
                          |P_RED_Pin|C_RED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
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

#ifdef  USE_FULL_ASSERT
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
