#include <string.h>
#include "LCD_DFR0554.h"
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/ets_sys.h"

/*For More Info about what each register bit does for RGB check datasheet*/

/*Clock speed of i2c and TIMEOUT for response*/
#define CLK_SPEED 400000
#define TIMEOUT_MS 100

/*I2C Addresses of Backlight and LCD*/
#define DISPLAY_I2C_ADDRESS 0x3E // Display i2c address
#define RGB_I2C_ADDRESS 0x60 // Backlight i2c address

/*========== Display MACROS ==========*/

/*Instruction Starts*/
#define DISPLAY_COMMAND_START       0x00
#define DISPLAY_COMMAND_DDRAM_WRITE 0x40

/*Clear Display Instruction*/
#define DISPLAY_CLEAR 0x01

/*Entry Mode Instruction*/
#define DISPLAY_ENTRY_MODE 0x04
#define _ENTRY_LEFT        0x00
#define _ENTRY_RIGHT       0x02
#define _ENTRY_SHIFT_INC   0x01
#define _ENTRY_SHIFT_DEC   0x00

/*ON_OFF Instruction*/
#define DISPLAY_ON_OFF  0X08
#define _DIS_ON         0x04
#define _DIS_OFF        0x00
#define _DIS_CURSOR_ON  0x02
#define _DIS_CURSOR_OFF 0x00
#define _DIS_BLINK_ON   0x01
#define _DIS_BLINK_OFF  0x00

/*Cursor Shift Instruction*/
#define DISPLAY_CURSOR_OR_SHIFT 0x10
#define _CURSOR_LEFT            0x00
#define _CURSOR_RIGHT           0x04
#define _DISPLAY_LEFT           0x08
#define _DISPLAY_RIGHT          0x0C

/*Function Set Instruction*/
#define DISPLAY_FUNCTION_SET 0x20
#define _FUNC_ROW_NUM1       0x00 // 1 Line Display
#define _FUNC_ROW_NUM2       0x08 // 2 Line Display
#define _FUNC_FONT_5x8       0x00 // 5x8 Font
#define _FUNC_FONT_5x10      0x04 // 5x10 Font

/*DDRAM Set Address Instruction*/
#define DISPLAY_SET_DDRAM_ADDR   0x80
#define _DDRAM_FIRST_LINE_START  0x00
#define _DDRAM_FIRST_LINE_END    0x27
#define _DDRAM_SECOND_LINE_START 0x40
#define _DDRAM_SECOND_LINE_END   0x67

/*========== Backlight MACROS ==========*/
/*RGB Backlight Registers*/
#define R_RGB_MODE1      0x00
#define R_RGB_MODE2      0x01
#define R_RGB_LED0_PWM   0x02
#define R_RGB_LED1_PWM   0x03
#define R_RGB_LED2_PWM   0x04
#define R_RGB_LED3_PWM   0x05
#define R_RGB_GRPPWM     0x06
#define R_RGB_GRPFREQ    0x07
#define R_RGB_LEDOUT     0x08
#define R_RGB_SUBADR1    0x09
#define R_RGB_SUBADR2    0x0A
#define R_RGB_SUBADR3    0x0B
#define R_RGB_ALLCALLADR 0x0C

/*RGB Control Register Auto-Increment Flags*/
#define F_RGB_NO_AUTO_INCREMENT         0x00 // Don't increment register address
#define F_RGB_AUTO_INCREMENT_ALL        0x80 // Increment through all registers
#define F_RGB_AUTO_INCRENT_PWM          0xA0 // Increment through PWM Registers
#define F_RGB_AUTO_INCREMENT_GLOBAL     0xC0 // Increment Global Registers
#define F_RGB_AUTO_INCREMENT_GLOBAL_ALL 0xE0 // Increment Global and PWM Registers

/*i2c device handles for backlight and display*/
static i2c_master_dev_handle_t backlightDeviceHandle;
static i2c_master_dev_handle_t LCDDeviceHandle;

/*static global vars to hold states of display*/
static Mode_t displayCurrentMode = MODE_OFF;
static Mode_t cursorCurrentMode = MODE_OFF;
static Mode_t cursorCurrentBlinkMode = MODE_OFF;
static Mode_t autoScrollCurrentMode = MODE_OFF;

static esp_err_t Display_Command_Setup(uint8_t command) {
	uint8_t dataToSend[2] = {DISPLAY_COMMAND_START, command};
	ESP_RETURN_ON_ERROR(i2c_master_transmit(LCDDeviceHandle, dataToSend, 2, TIMEOUT_MS),
									"LCD_DFR0554 -> private: Display_Command_Setup()", "Issue transmitting command to Display");
	ets_delay_us(50); // Delay after each command.  Note Some commands require larger delays which are called in their respective functions
	return ESP_OK;
}

static esp_err_t Display_Print_Setup(uint8_t character) {
	uint8_t dataToSend[2] = {DISPLAY_COMMAND_DDRAM_WRITE, character};
	ESP_RETURN_ON_ERROR(i2c_master_transmit(LCDDeviceHandle, dataToSend, 2, TIMEOUT_MS),
									"LCD_DFR0554 -> private: Display_pRINT_Setup()", "Issue transmitting command to Display");
	ets_delay_us(50); // Delay after each command.  Note Some commands require larger delays which are called in their respective functions
	return ESP_OK;
}

esp_err_t LCD_Start(i2c_master_bus_handle_t masterBusHandle) {
	vTaskDelay(pdMS_TO_TICKS(18)); // Initial startup wait time > 15ms

	ESP_RETURN_ON_FALSE(masterBusHandle != NULL, ESP_ERR_INVALID_ARG, "LCD_DFR0554 -> LCD_Start()", "masterBusHandle is NULL");
	esp_err_t errorStatus = ESP_OK;

/*Connecting Backlight to master bus*/
	i2c_device_config_t devConfig = {};
	devConfig.dev_addr_length = I2C_ADDR_BIT_LEN_7;
	devConfig.device_address = RGB_I2C_ADDRESS;
	devConfig.scl_speed_hz = CLK_SPEED;

	errorStatus = i2c_master_bus_add_device(masterBusHandle, &devConfig, &backlightDeviceHandle);
	ESP_RETURN_ON_ERROR(errorStatus, "LCD_DFR0554 -> LCD_Start()", "Could Not add backlight to master bus");

/*Connecting Display to master bus*/
	devConfig.device_address = DISPLAY_I2C_ADDRESS;
	errorStatus = i2c_master_bus_add_device(masterBusHandle, &devConfig, &LCDDeviceHandle);
	ESP_RETURN_ON_ERROR(errorStatus, "LCD_DFR0554 -> LCD_Start()", "Could Not add Display to master bus");

/*Turning on RGB Backlight of LCD*/

	/* Writing to RGB_MODE_1 reg*/
	uint8_t dataSend[2] = {R_RGB_MODE1, 0x00}; // Set sleep to normal
	errorStatus = i2c_master_transmit(backlightDeviceHandle, dataSend, 2, TIMEOUT_MS);
	ESP_RETURN_ON_ERROR(errorStatus, "LCD_DFR0554 -> LCD_Start()", "Issue Writing to backlight");

	/*Writing to RGB_LEDOUT reg*/
	dataSend[0] = R_RGB_LEDOUT;
	dataSend[1] = 0x2A; // Sets each LED to be controlled by PWM except last one
	errorStatus = i2c_master_transmit(backlightDeviceHandle, dataSend, 2, TIMEOUT_MS);
	ESP_RETURN_ON_ERROR(errorStatus, "LCD_DFR0554 -> LCD_Start()", "Issue transmitting to backlight");

	/*Setting Color to white*/
	ESP_RETURN_ON_ERROR(LCD_Set_Color(COLOR_WHITE), "LCD_DFR0554 -> LCD_Start() -> LCD_Set_Color()", "Issue setting color");

/*Setting Up The Display*/

	/*Function Set Setup*/
	ESP_RETURN_ON_ERROR(Display_Command_Setup(DISPLAY_FUNCTION_SET | _FUNC_ROW_NUM2), "LCD_DFR0554 -> LCD_Start()", "Issue setting display");

	/*Display On / OFF Setup*/
	ESP_RETURN_ON_ERROR(LCD_Display_Mode(MODE_ON), "LCD_DFR0554 -> LCD_Start() -> LCD_Display_Mode()", "Issue turning on display");

	/*Display Clear*/
	ESP_RETURN_ON_ERROR(LCD_Clear_Screen(), "LCD_DFR0554 -> LCD_Start() -> LCD_Clear_Screen()", "Issue  clearing screen");
	vTaskDelay(pdMS_TO_TICKS(3)); // Datasheet required delay

	/*Entry Mode Setup*/
	ESP_RETURN_ON_ERROR(Display_Command_Setup(DISPLAY_ENTRY_MODE | _ENTRY_RIGHT), "LCD_DFR0554 -> LCD_Start() -> LCD_Clear_Screen()", "Issue clearing screen");

	return ESP_OK;
}

esp_err_t LCD_Display_Mode(Mode_t displayMode) {
	if (displayMode == displayCurrentMode) return ESP_OK;

	uint8_t dataToSend = DISPLAY_ON_OFF;
	if (displayMode == MODE_ON) dataToSend |= _DIS_ON;
	if (cursorCurrentMode == MODE_ON) dataToSend |= _DIS_CURSOR_ON;
	if (cursorCurrentBlinkMode == MODE_ON) dataToSend |= _DIS_BLINK_ON;
	ESP_RETURN_ON_ERROR(Display_Command_Setup(dataToSend), "LCD_DFR0554 -> LCD_Display_Mode()", "Issue transmitting to Display");
	displayCurrentMode = displayMode == MODE_ON ? MODE_ON : MODE_OFF;

	return ESP_OK;
}

esp_err_t LCD_Blink_Mode(Mode_t blinkMode) {
	if (cursorCurrentBlinkMode == blinkMode) return ESP_OK;

	cursorCurrentBlinkMode = blinkMode;

	if (displayCurrentMode == MODE_OFF || cursorCurrentMode == MODE_OFF) return ESP_OK;

	uint8_t dataToSend = DISPLAY_ON_OFF | _DIS_ON;
	dataToSend |= (blinkMode == MODE_ON) ? _DIS_BLINK_ON : _DIS_BLINK_OFF;
	dataToSend |= _DIS_CURSOR_ON;
	ESP_RETURN_ON_ERROR(Display_Command_Setup(dataToSend), "LCD_DFR0554 -> LCD_Blink_Mode()", "Error setting blinking mode");

	return ESP_OK;
}

esp_err_t LCD_Cursor_Mode(Mode_t cursorMode) {
	if (cursorCurrentMode == cursorMode) return ESP_OK;

	cursorCurrentMode = cursorMode;

	if (displayCurrentMode == MODE_OFF) return ESP_OK;

	uint8_t dataToSend = DISPLAY_ON_OFF | _DIS_ON;

	if (cursorMode != MODE_OFF)
	{
		dataToSend |= _DIS_CURSOR_ON;
		dataToSend |= (cursorCurrentBlinkMode == MODE_ON) ? _DIS_BLINK_ON : _DIS_BLINK_OFF;
	}

	ESP_RETURN_ON_ERROR(Display_Command_Setup(dataToSend), "LCD_DFR0554 -> LCD_Blink_Mode()", "Error setting cursor mode");

	return ESP_OK;
}

esp_err_t LCD_Autoscroll_Mode (Mode_t scrollMode) {
	if (autoScrollCurrentMode == scrollMode) return ESP_OK;

	uint8_t dataToSend = DISPLAY_ENTRY_MODE | _ENTRY_RIGHT;

	dataToSend |= (scrollMode == MODE_ON) ? _ENTRY_SHIFT_INC : _ENTRY_SHIFT_DEC;

	ESP_RETURN_ON_ERROR(Display_Command_Setup(dataToSend), "LCD_DFR0554 -> LCD_Autoscroll_Mode", "Issue setting up autoscroll");

	return ESP_OK;
}

esp_err_t LCD_Scroll_Direction (Scroll_Direction_t scrollDirection) {
	uint8_t dataToSend = DISPLAY_CURSOR_OR_SHIFT;
	dataToSend |= (scrollDirection == SCROLL_LEFT) ? _CURSOR_LEFT: _CURSOR_RIGHT;

	ESP_RETURN_ON_ERROR(Display_Command_Setup(dataToSend), "LCD_DFR0554 -> LCD_Scroll_Direction()", "Issue moving cursor");

	return ESP_OK;
}

esp_err_t LCD_Set_Color (Color_Solid_t solidColor) {
	esp_err_t errorStatus = ESP_OK;
	uint8_t dataToSend[4] = {F_RGB_AUTO_INCRENT_PWM | R_RGB_LED0_PWM, 0xFF, 0xFF, 0xFF};
	switch(solidColor)
	{
		case COLOR_WHITE:
			errorStatus = i2c_master_transmit(backlightDeviceHandle, dataToSend, 4, TIMEOUT_MS);
			break;

		case COLOR_RED:
			dataToSend[1] = 0x00; // turning off blue
			dataToSend[2] = 0x00; // turning off green
			errorStatus = i2c_master_transmit(backlightDeviceHandle, dataToSend, 4, TIMEOUT_MS);
			break;

		case COLOR_BLUE:
			dataToSend[2] = 0x00; // turning off green
			dataToSend[3] = 0x00; // turning off red
			errorStatus = i2c_master_transmit(backlightDeviceHandle, dataToSend, 4, TIMEOUT_MS);
			break;

		case COLOR_GREEN:
			dataToSend[1] = 0x00; // turning off blue
			dataToSend[3] = 0x00; // turning off red
			errorStatus = i2c_master_transmit(backlightDeviceHandle, dataToSend, 4, TIMEOUT_MS);
			break;

		default:
			ESP_LOGW("LCD_DFR0554 -> LCD_Set_Color()", "Invalid argument provide to function");
			return ESP_ERR_INVALID_ARG;
	}
	ESP_RETURN_ON_ERROR(errorStatus, "LCD_DFR0554 -> LCD_Set_Color()", "Error setting color");
	ets_delay_us(50);
	return ESP_OK;
}

esp_err_t LCD_Set_RGB (Color_RGB_t rgbColor) {
/*Writing to each PWM register*/
	uint8_t dataColor[4] = {F_RGB_AUTO_INCRENT_PWM | R_RGB_LED0_PWM, rgbColor.b, rgbColor.g, rgbColor.r};

	esp_err_t errorStatus = ESP_OK;
	errorStatus = i2c_master_transmit(backlightDeviceHandle, dataColor, 4, TIMEOUT_MS);
	ESP_RETURN_ON_ERROR(errorStatus, "LCD_DFR0554 -> LCD_Set_RGB()", "Couldn't set rgb color");
	ets_delay_us(50);
	return ESP_OK;
}

esp_err_t LCD_Set_Cursor(uint8_t row, uint8_t col) {
	bool checker = (col <= 15) && (row <= 1);
	ESP_RETURN_ON_FALSE(checker, ESP_ERR_INVALID_ARG, "LCD_DFR0554 -> LCD_Set_Cursor()", "Invalid col or row argument");
	uint8_t rowOffSet = (row == 0) ? _DDRAM_FIRST_LINE_START : _DDRAM_SECOND_LINE_START;

	uint8_t dataToSend = DISPLAY_SET_DDRAM_ADDR | (col + rowOffSet);

	ESP_RETURN_ON_ERROR(Display_Command_Setup(dataToSend), "LCD_DFR0554 -> LCD_Set_Cursor()", "Issue setting cursor");

	return ESP_OK;
}

esp_err_t LCD_Clear_Screen() {
	ESP_RETURN_ON_ERROR(Display_Command_Setup(DISPLAY_CLEAR), "LCD_DFR0554 -> LCD_Clear_Screen()", "Issue transmitting to display");
	vTaskDelay(pdMS_TO_TICKS(3)); // Datasheet required delay
	return ESP_OK;
}

esp_err_t LCD_Print(char *str) {
	ESP_RETURN_ON_FALSE(str != NULL, ESP_ERR_INVALID_ARG, "LCD_DFR0554 -> LCD_Print()", "str points to NULL");


	for (int i = 0; i < strlen(str); ++i)
	{
		if (i == 16) LCD_Autoscroll_Mode(MODE_ON);
		ESP_RETURN_ON_ERROR(Display_Print_Setup((uint8_t) str[i]), "LCD_DFR0554 -> LCD_Print()", "Issue printing to screen");
	}

	return ESP_OK;
}
