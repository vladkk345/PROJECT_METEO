#include "stm32f0xx.h"
#include "clock.h"

// Функция настройки тактирования 

void Set_Clock(void)
{
	RCC->CR |= ((uint32_t)RCC_CR_HSEON);// Включить генератор HSE
  while (!(RCC->CR & RCC_CR_HSERDY)) // Ждать готовность HSE
		
	//RCC->CSR |= ((uint32_t)RCC_CSR_LSION); // Включить LSI
	//while (!(RCC->CSR & RCC_CSR_LSIRDY)) // Ждать готовность LSI
		
	// Настройка частоты работы flash - памяти
	FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY;
	
	/* HCLK = SYSCLK / 1 */
	RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;
	/* PCLK = HCLK / 1 */
	RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE_DIV1;
	
	/* PLL configuration */
	// Сброс битов регистра CFGR
	RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMUL));
	// Тактирование от HSE	
	RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLSRC_HSE_PREDIV | RCC_CFGR_PLLXTPRE_HSE_PREDIV_DIV2| RCC_CFGR_PLLMUL6);
	// Сброс регистра CFGR2
	//RCC->CFGR2 &= (uint32_t)((uint32_t)~(RCC_CFGR2_PREDIV_1));
	// Тактирование от HSE (выбор предделителя)
	//RCC->CFGR2 |= (uint32_t)(RCC_CFGR2_PREDIV_DIV2); 
	RCC->CR |= RCC_CR_PLLON; // Запустить PLL	
	
	while((RCC->CR & RCC_CR_PLLRDY) == 0){} // Ожидание готовности PLL
	/* Select PLL as system clock source */
	RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW)); //Очистить биты SW0 и SW1	
		
	RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL; // Тактирование с выхода PLL
	/* Ожидание переключения на PLL */
	while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)RCC_CFGR_SWS_PLL) {}
		
	RCC->CR |= ((uint32_t)RCC_CR_CSSON);  // Включение CSS

	// Настройка вывода MCO
	//RCC->CFGR &= (uint32_t)((uint32_t) ~(RCC_CFGR_MCO)); // Сброс бита MCO
	//RCC->CFGR |= (uint32_t)(RCC_CFGR_MCOSEL_SYSCLK);		
}
