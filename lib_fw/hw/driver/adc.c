/*
 * adc.c
 *
 *  Created on: 2021. 2. 12.
 *      Author: baram
 */


#include "adc.h"
#include "cli.h"



#ifdef _USE_HW_ADC


typedef struct
{
  bool                    is_init;
  ADC_HandleTypeDef      *hADCx;
  uint32_t                adc_channel;
} adc_tbl_t;



static adc_tbl_t adc_tbl[ADC_MAX_CH];
static uint16_t adc_data[ADC_MAX_CH];

ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

#ifdef _USE_HW_CLI
static void cliAdc(cli_args_t *args);
#endif


bool adcInit(void)
{
  uint32_t i;
  uint32_t ch;



  for (i=0; i<ADC_MAX_CH; i++)
  {
    adc_tbl[i].is_init = false;
  }


  ADC_ChannelConfTypeDef sConfig = {0};

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ENABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;

  HAL_ADC_DeInit(&hadc1);
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }


  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_144CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }


  if (HAL_ADC_Start_DMA(&hadc1,
                        (uint32_t *)adc_data,
                        ADC_MAX_CH
                       ) != HAL_OK)
  {
    Error_Handler();
  }


  ch = 0;
  adc_tbl[ch].is_init     = true;
  adc_tbl[ch].hADCx       = &hadc1;
  adc_tbl[ch].adc_channel = ADC_CHANNEL_4;


#ifdef _USE_HW_CLI
  cliAdd("adc", cliAdc);
#endif

  return true;
}

uint32_t adcRead(uint8_t ch)
{
  uint32_t adc_value;

  if (adc_tbl[ch].is_init != true)
  {
    return 0;
  }

  adc_value = adc_data[ch];

  return adc_value;
}

uint32_t adcRead8(uint8_t ch)
{
  return adcRead(ch)>>4;
}

uint32_t adcRead10(uint8_t ch)
{
  return adcRead(ch)>>2;
}

uint32_t adcRead12(uint8_t ch)
{
  return adcRead(ch);
}

uint32_t adcRead16(uint8_t ch)
{
  return adcRead(ch)<<4;
}

uint32_t adcReadVoltage(uint8_t ch)
{
  return adcConvVoltage(ch, adcRead(ch));
}

uint32_t adcReadCurrent(uint8_t ch)
{

  return adcConvCurrent(ch, adcRead(ch));
}

uint32_t adcConvVoltage(uint8_t ch, uint32_t adc_value)
{
  uint32_t ret = 0;

  switch(ch)
  {
    case 0:
    case 1:
      ret  = (uint32_t)((adc_value * 3300 * 10) / (4095*10));
      ret += 5;
      ret /= 10;
      break;

    case 2:
      ret  = (uint32_t)((adc_value * 3445 * 26) / (4095*10));
      ret += 5;
      ret /= 10;
      break;

  }

  return ret;
}

uint32_t adcConvCurrent(uint8_t ch, uint32_t adc_value)
{
  return 0;
}

uint8_t  adcGetRes(uint8_t ch)
{
  return 12;
}




void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(adcHandle->Instance==ADC1)
  {
  /* USER CODE BEGIN ADC1_MspInit 0 */
    __HAL_RCC_DMA2_CLK_ENABLE();
  /* USER CODE END ADC1_MspInit 0 */
    /* ADC1 clock enable */
    __HAL_RCC_ADC1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**ADC1 GPIO Configuration
    PA4     ------> ADC1_IN4
    */
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* ADC1 DMA Init */
    /* ADC1 Init */
    hdma_adc1.Instance = DMA2_Stream0;
    hdma_adc1.Init.Channel = DMA_CHANNEL_0;
    hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode = DMA_CIRCULAR;
    hdma_adc1.Init.Priority = DMA_PRIORITY_LOW;
    hdma_adc1.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_adc1) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(adcHandle,DMA_Handle,hdma_adc1);

  /* USER CODE BEGIN ADC1_MspInit 1 */
    //HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 5, 0);
    //HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
  /* USER CODE END ADC1_MspInit 1 */
  }
}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef* adcHandle)
{

  if(adcHandle->Instance==ADC1)
  {
  /* USER CODE BEGIN ADC1_MspDeInit 0 */

  /* USER CODE END ADC1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_ADC1_CLK_DISABLE();

    /**ADC1 GPIO Configuration
    PA4     ------> ADC1_IN4
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_4);

    /* ADC1 DMA DeInit */
    HAL_DMA_DeInit(adcHandle->DMA_Handle);
  /* USER CODE BEGIN ADC1_MspDeInit 1 */

  /* USER CODE END ADC1_MspDeInit 1 */
  }
}



#ifdef _USE_HW_CLI
void cliAdc(cli_args_t *args)
{
  bool ret = true;


  if (args->argc == 1)
  {
    if (args->isStr(0, "show") == true)
    {
      while(cliKeepLoop())
      {
        for (int i=0; i<ADC_MAX_CH; i++)
        {
          cliPrintf("%04d ", adcRead(i));
        }
        cliPrintf("\r\n");
        delay(50);
      }
    }
    else
    {
      ret = false;
    }
  }
  else if (args->argc == 2)
  {
    if (args->isStr(0, "show") == true && args->isStr(1, "voltage") == true)
    {
      while(cliKeepLoop())
      {
        for (int i=0; i<ADC_MAX_CH; i++)
        {
          uint32_t adc_data;

          adc_data = adcReadVoltage(i);

          cliPrintf("%d.%02dV ", adc_data/100, adc_data%100);
        }
        cliPrintf("\r\n");
        delay(50);
      }
    }
    else
    {
      ret = false;
    }
  }
  else
  {
    ret = false;
  }


  if (ret == false)
  {
    cliPrintf( "adc show\n");
    cliPrintf( "adc show voltage\n");
  }
}
#endif


#endif
