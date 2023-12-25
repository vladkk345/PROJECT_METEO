#include "stm32f0xx.h"
#include "dht11.h"
#include "delay.h"

extern float Humidity;
extern float Temperature;

//------------------------------------Функция настройки вывода на выход---------------------------------------------------

static void goToOutput(DHT_sensor *sensor) 
{
  // По умолчанию на линии высокий уровень
  lineUp();
	
  // Настройка вывода (PA3)
	GPIOA->MODER |= (1 << 6); 			// Установка вывода PA3 как выход
	GPIOA->OSPEEDR |= (1 << 6);   	// Установка вывода со скоростью работы Medium
	GPIOA->PUPDR |= (1 << 6); 			// Установка подтяжки вывода к питанию
	GPIOA->OTYPER |= (1 << 3);    	// Установка вывода в режим open-drain
}

//------------------------------------Функция настройки вывода на вход---------------------------------------------------

static void goToInput(DHT_sensor *sensor) 
{
	// Настройка вывода (PA3)
	GPIOA->MODER &= ~(1 << 6); 			// Установка вывода PA3 как вход
	GPIOA->PUPDR |= (1 << 6); 			// Установка подтяжки к питанию
}

//------------------------------------Функция инициализации и получения данных с датчика---------------------------------------------------

DHT_data DHT_getData(DHT_sensor *sensor) 
{
	DHT_data data = {Humiduty, Temperature}; // Выдаваемые датчиком значения
	
	#if DHT_POLLING_CONTROL == 1
	/* Ограничение по частоте опроса датчика */
	// Определение интервала опроса в зависимости от датчика
	uint16_t pollingInterval;
	
	pollingInterval = DHT_POLLING_INTERVAL_DHT11;
	 
	#endif

	/* Запрос данных у датчика */
	// Перевод пина "на выход"
	goToOutput(sensor);
	
	// Опускание линии данных на 18 мс
	lineDown();
	Delay_ms(18);
	
	// Подъём линии, перевод порта "на вход"
	lineUp();
	goToInput(sensor);

	#ifdef DHT_IRQ_CONTROL
	// Выключение прерываний, чтобы ничто не мешало обработке данных
	__disable_irq();
	#endif
	
	/* Ожидание ответа от датчика */
	uint16_t timeout = 0;
	
	// Ожидание спада сигнала на выводе
	while((GPIOA->IDR & GPIO_IDR_3)) 
	{
		timeout++;
		if (timeout > DHT_TIMEOUT) 
		{
			#ifdef DHT_IRQ_CONTROL
			__enable_irq();
			#endif
			// Если датчик не отозвался, значит его точно нет
			// Обнуление последнего удачного значения, чтобы
			// не получать фантомные значения
			sensor->lastHum = -128.0f;
			sensor->lastTemp = -128.0f;

			return data;
		}
	}
	timeout = 0;
	
	// Ожидание подъёма сигнала на выводе
	while(!(GPIOA->IDR & GPIO_IDR_3)) 
	{
		timeout++;
		if (timeout > DHT_TIMEOUT) 
		{
			#ifdef DHT_IRQ_CONTROL
			__enable_irq();
			#endif
			// Если датчик не отозвался, значит его точно нет
			// Обнуление последнего удачного значения, чтобы
			// не получать фантомные значения
			sensor->lastHum = -128.0f;
			sensor->lastTemp = -128.0f;

			return data;
		}
	}
	timeout = 0;
	
	// Ожидание спада сигнала на выводе
	while((GPIOA->IDR & GPIO_IDR_3)) 
	{
		timeout++;
		if (timeout > DHT_TIMEOUT) 
		{
			#ifdef DHT_IRQ_CONTROL
			__enable_irq();
			#endif
			
			return data;
		}
	}
	
	/* Чтение ответа от датчика */
	uint8_t rawData[5] = {0,0,0,0,0};
	
	for(uint8_t a = 0; a < 5; a++) 
	{
		for(uint8_t b = 7; b != 255; b--) 
		{
			uint16_t hT = 0, lT = 0;
			// Пока линия в низком уровне, инкремент переменной lT
			while(!(GPIOA->IDR & GPIO_IDR_3) && lT != 65535) lT++;
			
			// Пока линия в высоком уровне, инкремент переменной hT
			timeout = 0;
			while((GPIOA->IDR & GPIO_IDR_3) && hT != 65535) hT++;
			
			// Если hT больше lT, то пришла единица
			if(hT > lT) rawData[a] |= (1<<b);
		}
	}

    #ifdef DHT_IRQ_CONTROL
	
	// Включение прерываний после приёма данных
	__enable_irq();
    #endif

	/* Проверка целостности данных */
	if((uint8_t)(rawData[0] + rawData[1] + rawData[2] + rawData[3]) == rawData[4]) 
	{
		data.hum = (float)rawData[0];
		data.temp = (float)rawData[2];	
	}
	
	#if DHT_POLLING_CONTROL == 1
	sensor->lastHum = data.hum;
	sensor->lastTemp = data.temp;
	#endif

	return data;	
}
