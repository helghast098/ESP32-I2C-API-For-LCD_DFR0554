#ifndef LCD_DFR0554
#define LCD_DFR0554

#include <stdint.h>
#include "esp_err.h"

#include "driver/i2c_master.h"

typedef struct { uint8_t r;
				 uint8_t g;
				 uint8_t b; } Color_RGB_t;

/**
* @brief Color Modes for display
*
*/
typedef enum { COLOR_BLUE,
			   COLOR_GREEN,
			   COLOR_RED,
			   COLOR_WHITE } Color_Solid_t;

/**
* @brief Modes off and on for different functions
*
*/
typedef enum { MODE_OFF,
			   MODE_ON } Mode_t;

/**
* @brief Scroll Mode for display
*
*/
typedef enum { SCROLL_LEFT,
			   SCROLL_RIGHT } Scroll_Direction_t;


/**
* @brief Initialize LCD
*
* @param masterBusHandle: takes in an handle to a master bus.
* @return
*		- ESP_OK: On success
*		- ESP_ERR_NO_MEM: Depends on i2c_master_bus_add_device
*       - ESP_ERR_TIMEOUT: Device didn't respond. Depends on i2c_master_transmit
*
*/
esp_err_t LCD_Start(i2c_master_bus_handle_t masterBusHandle); //

/**
* @brief Turn on/off display
*
* @param displayMode: Can be MODE_ON or MODE_OFF
* @return
*		- ESP_OK: On success
*       - ESP_ERR_TIMEOUT: Device didn't respond. Depends on i2c_master_transmit
*
*/
esp_err_t LCD_Display_Mode(Mode_t displayMode); //

/**
* @brief Turn on or off blinking cursor
*
* @param blinkMode: Can be MODE_ON or MODE_OFF
* @return
*		- ESP_OK: On success
*       - ESP_ERR_TIMEOUT: Device didn't respond. Depends on i2c_master_transmit
*
*/
esp_err_t LCD_Blink_Mode(Mode_t blinkMode); //

/**
* @brief Turn on or off cursor
*
* @param cursorMode: Can be MODE_ON or MODE_OFF
* @return
*		- ESP_OK: On success
*       - ESP_ERR_TIMEOUT: Device didn't respond. Depends on i2c_master_transmit
*
*/
esp_err_t LCD_Cursor_Mode(Mode_t cursorMode); //

/**
* @brief Turn on or off autoscroll.
*
* @param scrollMode: Can be MODE_ON or MODE_OFF
* @return
*		- ESP_OK: On success
*       - ESP_ERR_TIMEOUT: Device didn't respond. Depends on i2c_master_transmit
*
*/
esp_err_t LCD_Autoscroll_Mode (Mode_t scrollMode); //

/**
* @brief Move Cursor Left or Right
*
* @param scrollDirection: Can be SCROLL_LEFT or SCROLL_RIGHT
* @return
*		- ESP_OK: On success
*       - ESP_ERR_TIMEOUT: Device didn't respond. Depends on i2c_master_transmit
*
*/
esp_err_t LCD_Scroll_Direction (Scroll_Direction_t scrollDirection); //

/**
* @brief Set Solid Color of backlight
*
* @param solidColor: Color to change backlight to
* @return
*		- ESP_OK: On success
*       - ESP_ERR_TIMEOUT: Device didn't respond. Depends on i2c_master_transmit
*
*/
esp_err_t LCD_Set_Color (Color_Solid_t solidColor); //

/**
* @brief Set rgb color of backlight
*
* @param solidColor: Color to change backlight to
* @return
*		- ESP_OK: On succes
*       - ESP_ERR_TIMEOUT: Device didn't respond. Depends on i2c_master_transmit
*
*/
esp_err_t LCD_Set_RGB (Color_RGB_t color); //

/**
* @brief Set Cursor to position
*
* @param row: 0 for top line		1 for bottom line
* @param col: 0 - 15
* @return
*		- ESP_OK: On success
*		- ESP_ERR_INVALID_ARG: If row or col values fall out of range
*       - ESP_ERR_TIMEOUT: Device didn't respond. Depends on i2c_master_transmit
*
*/
esp_err_t LCD_Set_Cursor(uint8_t row, uint8_t col); //

/**
* @brief Clears screen and returns to cursor to top of screen
*
* @return
*		- ESP_OK: On success
*       - ESP_ERR_TIMEOUT: Device didn't respond. Depends on i2c_master_transmit
*
*/
esp_err_t LCD_Clear_Screen(); //

/**
* @brief Prints message to Screen
*
* @param str: String to print to LCD
* @return
*		- ESP_OK: On succes
*		- ESP_ERR_INVALID_ARG: str == NULL
*       - ESP_ERR_TIMEOUT: Device didn't respond. Depends on i2c_master_transmit
*
*/
esp_err_t LCD_Print(char *str);

#endif
