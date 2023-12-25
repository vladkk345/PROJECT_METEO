#include "stm32f0xx.h"
#include "bmp280.h"
#include "delay.h"
//#include "stdbool.h"

//uint32_t timeout_ms = 0;
long signed int t_fine; // Параметр для пересчета давления и температуры
// Массив регистров настроек
uint8_t addr[3] = {0xE0, 0x74, 0x75};
// Массив данных о настройках 
uint8_t data[3] = {0xB6, 0x27, 0xA0};
// 0xB6 - Полная процедура сброса датчика
// 0x27 - Нормальный режим работы, передискретизация температуры и давления * 1
// 0xA0 - Фильтрация отключена, 3-проводной SPI отключен, время между пакетами данных - 1000 ms

//-------------------------Инициализация шины SPI-------------------------------------------------------------

void SPI_init_BMP(void)
{
	RCC->APB2ENR |= (1 << 12); 																		// Тактирование шины SPI
	
	SPI1->CR1 |= SPI_CR1_MSTR; 																		// Режим ведущего
	SPI1->CR1 |= SPI_CR1_BR_1; 																		// Скорость 3 МГц
	SPI1->CR1 &= ~SPI_CR1_LSBFIRST; 															// Начало с MSB
	SPI1->CR1 &= ~SPI_CR1_CPOL; 																	// Синхросигнал с 0 
	SPI1->CR1 &= ~SPI_CR1_CPHA; 																	// Захват по первому фронту
	//SPI1->CR1 &= ~SPI_CR1_SSM; 																	// Программное управление выключено
	//SPI1->CR2 |= SPI_CR2_SSOE; 																	// Включение аппаратного NSS
	SPI1->CR1 |= SPI_CR1_SSI | SPI_CR1_SSM; 											// Программное управление NSS
	SPI1->CR1 &= ~SPI_CR1_BIDIMODE; 															// Режим full-duplex
	//SPI1->CR1 |= SPI_CR1_RXONLY;
	SPI1->CR1 &= ~SPI_CR1_RXONLY; 																// Прием и передача
	SPI1->CR2 |= SPI_CR2_DS_0 | SPI_CR2_DS_1 | SPI_CR2_DS_2; 			// Размер данных 8 бит
	SPI1->CR2 |= SPI_CR2_FRXTH;
	
	SPI1->I2SCFGR &= ~SPI_I2SCFGR_I2SMOD;													// Выбираем работу по SPI
	SPI1->CR2 = 0;																								// Очистка регистра CR2
	
	SPI1->CR1 |= SPI_CR1_SPE; 																		// Включение шины SPI
}

//-------------------------Инициализация датчика BMP280------------------------------------------------------

void BMP_init(void)
{
	// Опускаем CS
	CS_OFF;
	SPI_Send(&addr[2]); // Отправка адреса датчика 0x75 (на запись)
	SPI_Send(&data[2]); // Отправка данных в первый регистр
	// Поднимаем CS
	CS_ON;
	
	// Опускаем CS
	CS_OFF;
	SPI_Send(&addr[1]); // Отправка адреса датчика 0x74(на запись)
	SPI_Send(&data[1]); // Отправка данных во второй регистр
	// Поднимаем CS
	CS_ON;
	
	// Опускаем CS
	CS_OFF;
	SPI_Send(&addr[0]); // Отправка адреса датчика 0xE0(на запись)
	SPI_Send(&data[0]); // Отправка данных в третий регистр
	// Поднимаем CS
	CS_ON;
}

//-----------------------------------Функция вычисления температуры----------------------------------------------------

double Temp_calculation(long signed int adc_T)
{
	double var1, var2, T; // Переменные для пересчета данных
	
	// Параметры компенсации
	unsigned short dig_T1 = 28233;
	short dig_T2 = 26716; 
	short dig_T3 = 64536; 
	
	var1 = (((double)adc_T)/16384.0 - ((double)dig_T1)/1024.0) * ((double)dig_T2);
	var2 = ((((double)adc_T)/131072.0 - ((double)dig_T1)/8192.0) *
	(((double)adc_T)/131072.0 - ((double) dig_T1)/8192.0)) * ((double)dig_T3);
	t_fine = (long signed int)(var1 + var2);
	T = (var1 + var2) / 5120.0; // Итоговое значение температуры в градусах Цельсия
	
	// Возвращаем температуру
	return T;
}

//--------------------------------------Функция вычисления давления----------------------------------------------------

double BMP_calculation(long signed int adc_P)
{
	double var1, var2, p; // Переменные для пересчета данных
	
	// Параметры компенсации
	unsigned short dig_P1 = 37115;
	short dig_P2 = 54888; 
	short dig_P3 = 3024; 
	short dig_P4 = 2134; 
	short dig_P5 = 247; 
	short dig_P6 = 65529; 
	short dig_P7 = 15500; 
	short dig_P8 = 50936; 
	short dig_P9 = 6000;
	
	var1 = ((double)t_fine/2.0) - 64000.0;
	var2 = var1 * var1 * ((double)dig_P6)/32768.0;
	var2 = var2 + var1 * ((double)dig_P5) * 2.0;
	var2 = (var2/4.0) + (((double)dig_P4) * 65536.0);
	var1 = (((double)dig_P3) * var1 * var1/524288.0 + ((double)dig_P2) * var1)/524288.0;
	var1 = (1.0 + var1/32768.0)*((double)dig_P1);
	
	if (var1 == 0.0)
	{
		return 0; // Избегаем деления на 0
	}
	
	p = 1048576.0 - (double)adc_P;
	p = (p - (var2/4096.0)) * 6250.0/var1;
	var1 = ((double)dig_P9) * p * p /2147483648.0;
	var2 = p * ((double)dig_P8)/ 32768.0;
	p = (p + (var1 + var2 + ((double)dig_P7))/16.0) / 1000; // Итоговое значение давления в кПа
	
	// Возвращаем давление
	return p;
}

//bool Send_data_8bit(SPI_TypeDef* SPI, uint8_t* data, uint16_t size_data, uint32_t timeout)
//{
//	//Ждем, пока не освободится линия передачи SPI
//	if((!SPI1->SR & SPI_SR_BSY))
//	{
//		SPI1->DR = *(data);
//		for(uint16_t i = 1; i < size_data; i++)
//		{
//			timeout_ms = timeout;
//			//Ждем, пока не освободится буфер передатчика
//			while(!((SPI1->SR) & (SPI_SR_TXE)))
//			{
//				if(!timeout_ms)
//				{
//					return 0;
//				}
//			}
//			SPI1->DR = *(data + i);
//		}
//		timeout_ms = timeout;
//		while(!((SPI1->SR) & (SPI_SR_TXE)))
//		{
//				if(!timeout_ms)
//				{
//					return 0;
//				}
//		}
//		timeout_ms = timeout;
//		while(((SPI1->SR) & (SPI_SR_BSY)))
//		{
//				if(!timeout_ms)
//				{
//					return 0;
//				}
//		}
//		
//		return 1;
//		
//	}
//	else
//	{
//		return 0;
//	}
//	
//}

//------------------------------------Функция отправки данных---------------------------------------------------

void SPI_Send(uint8_t *tx_data)
{
	Delay_us(10);

	// Ждем, пока не освободится буфер передатчика
	while(!(SPI1->SR) & (SPI_SR_TXE)) {};
		
	// Заполняем буфер передатчика
	*(__IO uint8_t*)&SPI1->DR = *tx_data;

	Delay_us(10);
	
	// Ждем установки флагов занятости и передающего буфера	
	while(!(SPI1->SR) & (SPI_SR_TXE)) {};
	while((SPI1->SR) & (SPI_SR_BSY)) {};
	
	// Очищаем регистры для избежания переполнения
	uint8_t temp = *(__IO uint8_t*)&SPI1->SR;
					temp = *(__IO uint8_t*)&SPI1->DR;
		
	Delay_us(10);
}

//------------------------------------Функция приема данных---------------------------------------------------

uint8_t SPI_Receive(uint8_t *tx_data, uint8_t *rx_data)
{
	Delay_us(10);
	
	// Ждем, пока не освободится буфер передатчика
	while(!(SPI1->SR) & (SPI_SR_TXE)) {};
		
	// Заполняем буфер передатчика
	*(__IO uint8_t*)&SPI1->DR = *tx_data;

	Delay_us(10);
	
	// Ждем установки флагов занятости и передающего буфера	
	while(!(SPI1->SR) & (SPI_SR_TXE)) {};
	while((SPI1->SR) & (SPI_SR_BSY)) {};
	
	// Отправляем фиктивный байт для принятия ответа от устройства
	*(__IO uint8_t*)&SPI1->DR = 0; 
		
	// Ждем установки флага приемного буфера	
	while(!(SPI1->SR) & (SPI_SR_RXNE)) {};
		
	// Читаем принятые из регистра DR данные
	*rx_data = *(__IO uint8_t*)&SPI1->DR;
				
  Delay_us(10);
		
	// Возвращаем прочитанные данные
  return *rx_data;
}

//------------------------------------Функция чтения принятых данных---------------------------------------------------

uint8_t SPI_Reading(uint8_t* tx_data)
{
	uint8_t receive = 0; // Ответ от ведомого устройства
	uint8_t rx_data[1];
	
	// Опускаем CS
	CS_OFF;
	SPI_Send(tx_data); // Отправка адреса датчика (на чтение)
	// Поднимаем CS
	CS_ON;
	
	// Опускаем CS
	CS_OFF;
	receive = SPI_Receive(tx_data, rx_data); // Получение ответных данных от датчика;
	// Поднимаем CS
	CS_ON;
	
	// Возвращаем принятые данные
	return receive;
}
