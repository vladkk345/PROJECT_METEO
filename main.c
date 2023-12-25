#include "stm32f0xx.h"
#include "clock.h"
#include "gpio.h"
#include "bmp280.h"
#include "dht11.h"
#include "stdio.h"
#include "lcd.h"
#include "string.h"
#include "math.h"
#include "delay.h"

// Коэффициенты для расчета точки росы
float a = 17.27;								
float b = 237.7;
long signed int result = 0; 		// Результат измерения давления или температуры
double pressure = 0; 						// Значение давления (в кПа)
double temp = 0;								// Значение температуры (в градусах Цельсия)
double t_dew_point = 0;					// Значение температуры точки росы
float Ta = 0;										// Переменная температуры для расчета точки росы
float H = 0;										// Переменная влажности для расчета точки росы
double func_dp = 0;							// Функция для расчета точки росы
// Адреса регистров датчика BMP280
uint8_t addr_press_msb = 0xF7;
uint8_t addr_press_lsb = 0xF8;
uint8_t addr_press_xlsb = 0xF9;
uint8_t addr_temp_msb = 0xFA;
uint8_t addr_temp_lsb = 0xFB;
uint8_t addr_temp_xlsb = 0xFC;

// Переменные результатов измерений по каждому из регистров датчика BMP280
uint8_t res_press_msb[1] = {0};
uint8_t res_press_lsb[1] = {0};
uint8_t res_press_xlsb[1] = {0};
uint8_t res_temp_msb[1] = {0};
uint8_t res_temp_lsb[1] = {0};
uint8_t res_temp_xlsb[1] = {0};

// Переменны для датчика DHT11
float Temperature = 0; // Переменная для хранения значения температуры в 10-чном виде
float Humidity = 0; // Переменная для хранения значения влажности в 10-чном виде

// Буферы строк для вывода параметров метеостанции на дисплей
char str[30] = "Метео данные";
char hum_msg[30];  
char press_msg[30];
char temp_msg[30];
char tp_msg[30];

int main(void)
{
	Set_Clock(); 							// Настройки тактирования
	GPIO_init(); 							// Инициализация портов ввода-вывода
	SPI_init_BMP();						// Инициализация шины SPI для работы с BMP280
	SPI_init_LCD();						// Инициализация шины SPI для работы с LCD 5110
	TIM6_init();							// Инициализация таймера 6 (для задержки)
	
	// Настройки для дисплея
	
	GPIOA->BSRR |= (1 << 0); 					// Установка сигнала сброса в 1
	Delay_ms(10);
	GPIOA->BSRR |= ((1 << 0) << 16); 	// Установка сигнала сброса в 0
	Delay_ms(10);
	GPIOA->BSRR |= (1 << 0); 					// Установка сигнала сброса в 1
	
	GPIOA->BSRR |= ((1 << 1) << 16); 	// Установка сигнала DC в 0 (Подстветка выключена)
	GPIOA->BSRR |= (1 << 2); 					// Установка сигнала BL в 1 (Подсветка включена)

	Lcd_init();												// Инициализация дисплея NOKIA 5110
	
	Delay_ms(3000);
	BMP_init();							  				// Инициализация датчика BMP280
	
	static DHT_sensor livingRoom = {GPIOA, GPIO_PIN_3, GPIO_PULLUP}; // Инициализация вывода датчика влажности
	
	while(1)
	{
//--------------------------------------Реализация работы датчика BMP280----------------------------------------
		
	result = 0; // Обнуляем результат для следующего измерения
		
	// MSB часть данных измеренного давления
	SPI_Reading(&addr_press_msb); 										// Выдача данных в регистр буфера DR
	SPI_Reading(&addr_press_msb);											// Еще раз выдача данных и чтение данных из регистра буфера DR
	res_press_msb[0] = SPI_Reading(&addr_press_msb);  // Присвоение полученных данных
		
	// LSB часть данных измеренного давления (Чтение аналогично MSB)
	SPI_Reading(&addr_press_lsb);									
	SPI_Reading(&addr_press_lsb);			
	res_press_lsb[0] = SPI_Reading(&addr_press_lsb);
		
	// XLSB часть данных измеренного давления - нулевая (Чтение аналогично MSB и LSB)	
	SPI_Reading(&addr_press_xlsb);
	SPI_Reading(&addr_press_xlsb);
	res_press_xlsb[0] = SPI_Reading(&addr_press_xlsb);
		
	// Чтение данных о температуре (аналогично давлению)
	// MSB часть данных измеренной температуры	
	SPI_Reading(&addr_temp_msb);
	SPI_Reading(&addr_temp_msb);
	res_temp_msb[0] = SPI_Reading(&addr_temp_msb);
		
	// LSB часть данных измеренной температуры	
	SPI_Reading(&addr_temp_lsb);
	SPI_Reading(&addr_temp_lsb);
	res_temp_lsb[0] = SPI_Reading(&addr_temp_lsb);
	
	// XLSB часть данных измеренной температуры - нулевая
	SPI_Reading(&addr_temp_xlsb);
	SPI_Reading(&addr_temp_xlsb);
	res_temp_xlsb[0] = SPI_Reading(&addr_temp_xlsb);
		
	result = (((res_press_msb[0] << 8) | res_press_lsb[0]) << 4); // Измеренное значение давления в 20-битном формате
	pressure = BMP_calculation(result); // Пересчет давления с учетом параметров компенсации
		
	result = 0; // Обнуляем результат для следующего измерения
	result = (((res_temp_msb[0] << 8) | res_temp_lsb[0]) << 4); // Измеренное значение температуры в 20-битном формате
	
	temp = Temp_calculation(result); // Пересчет температуры с учетом параметров компенсации
		
//------------------------------Получение данных с датчика DHT11-------------------------------------------------

  DHT_data d = DHT_getData(&livingRoom); 
		
//------------------------------Расчет температуры точки росы-----------------------------------------------------

	H = d.hum; // Полученная влажность с DHT11
	Ta = temp; // Полученная температура с BMP280
	
	func_dp = a * Ta / (b + Ta) + log(H / 100); // Вычисление функции по формуле
	t_dew_point = (b * func_dp) / (a - func_dp); // Значение температуры в градусах Цельсия
		
//------------------------------Печать данных на дисплей-----------------------------------------------------		
	
	lcd_print(0,0, FONT_1X, (unsigned char*)str);
	
  sprintf(hum_msg, "H = %d %%", d.hum);
  lcd_print(0,1, FONT_1X, (unsigned char*)hum_msg);
	
	sprintf(press_msg, "P = %.1f кПа", pressure);
	lcd_print(0,2, FONT_1X, (unsigned char*)press_msg);
	
	sprintf(temp_msg, "T = %.1f гр.C", temp);
	lcd_print(0,3, FONT_1X, (unsigned char*)temp_msg);
	
	sprintf(tp_msg, "Tр = %.1f гр.C", t_dew_point);
	lcd_print(0,4, FONT_1X, (unsigned char*)tp_msg);
		
	Lcd_update(); 						// Обновление дисплея (отправка данных)
	Delay_ms(500);
	Lcd_clear();							// Очистка дисплея
	Delay_ms(1000);						// Обновление данных 1 раз в секунду
	
//--------------------------------------------------------------------------------------------------------------------		
			
	//SystemCoreClockUpdate();
	}
}
