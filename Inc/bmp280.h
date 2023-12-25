#ifndef BMP280_H
#define BMP280_H

//#include "stm32f0xx.h"
#define CS_OFF   			GPIOA->BSRR |= ((1 << 4) << 16)
#define CS_ON					GPIOA->BSRR |= (1 << 4)
#define OE_OFF				GPIOA->BSRR |= ((1 << 3) << 16)

//bool Send_data_8bit(SPI_TypeDef* SPI, uint8_t* data, uint16_t size_data, uint32_t timeout);
uint8_t SPI_Reading(uint8_t* tx_data);
void SPI_Send(uint8_t *tx_data);
uint8_t SPI_Receive(uint8_t *tx_data, uint8_t *rx_data);
void SPI_init_BMP(void);
void BMP_init(void);
double BMP_calculation(long signed int adc_P);
double Temp_calculation(long signed int adc_T);

#endif
