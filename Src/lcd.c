#include "stm32f0xx.h"
#include "lcd.h"
#include <string.h>
#include "delay.h"

// Границы кэша в местах, где произошли изменения. 
// и копирование этой части кэша между границами в ОЗУ.
static int LoWaterMark; 												// Нижняя граница
static int HiWaterMark; 												// Верхняя граница
static unsigned char LcdCache[LCD_CACHE_SIZE];  // Кэш в ОЗУ 504 байта
static int LcdCacheIdx; 												// Указатель для работы с LcdCache[]
static unsigned char UpdateLcd; 								// Флаг изменений кэша

//-----------------------------Инициализация шины SPI-------------------------------------------------------------

void SPI_init_LCD(void)
{
	RCC->APB1ENR |= (1 << 14); 															// Тактирование 2 канала шины SPI 
	
	SPI2->CR1 |= SPI_CR1_MSTR; 															// Режим ведущего
	SPI2->CR1 |= SPI_CR1_BR_1; 															// Скорость 3 МГц
	SPI2->CR1 &= ~SPI_CR1_LSBFIRST; 												// Начало с MSB
	SPI2->CR1 &= ~SPI_CR1_CPOL; 														// Синхросигнал с 0 
	SPI2->CR1 |= SPI_CR1_CPHA; 															// Захват по первому фронту													
	SPI2->CR1 |= SPI_CR1_SSI | SPI_CR1_SSM; 								// Программное управление NSS
	SPI2->CR1 |= SPI_CR1_BIDIMODE; 													// Двунаправленный обмен данными
	SPI2->CR1 |= SPI_CR1_BIDIOE; 														// Только передача
	SPI2->CR1 &= ~SPI_CR1_RXONLY; 													// Отключение режима только прием
	SPI2->CR2 |= SPI_CR2_DS_0 | SPI_CR2_DS_1 |SPI_CR2_DS_2; // Размер данных 8 бит
	SPI2->CR1 |= SPI_CR1_SPE; 															// Включение шины SPI
}

//----------------------------Функция отправки данных по SPI--------------------------------------------

void SPI_send(uint8_t Word)
{
	LATCH_OFF; // Опускаем Chip Select
	
	// Ждем, пока не освободится буфер передатчика
	while ((SPI2->SR & SPI_SR_TXE) != SPI_SR_TXE);
			
	// Заполняем буфер передатчика
	*(uint8_t*)&SPI2->DR = Word;
	
	// Ждем, пока не освободится линия передачи (занятости шины)
	while ((SPI2->SR & SPI_SR_BSY) == SPI_SR_BSY);

	Delay_us(1);  // Задержка, чтобы все данные успели передаться
	LATCH_ON; 		// Поднимаем Chip Select
	Delay_us(10); // Удержание CS в высоком уровне
}

//----------------------------Функция обновления данных (отправка в дисплей)--------------------------------------------

void Lcd_update(void)
{
  int i;

  if (LoWaterMark < 0)
	{
		LoWaterMark = 0;
	}
	
  else if (LoWaterMark >= LCD_CACHE_SIZE)
	{
    LoWaterMark = LCD_CACHE_SIZE - 1;
	}

  if (HiWaterMark < 0)
	{
    HiWaterMark = 0;
	}
	
  else if (HiWaterMark >= LCD_CACHE_SIZE)
	{
    HiWaterMark = LCD_CACHE_SIZE - 1;
	}

	// Алгоритм для оригинального дисплея
	
	GPIOA->BSRR |= ((1 << 1) << 16); 	// Установка сигнала DC в 0
	
  // Установка начального адреса в соответствии с LoWaterMark
  SPI_send(0x80 | (LoWaterMark % LCD_X_RES));
  SPI_send(0x40 | (LoWaterMark / LCD_X_RES));
		
	GPIOA->BSRR |= (1 << 1); 	// Установка сигнала DC в 1
		
  // Обновление необходимой части буфера дисплея
  for (i = LoWaterMark; i <= HiWaterMark; i++)
  {
    // Последовательный вывод данных
					
    SPI_send(LcdCache[i]); // Отправка кэша по SPI
  }

  // Сброс указателей границ	
  LoWaterMark = LCD_CACHE_SIZE - 1;
  HiWaterMark = 0;

  // Сброс флага изменений кэша
  UpdateLcd = FALSE;
}

//----------------------------Функция рисования символа--------------------------------------------

int LcdChr(FontSize size, unsigned char ch)
{
  unsigned char i, c;
  unsigned char b1, b2;
  int tmpIdx;

  if (LcdCacheIdx < LoWaterMark)
  {
    // Обновление нижней границы
    LoWaterMark = LcdCacheIdx;
  }

  if ((ch >= 0x20) && (ch <= 0x7F))
  {
		// Смещение в таблице для латинских символов в ASCII
    ch -= 32;
  }
	
  else if (ch >= 0xC0)
  {			
		// Смещение в таблице для русских символов в ASCII
		ch -= 96;						     
  }
	
  else
  {
    // Смещение для отсутствующих символов
    ch = 95;
  }

  if (size == FONT_1X)
  {
		for (i = 0; i < 5; i++)
		{
      // Копирование вида символа из таблицы в кэш
      LcdCache[LcdCacheIdx++] = ((ASCII[ch][i]));
    }
  }
	
  else if (size == FONT_2X)
  {
     tmpIdx = LcdCacheIdx;

     if (tmpIdx < LoWaterMark)
     {
       LoWaterMark = tmpIdx;
     }

     if (tmpIdx < 0) 
		 {
			 return OUT_OF_BORDER;
		 }

     for (i = 0; i < 5; i++)
     {
			 // Копирование вида символа из таблицы во временную переменную
       c = (ASCII[ch][i]);
				
			 // Увеличение первой части картинки
       b1 =  (c & 0x01) * 3;
       b1 |= (c & 0x02) * 6;
       b1 |= (c & 0x04) * 12;
       b1 |= (c & 0x08) * 24;

       c >>= 4;
			 
       // Увеличение второй части
       b2 =  (c & 0x01) * 3;
       b2 |= (c & 0x02) * 6;
       b2 |= (c & 0x04) * 12;
       b2 |= (c & 0x08) * 24;

       // Копирование двух частей в кэш
       LcdCache[tmpIdx++] = b1;
       LcdCache[tmpIdx++] = b1;
       LcdCache[tmpIdx + 82] = b2;
       LcdCache[tmpIdx + 83] = b2;
     }

     // Обновление x координаты курсора
     LcdCacheIdx = (LcdCacheIdx + 11) % LCD_CACHE_SIZE;
  }

  if (LcdCacheIdx > HiWaterMark)
  {
    // Обновление верхней границы
    HiWaterMark = LcdCacheIdx;
  }

  // Горизонтальный разрыв между символами
  LcdCache[LcdCacheIdx] = 0x00;
		
  // При достижении позиции указателя LCD_CACHE_SIZE - 1, переход в начало
  if (LcdCacheIdx == (LCD_CACHE_SIZE - 1))
  {
    LcdCacheIdx = 0;
    return OK_WITH_WRAP;
  }
		
  // Инкремент указателя
  LcdCacheIdx++;
		
  return OK;
}

//----------------------------Функция установки курсора в позицию x,y--------------------------------------------------------------

unsigned char LcdGotoXY(unsigned char x, unsigned char y)
{
  // Проверка границ
  if (x > 13 || y > 5) 
	{
		return OUT_OF_BORDER;
	}

  // Вычисление указателя. Определен как адрес в пределах 504 байт
  LcdCacheIdx = x * 6 + y * 84;
		
  return OK;
}

//----------------------------Функция вывода строк------------------------------------------------------------------------------

unsigned char lcd_print(unsigned char x, unsigned char y, FontSize size, unsigned char text[])
{
	LcdGotoXY(x,y);
	
	int tmpIdx = 0;
	int response;
	
	while(text[tmpIdx] != '\0')
	{
		// Вывод символа
    response = LcdChr(size, text[tmpIdx]);
		
    // Печать строки дальше из начала дисплея
    if(response == OUT_OF_BORDER)
		{
      return OUT_OF_BORDER;
		}
		
    // Увеличение указателя
    tmpIdx++;	
	}
	
	return OK;
}

//----------------------------Функция очистки дисплея--------------------------------------------

void Lcd_clear(void)
{
	// Заполнение кэша нулевыми значениями
  memset(LcdCache, 0x00, LCD_CACHE_SIZE); 
    
  // Сброс указателей границ в максимальное значение
  LoWaterMark = 0;
  HiWaterMark = LCD_CACHE_SIZE - 1;
	
  // Установка флага изменений кэша
  UpdateLcd = TRUE;
}

//----------------------------Функция инициализации дисплея--------------------------------------------

void Lcd_init(void)
{
	SPI_send(0x21); // Расширенная настройка
	SPI_send(0xD0); // Установка напряжения 
	SPI_send(0x20); // Обычная настройка
	SPI_send(0x0C); // Нормальный режим работы дисплея
}
