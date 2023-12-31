#include "stm32f0xx.h"
#include "delay.h"

//--------------------------------Инициализация таймера для реализации мкс и мс задежки-------------------------

void TIM6_init(void)
{
	RCC->APB1ENR |= (1 << 4); 	// Тактирование таймера 6
	
	//TIM6->DIER |= (1 << 0); 		// Разрешаем прерывания по таймеру 6
	//NVIC_EnableIRQ(TIM6_IRQn);	// Включаем прерывания по таймеру 6
	TIM6->PSC = 24 - 1; 			// Значение предделителя
	TIM6->ARR = 65535-1;						// Значение автоперезагрузки
	TIM6->CR1 |= (1 << 0);
	while (!(TIM6->SR & (1 << 0))); // Сброс флага
}

//------------------------------------ Функция задержки в мс----------------------------------------------------

void Delay_ms(uint16_t ms)
{
	for(uint16_t i = 0; i < ms;i++)
	{
		Delay_us(1000);
	}
}

//------------------------------------ Функция задержки в мкс---------------------------------------------------

void Delay_us(uint16_t us)
{
	TIM6->CNT = 0;
	while(TIM6->CNT < us);
}
