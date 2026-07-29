/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "string.h"
#include "stdlib.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define CANTIDAD 10

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */



uint8_t letra;
uint8_t num_letra = 0;
char orden[CANTIDAD];


unsigned char distancia_min = 5;
unsigned char distancia_max = 30;
uint32_t timeout = 10000;


enum msg_index{
  ahead,
  right,
  left,
  back,
  stop,
  min_update,
  max_update,
  invalid,
  auto_mode,
  manual_mode,
  manual_not_set,
  manual_already_set,
  auto_already_set
};
char modo_manual = 1;
enum msg_index mode;
char* msgs[] = {
    "going ahead\n",
    "turning right\n",
    "turning left\n",
    "going backwards\n",
    "stopping!\n",
    "MIN distance updated\n",
    "MAX distance updated\n",
    "invalid command",
    "mode AUTO: ON\n",
    "mode MANUAL: ON\n",
    "manual mode NOT set :(\n",
    "manual mode already set\n",
    "auto mode already set\n"
};


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC_Init(void);
static void MX_TS_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

void send_msg(enum msg_index i);
enum msg_index ejecutar_orden(char* orden);
void parar(void);
void girar_der(unsigned char velocidad);
void girar_izq(unsigned char velocidad);
void retroceder(unsigned char velocidad);
void avanzar(unsigned char velocidad); //OK
void retroceder_der(unsigned char velocidad);//OK
void avanzar_der(unsigned char velocidad); //OK
void retroceder_izq(unsigned char velocidad); //OK
void avanzar_izq(unsigned char velocidad); // OK


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */



////////////////////////////////////////////////////////////////////////////////
// Funciones de las ruedas:
// IZQ:
//    PB12 en DIGITAL OUTPUT 01
//    PB10 en función alternativa 10 - TIM2_CH3
// DER:
//    PB13 en DIGITAL OUTPUT 01
//    PB11 en función alternativa 10 - TIM2_CH4
////////////////////////////////////////////////////////////////////////////////

void avanzar_izq(unsigned char velocidad){ // OK
  GPIOB->BSRR |=  (1<<(12+16));
  TIM2->CCR3 = velocidad;
}


void retroceder_izq(unsigned char velocidad){ //OK
  GPIOB->BSRR |=  (1<<12);
  TIM2->CCR3 = 100 - velocidad;
}

void avanzar_der(unsigned char velocidad){ //OK
  GPIOB->BSRR |=  (1<<(13+16));
  TIM2->CCR4 = velocidad;
}


void retroceder_der(unsigned char velocidad){//OK
  GPIOB->BSRR |=  (1<<13);
  TIM2->CCR4 = 100 - velocidad;
}


void avanzar(unsigned char velocidad){ //OK
  avanzar_izq(velocidad);
  avanzar_der(velocidad);
}


void retroceder(unsigned char velocidad){
  retroceder_izq(velocidad);
  retroceder_der(velocidad);
}

void girar_izq(unsigned char velocidad){
  avanzar_der(velocidad);
  retroceder_izq(velocidad);
}

void girar_der(unsigned char velocidad){
  avanzar_izq(velocidad);
  retroceder_der(velocidad);
}

void parar(void){
  avanzar_izq(1);
  avanzar_der(1);
}


void reset_auto_mode(){
  parar();
  modo_manual = 0;
}


enum msg_index ejecutar_orden(char* orden){


  if(strcmp(orden, "MODE_MANUAL") == 0){
    if(modo_manual == 1)
      return manual_already_set;
    parar();
//    sonido_off();
    modo_manual = 1;
    return manual_mode;
  }
  if(strcmp(orden, "MODE_AUTO") == 0 ){
    if(modo_manual == 0)
      return auto_already_set;


    reset_auto_mode();
    return auto_mode;
  }

  // si llega aquí solo puede ejecutar una acción del modo manual si está en modo manual.
  // Si no, termina indicando que no está en modo manual
  if(modo_manual == 0)
    return manual_not_set;

  if(strncmp(orden, "MIN", 3) == 0){
    char num[4];
    for(int i = 0; i<4;i++)
      num[i] = '\0';
    strcpy(num, orden+3);
    distancia_min = strtol(num, NULL, 10);
    return min_update;
  }
  if(strncmp(orden, "MAX", 3) == 0){
    char num[4];
    for(int i = 0; i<4;i++)
      num[i] = '\0';
    strcpy(num, orden+3);
    distancia_max = strtol(num, NULL, 10);
    return max_update;
  }
  if(strcmp(orden, "GO")==0 ){
    avanzar(99);
    return ahead;
  }
  if(strcmp(orden, "BACK")==0 ){
    retroceder(99);
    return back;
  }
  if(strcmp(orden, "IZQ")==0 ){
    girar_izq(99);
    return left;
  }
  if(strcmp(orden, "DER")==0 ){
    girar_der(99);
    return right;
  }
  if(strcmp(orden, "STP")==0 ){
    parar();
    return stop;
  }
  return invalid;
}


void send_msg(enum msg_index i){
  //el casting es par que no salte el warning
  HAL_UART_Transmit(&huart1, (unsigned char*)msgs[i], strlen(msgs[i]) , timeout);
}


////////////////////////////////////////////////////////////////////////////////////////////////
// Lee las letras de 1 en 1 hasta el \n
////////////////////////////////////////////////////////////////////////////////////////////////

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
  if(letra == '\n'){
    orden[num_letra-1] = '\0';    //quita el último caracter
    mode = ejecutar_orden(orden);     //ejecuta la orden
    send_msg(mode);

   for(int i = 0; i<CANTIDAD; i++)  //resetea el buffer a \0s
    orden[i] = '\0';
   num_letra = 0;

  }
  else{//va guardando las letras en el buffer
    orden[num_letra] = letra;
    num_letra++;
  }
  HAL_UART_Receive_IT(huart, &letra, 1);

}

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

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC_Init();
  MX_TS_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */


  ////////////////////////////////////////////////////////////////////////////////
    //PB12 en DIGITAL OUTPUT 01
      //PB10 en función alternativa 10
      //
    //PB13 en DIGITAL OUTPUT 01
      //PB11 en función alternativa 10
    ////////////////////////////////////////////////////////////////////////////////
    GPIOB->MODER &= ~(1<<(12*2+1));
    GPIOB->MODER |=  (1<<(12*2));

    GPIOB->MODER |=  (1<<(10*2+1));
    GPIOB->MODER &= ~(1<<(10*2));

    GPIOB->MODER &= ~(1<<(13*2+1));
    GPIOB->MODER |=  (1<<(13*2));

    GPIOB->MODER |=  (1<<(11*2+1));
    GPIOB->MODER &= ~(1<<(11*2));

    GPIOB->AFR[1] = 0x00001100;



    ////////////////////////////////////////////////////////////////////////////////
    // CONFIGURACIÓN TIM2_CH3 en PWM
    ////////////////////////////////////////////////////////////////////////////////
    TIM2->CR1 = 0;
    TIM2->CR2 = 0;
    TIM2->SMCR = 0;

    //conf. de la propia PWM
    TIM2->CNT = 0;
    TIM2->PSC = 32-1;
    TIM2->ARR = 100-1;
    TIM2->CCR3 = 1; //EMPIEZA PARADO
    TIM2->CCR4 = 1; //EMPIEZA PARADO

    // no nos hace falta la interrupción
    TIM2->DCR = 0;
    TIM2->DIER = 0x0000;

    TIM2->CCMR2 = 0;

    //CH3
    TIM2->CCMR2 |= (1<<3);  //PE
    //Los 3 siguientes para PWM (ver manual)
    TIM2->CCMR2 |= (1<<6);  //1
    TIM2->CCMR2 |= (1<<5);  //1
    TIM2->CCMR2 &= ~(1<<4); //0

    //CH4
    TIM2->CCMR2 |= (1<<11); //PE
    //Los 3 siguientes para PWM (ver manual)
    TIM2->CCMR2 |= (1<<14); //1
    TIM2->CCMR2 |= (1<<13); //1
    TIM2->CCMR2 &= ~(1<<12);  //0

    TIM2->CCER |= (1<<8);   //CC3E
    TIM2->CCER |= (1<<12);    //CC4E

    TIM2->CR1 |= (1<<7);    //HW (bit ARPE)
    TIM2->EGR |= (1<<0);    //UG
    TIM2->CR1 |= (1<<0);    //ON
    TIM2->SR = 0;       //FLAG


  uint8_t* msg = (uint8_t*)"ready!\n";// casting para que no haya warning

  //un buffer para la orden recibida desde el móvil
  for(int i = 0; i<CANTIDAD; i++)
    orden[i] = '\0';

  //transmisión del mensaje de 'ready!'
  HAL_UART_Transmit(&huart1, msg, strlen((char*)msg) , timeout);

  HAL_UART_Receive_IT(&huart1, &letra, 1);


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

    if(modo_manual == 0){

    }
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PLLDIV = RCC_PLL_DIV3;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC_Init(void)
{

  /* USER CODE BEGIN ADC_Init 0 */

  /* USER CODE END ADC_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC_Init 1 */

  /* USER CODE END ADC_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc.Instance = ADC1;
  hadc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  hadc.Init.LowPowerAutoWait = ADC_AUTOWAIT_DISABLE;
  hadc.Init.LowPowerAutoPowerOff = ADC_AUTOPOWEROFF_DISABLE;
  hadc.Init.ChannelsBank = ADC_CHANNELS_BANK_A;
  hadc.Init.ContinuousConvMode = DISABLE;
  hadc.Init.NbrOfConversion = 1;
  hadc.Init.DiscontinuousConvMode = DISABLE;
  hadc.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_CC3;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
  hadc.Init.DMAContinuousRequests = DISABLE;
  if (HAL_ADC_Init(&hadc) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_4CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC_Init 2 */

  /* USER CODE END ADC_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TS Initialization Function
  * @param None
  * @retval None
  */
static void MX_TS_Init(void)
{

  /* USER CODE BEGIN TS_Init 0 */

  /* USER CODE END TS_Init 0 */

  /* USER CODE BEGIN TS_Init 1 */

  /* USER CODE END TS_Init 1 */
  /* USER CODE BEGIN TS_Init 2 */

  /* USER CODE END TS_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pins : SEG14_Pin SEG15_Pin SEG16_Pin SEG17_Pin
                           SEG18_Pin SEG19_Pin SEG20_Pin SEG21_Pin
                           SEG22_Pin SEG23_Pin */
  GPIO_InitStruct.Pin = SEG14_Pin|SEG15_Pin|SEG16_Pin|SEG17_Pin
                          |SEG18_Pin|SEG19_Pin|SEG20_Pin|SEG21_Pin
                          |SEG22_Pin|SEG23_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF11_LCD;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SEG0_Pin SEG1_Pin SEG2_Pin COM0_Pin
                           COM1_Pin COM2_Pin SEG12_Pin */
  GPIO_InitStruct.Pin = SEG0_Pin|SEG1_Pin|SEG2_Pin|COM0_Pin
                          |COM1_Pin|COM2_Pin|SEG12_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF11_LCD;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : SEG6_Pin SEG7_Pin SEG8_Pin SEG9_Pin
                           SEG10_Pin SEG11_Pin SEG3_Pin SEG4_Pin
                           SEG5_Pin SEG13_Pin COM3_Pin */
  GPIO_InitStruct.Pin = SEG6_Pin|SEG7_Pin|SEG8_Pin|SEG9_Pin
                          |SEG10_Pin|SEG11_Pin|SEG3_Pin|SEG4_Pin
                          |SEG5_Pin|SEG13_Pin|COM3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF11_LCD;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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
