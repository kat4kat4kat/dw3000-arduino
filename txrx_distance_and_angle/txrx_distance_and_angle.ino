#include "DW3000_UWB.h"

/*** Pinouts: ******
SCK  -> PA5 (D13)
MISO -> PA6 (D12)
MOSI -> PA7 (D11)
CS   -> PB6 (D10)
IRQ  -> PC7 (D9)
RST  -> PA9 (D8)
*******************/

#define DW3ASRX 0
#define DW3ASTX 1
#define DW3CFG  DW3ASRX   // Modify Function Here

double distance = 0.0F;
int16_t angle = 0U;

void setup()
{
  Serial.begin(115200);
  Serial.print("Initializing DW3000... ");

  DW3_Init();
  DW3_Register(0x1235, 0xABCD);

  Serial.println("DONE");

  delay(100);

}

void loop()
{
  #if DW3CFG == DW3ASRX

  DW3_StatusTypeDef ret = DW3_StreamRX(&distance, &angle, DW3_DEFAULT_TIMEOUT);
  if (ret == DW3_OK)
  {
    Serial.println("Distance = \t" + String(distance) + " m" + "\tAngle = \t" + String(angle) + " Degrees");
  }
  else if (ret == DW3_TIMEDOUT)
  {
    // Serial.println("RX Timed Out");
  }
  else if (ret == DW3_ERROR)
  {
    Serial.println("RX Error");
  }
  else
  {
    Serial.println("RX Unknown Error");
  }

  #else
  
  DW3_StatusTypeDef ret = DW3_StreamTX(DW3_DEFAULT_TIMEOUT);
  if (ret == DW3_OK)
  {
    Serial.println("TX Success");
  }
  else if (ret == DW3_ERROR)
  {
    Serial.println("TX Error");
  }
  else if (ret == DW3_TIMEDOUT)
  {
    Serial.println("TX Timed Out");
  }
  else
  {
    Serial.println("TX Unknown Error");
  }

  #endif

}

extern void SystemClock_Config(void);
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 288;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 6;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);

}
