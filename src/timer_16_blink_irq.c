// 1 сек
#include "mik32_hal.h"
#include "mik32_hal_irq.h"
#include "mik32_hal_timer16.h"

#include "uart_lib.h"
#include "xprintf.h"

Timer16_HandleTypeDef htimer16_1;

void SystemClock_Config(void);
static void Timer16_1_Init(void);
void GPIO_Init();

volatile bool is_time_switch = false;

/*
 * Пример для платы BOARD_LITE
 * В данном примере демонстрируется работа прерываний TIMER16_1.
 * Таймер запускается и генерирует на выводе PORT0_10 ШИМ сигнал.
 * Выводы GPIO0_3 и GPIO1_3 меняют свое состояние по достижению счетчиком
 * CNT предельного значения ARR и значения сравнения CMP соответственно.
 *
 * */

int main() {
  HAL_Init();

  SystemClock_Config();

  GPIO_Init();

  Timer16_1_Init();

  /* Включать прерывания Timer16 рекомендуется после его инициализации */
  HAL_EPIC_MaskLevelSet(HAL_EPIC_TIMER16_1_MASK);
  HAL_IRQ_EnableInterrupts();

  HAL_Timer16_Counter_Start_IT(&htimer16_1, 2500); // 10 мс

  while (1) {

    if (is_time_switch) { // раз в 1 секунду
      is_time_switch = false;
      HAL_GPIO_TogglePin(GPIO_0, GPIO_PIN_3);
    }
  }
}

void SystemClock_Config(void) {
  PCC_InitTypeDef PCC_OscInit = {0};

  PCC_OscInit.OscillatorEnable = PCC_OSCILLATORTYPE_ALL;
  PCC_OscInit.FreqMon.OscillatorSystem = PCC_OSCILLATORTYPE_OSC32M;
  PCC_OscInit.FreqMon.ForceOscSys = PCC_FORCE_OSC_SYS_UNFIXED;
  PCC_OscInit.FreqMon.Force32KClk = PCC_FREQ_MONITOR_SOURCE_OSC32K;
  PCC_OscInit.AHBDivider = 0;
  PCC_OscInit.APBMDivider = 0;
  PCC_OscInit.APBPDivider = 0;
  PCC_OscInit.HSI32MCalibrationValue = 128;
  PCC_OscInit.LSI32KCalibrationValue = 8;
  PCC_OscInit.RTCClockSelection = PCC_RTC_CLOCK_SOURCE_AUTO;
  PCC_OscInit.RTCClockCPUSelection = PCC_CPU_RTC_CLOCK_SOURCE_OSC32K;
  HAL_PCC_Config(&PCC_OscInit);
}

static void Timer16_1_Init(void) {
  htimer16_1.Instance = TIMER16_1;

  /* Настройка тактирования */
  htimer16_1.Clock.Source = TIMER16_SOURCE_INTERNAL_SYSTEM;
  htimer16_1.CountMode = TIMER16_COUNTMODE_INTERNAL;
  htimer16_1.Clock.Prescaler = TIMER16_PRESCALER_128;

  /* Настройка режима обновления регистра ARR и CMP */
  htimer16_1.Preload = TIMER16_PRELOAD_AFTERWRITE;

  /* Настройки фильтра */
  htimer16_1.Filter.ExternalClock = TIMER16_FILTER_NONE;
  htimer16_1.Filter.Trigger = TIMER16_FILTER_NONE;

  /* Настройка режима энкодера */
  htimer16_1.EncoderMode = TIMER16_ENCODER_DISABLE;

  HAL_Timer16_Init(&htimer16_1);
}

void GPIO_Init() {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_PCC_GPIO_0_CLK_ENABLE();
  __HAL_PCC_GPIO_1_CLK_ENABLE();
  __HAL_PCC_GPIO_2_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = HAL_GPIO_MODE_GPIO_OUTPUT;
  GPIO_InitStruct.Pull = HAL_GPIO_PULL_NONE;
  HAL_GPIO_Init(GPIO_0, &GPIO_InitStruct);
  HAL_GPIO_Init(GPIO_1, &GPIO_InitStruct);
}

volatile uint32_t ms = 0;

void trap_handler() {
  if (EPIC_CHECK_TIMER16_1()) {

    if (__HAL_TIMER16_GET_FLAG_IT(&htimer16_1, TIMER16_FLAG_ARRM)) {
      __HAL_TIMER16_CLEAR_FLAG(&htimer16_1, TIMER16_FLAG_ARRM);

      ms = ms + 10;
      if (ms >= 1000) {
        is_time_switch = true;
        ms = 0;
      }
    }
  }

  /* Сброс прерываний */
  HAL_EPIC_Clear(0xFFFFFFFF);
}