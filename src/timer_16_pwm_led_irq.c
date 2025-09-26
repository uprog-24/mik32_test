// 1 сек
#include "mik32_hal.h"
#include "mik32_hal_irq.h"
#include "mik32_hal_timer16.h"
#include "mik32_hal_timer32.h"

#include "uart_lib.h"
#include "xprintf.h"

TIMER32_HandleTypeDef htimer32_1;
TIMER32_CHANNEL_HandleTypeDef htimer32_channel3;

Timer16_HandleTypeDef htimer16_1;

void SystemClock_Config(void);
static void Timer32_1_Init(void);
static void Timer16_1_Init(void);
void GPIO_Init();

volatile bool is_time_switch = false;

volatile uint16_t pulse = 0;
volatile int8_t step = 1;
volatile bool update_pwm = false;

int main() {
  HAL_Init();

  SystemClock_Config();

  GPIO_Init();

  Timer32_1_Init();
  Timer16_1_Init();

  HAL_Timer32_Channel_Enable(&htimer32_channel3);
  HAL_Timer32_Value_Clear(&htimer32_1);
  HAL_Timer32_Start(&htimer32_1);

  /* Включать прерывания Timer16 */
  HAL_EPIC_MaskLevelSet(HAL_EPIC_TIMER16_1_MASK);
  HAL_IRQ_EnableInterrupts();
  HAL_Timer16_Counter_Start_IT(&htimer16_1,
                               2500); // 10 мс

  uint16_t pulse;
  uint16_t max_pulse = htimer32_1.Top / 2; // максимальное значение CMP

  while (1) {

    if (update_pwm) {
      update_pwm = false;

      // Плавное увеличение/уменьшение
      pulse += step;
      if (pulse >= max_pulse) {
        step = -1;
      } else if (pulse == 0) {
        step = 1;
      }

      // При достижении предельных значений (max_pulse * 10 мс = 5 сек)
      // меняем состояние GPIO1_3
      if ((pulse == max_pulse) || (pulse == 0)) {
        HAL_GPIO_TogglePin(GPIO_1, GPIO_PIN_3);
      }

      HAL_Timer32_Channel_OCR_Set(&htimer32_channel3, pulse);
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

static void Timer32_1_Init(void) { // 1 kHz
  // Настройка таймера
  htimer32_1.Instance = TIMER32_1;
  htimer32_1.Top = 1000; // TOP = период ШИМ
  htimer32_1.State = TIMER32_STATE_DISABLE;
  htimer32_1.Clock.Source = TIMER32_SOURCE_PRESCALER;
  htimer32_1.Clock.Prescaler = 31;
  htimer32_1.InterruptMask = 0;
  htimer32_1.CountMode = TIMER32_COUNTMODE_FORWARD;
  HAL_Timer32_Init(&htimer32_1);

  // Настройка канала 4 для ШИМ
  htimer32_channel3.TimerInstance = htimer32_1.Instance;
  htimer32_channel3.ChannelIndex = TIMER32_CHANNEL_3;
  htimer32_channel3.Mode = TIMER32_CHANNEL_MODE_PWM;
  htimer32_channel3.PWM_Invert = TIMER32_CHANNEL_NON_INVERTED_PWM;
  htimer32_channel3.CaptureEdge = TIMER32_CHANNEL_CAPTUREEDGE_RISING;
  htimer32_channel3.OCR = 0; // стартовая скважность 0 %
  htimer32_channel3.Noise = TIMER32_CHANNEL_FILTER_OFF;

  HAL_Timer32_Channel_Init(&htimer32_channel3);
}

static void Timer16_1_Init(void) {
  htimer16_1.Instance = TIMER16_1;
  htimer16_1.Clock.Source = TIMER16_SOURCE_INTERNAL_SYSTEM;
  htimer16_1.CountMode = TIMER16_COUNTMODE_INTERNAL;
  htimer16_1.Clock.Prescaler = TIMER16_PRESCALER_128;
  htimer16_1.Preload = TIMER16_PRELOAD_AFTERWRITE;
  htimer16_1.Filter.ExternalClock = TIMER16_FILTER_NONE;
  htimer16_1.Filter.Trigger = TIMER16_FILTER_NONE;
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
  HAL_GPIO_Init(GPIO_1, &GPIO_InitStruct);
}

void trap_handler(void) {
  if (EPIC_CHECK_TIMER16_1()) {
    if (__HAL_TIMER16_GET_FLAG_IT(&htimer16_1, TIMER16_FLAG_ARRM)) {
      __HAL_TIMER16_CLEAR_FLAG(&htimer16_1, TIMER16_FLAG_ARRM);

      update_pwm = true;
    }
  }
  HAL_EPIC_Clear(0xFFFFFFFF);
}