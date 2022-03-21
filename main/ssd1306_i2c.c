#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c.h"
#include "esp_log.h"

#include "ssd1306.h"

#define tag "SSD1306"
#define TAG "SSD1306"

#include <esp_log.h>
#include <esp_idf_lib_helpers.h>

#define I2C_FREQ_HZ 1000000
#define CONFIG_OFFSETX 0

#define CHECK(x) do { esp_err_t __; if ((__ = x) != ESP_OK) return __; } while (0)
#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)
#define CHECK_LOGE(dev, x, msg, ...) do { \
        esp_err_t __; \
        if ((__ = x) != ESP_OK) { \
            I2C_DEV_GIVE_MUTEX(&dev->i2c_dev); \
            ESP_LOGE(TAG, msg, ## __VA_ARGS__); \
            return __; \
        } \
    } while (0)

esp_err_t SSD1306_init_desc(SSD1306_t *dev, uint8_t addr, i2c_port_t port,
		gpio_num_t sda_gpio, gpio_num_t scl_gpio) {
	CHECK_ARG(dev);

	if (addr != I2CAddress) {
		ESP_LOGE(tag, "Invalid I2C address");
		return ESP_ERR_INVALID_ARG;
	}

	dev->i2c_dev.port = port;
	dev->i2c_dev.addr = addr;
	dev->i2c_dev.cfg.sda_io_num = sda_gpio;
	dev->i2c_dev.cfg.scl_io_num = scl_gpio;
	dev->i2c_dev.cfg.master.clk_speed = I2C_FREQ_HZ;
	dev->_address = I2CAddress;

	return i2c_dev_create_mutex(&dev->i2c_dev);
}

esp_err_t SSD1306_i2cdev_init(SSD1306_t *dev, int width, int height) {
	CHECK_ARG(dev);
	I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);

	dev->_width = width;
	dev->_height = height;
	dev->_pages = 8;
	if (dev->_height == 32)
		dev->_pages = 4;

	uint8_t init_data[] = {
	OLED_CONTROL_BYTE_CMD_STREAM,
	OLED_CMD_DISPLAY_OFF,
	OLED_CMD_SET_MUX_RATIO,
	0x1F, // 128x32
	OLED_CMD_SET_DISPLAY_OFFSET,
	0x00,
	OLED_CONTROL_BYTE_DATA_STREAM,
	OLED_CMD_SET_SEGMENT_REMAP,
	OLED_CMD_SET_COM_SCAN_MODE,
	OLED_CMD_SET_DISPLAY_CLK_DIV,
	0x80,
	OLED_CMD_SET_COM_PIN_MAP,
	0x02, // 128x32
	OLED_CMD_SET_CONTRAST,
	0xFF,
	OLED_CMD_DISPLAY_RAM,
	OLED_CMD_SET_VCOMH_DESELCT,
	0x40,
	OLED_CMD_SET_MEMORY_ADDR_MODE,
	OLED_CMD_SET_PAGE_ADDR_MODE,
	0x00,
	0x10,
	OLED_CMD_SET_CHARGE_PUMP,
	0x14,
	OLED_CMD_DEACTIVE_SCROLL,
	OLED_CMD_DISPLAY_NORMAL,
	OLED_CMD_DISPLAY_ON
	};

	ESP_LOGI(tag,"init data: %i bytes", sizeof init_data);
	CHECK_LOGE(dev, i2c_dev_write(&dev->i2c_dev, NULL, 0, init_data, sizeof(init_data) ),
			"OLED configuration failed");

	I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);

	return ESP_OK;
}

esp_err_t i2cdev_display_image(SSD1306_t *dev, int page, int seg, uint8_t *images, int width) {

	CHECK_ARG(dev);
	I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);

	if (page >= dev->_pages)
		return -1;
	if (seg >= dev->_width)
		return -1;

	int _seg = seg + CONFIG_OFFSETX;
	uint8_t columLow = _seg & 0x0F;
	uint8_t columHigh = (_seg >> 4) & 0x0F;

	uint8_t init_data[] = {
			OLED_CONTROL_BYTE_CMD_STREAM,
			(0x00 + columLow),
			(0x10 + columHigh),
			0xB0 | page
	};

	CHECK_LOGE(dev, i2c_dev_write(&dev->i2c_dev, NULL, 0, init_data, 4 ),"init draw failed");

	uint8_t write_control[] = {
			OLED_CONTROL_BYTE_DATA_STREAM
	};

	CHECK_LOGE(dev,i2c_dev_write(&dev->i2c_dev, write_control, 1, images, 8 ),"Display Image failed");

	I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);

	return ESP_OK;
}

esp_err_t i2cdev_contrast(SSD1306_t *dev, int contrast) {

	CHECK_ARG(dev);
	I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);

	int _contrast = contrast;
	if (contrast < 0x0)
		_contrast = 0;
	if (contrast > 0xFF)
		_contrast = 0xFF;

	uint8_t init_data[] = {
				OLED_CONTROL_BYTE_CMD_STREAM,
				OLED_CMD_SET_CONTRAST,
				_contrast
		};

	CHECK_LOGE(dev,i2c_dev_write(&dev->i2c_dev, NULL, 0, init_data, 3),"Contrast setting failed" );

	I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);

	return ESP_OK;
}

esp_err_t i2cdev_hardware_scroll(SSD1306_t *dev, ssd1306_scroll_type_t scroll) {
	CHECK_ARG(dev);
	I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);



	uint8_t scroll_right[] = {
			OLED_CONTROL_BYTE_CMD_STREAM,
			OLED_CMD_HORIZONTAL_RIGHT,
			0x00,
			0x00,
			0x08,
			0x07,
			0x00,
			0xFF,
			OLED_CMD_ACTIVE_SCROLL
	};

	uint8_t scroll_left[] = {
				OLED_CONTROL_BYTE_CMD_STREAM,
				OLED_CMD_HORIZONTAL_LEFT,
				0x00,
				0x00,
				0x08,
				0x07,
				0x00,
				0xFF,
				OLED_CMD_ACTIVE_SCROLL
		};



	uint8_t scroll_down[] = {
					OLED_CONTROL_BYTE_CMD_STREAM,
					OLED_CMD_CONTINUOUS_SCROLL,
					0x00, 				// A  dummy byte
					0x04, 				// B  page start addr
					0x02, 				// C  speed
					0x04, 				// D  end page addres was 00
					0x1f, 				// E  vertical scrolling offset  3f
					OLED_CMD_VERTICAL, 	//Set Vertical Scroll Area
					0x00, 				// Set No. of rows in top fixed area.
					0x20, 				//32  Set No. of rows in scroll area
					OLED_CMD_ACTIVE_SCROLL
			};

	uint8_t scroll_up[] = {
						OLED_CONTROL_BYTE_CMD_STREAM,
						OLED_CMD_CONTINUOUS_SCROLL,
						0x00, 				// A  dummy byte
						0x04, 				// B  page start addr
						0x02, 				// C  speed
						0x04, 				// D  end page addr
						0x01, 				// E  vertical scrolling offset
						OLED_CMD_VERTICAL, 	//Set Vertical Scroll Area
						0x00,  				// Set No. of rows in top fixed area.
						0x20,  				//32  Set No. of rows in scroll area
						OLED_CMD_ACTIVE_SCROLL
				};

	uint8_t scroll_stop[] = {
			OLED_CONTROL_BYTE_CMD_STREAM,
			OLED_CMD_DEACTIVE_SCROLL
					};


	if (scroll == SCROLL_RIGHT) {
		CHECK_LOGE(dev,i2c_dev_write(&dev->i2c_dev, NULL, 0, scroll_right, sizeof scroll_right),"Scroll right failed" );
		}

		if (scroll == SCROLL_LEFT) {
			CHECK_LOGE(dev,i2c_dev_write(&dev->i2c_dev, NULL, 0, scroll_left, sizeof scroll_left),"Scroll left failed" );
		}

		if (scroll == SCROLL_DOWN) {
			CHECK_LOGE(dev,i2c_dev_write(&dev->i2c_dev, NULL, 0, scroll_down, sizeof scroll_down),"Scroll down failed" );
		}

		if (scroll == SCROLL_UP) {
			CHECK_LOGE(dev,i2c_dev_write(&dev->i2c_dev, NULL, 0, scroll_up, sizeof scroll_up),"Scroll up failed" );
		}

		if (scroll == SCROLL_STOP) {
			CHECK_LOGE(dev,i2c_dev_write(&dev->i2c_dev, NULL, 0, scroll_stop, sizeof scroll_stop),"Scroll stop failed" );
			}

		I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);
		return ESP_OK;

}

/*
*/
