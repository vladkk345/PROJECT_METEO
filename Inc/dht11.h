#ifndef DHT11_H
#define DHT11_H

#define DHT_TIMEOUT 										10000	// Количество итераций, после которых функция вернёт пустые значения
#define DHT_POLLING_CONTROL							1		  // Включение проверки частоты опроса датчика
#define DHT_POLLING_INTERVAL_DHT11	 		2000	// Интервал опроса DHT11 (0.5 Гц по даташиту).
#define DHT_IRQ_CONTROL												// Выключать прерывания во время обмена данных с датчиком

/* Структура возвращаемых датчиком данных */

typedef struct {
	int hum;
	int temp;
} DHT_data;

/* Структура объекта датчика */

typedef struct {
	GPIO_TypeDef *DHT_Port;					// Порт МК 
	uint16_t DHT_Pin;								// Номер вывода МК
	uint8_t pullUp;								  // Нужна ли подтяжка линии данных к питанию

	// Контроль частоты опроса датчика. Значения не заполнять!
	
	#if DHT_POLLING_CONTROL == 1
	uint32_t lastPollingTime;				// Время последнего опроса датчика
	float lastTemp;			 						// Последнее значение температуры
	float lastHum;			 						// Последнее значение влажности
	#endif
	
} DHT_sensor;

DHT_data DHT_getData(DHT_sensor *sensor); 									// Получить данные с датчика

#define lineDown() 							GPIOA->BSRR |= ((1 << 1) << 16)  	// Подтяжка пина к земле
#define lineUp()								GPIOA->BSRR |= ((1 << 1))  			  // Подтяжка пина к питанию
#define GPIO_PULLUP    					0x00000001U   										// Включение подтяжки 
#define GPIO_NOPULL    					0x00000000U   										// Отключение подтяжки 
#define GPIO_PIN_3							((uint16_t)0x0008)  							// 3 вывод МК   

#endif
