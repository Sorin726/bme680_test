#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/cyw43_arch.h"
#include "drivers/bme680/bme680.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c1
#define I2C_SDA 2
#define I2C_SCL 3

int main()
{
    stdio_init_all();

    // Initialise the Wi-Fi chip
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        return -1;
    }

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c

    while (true) {
        // Example to turn on the Pico W LED
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(1000);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(1000);

        // Set the device to conversion mode
        uint8_t config_data_write = (0x01 << 5 | 0x01);
        i2c_write(I2C_PORT, BME680_I2C_ADDR, CTRL_MEAS, config_data_write, 1);

        uint8_t device_id;
        i2c_read(I2C_PORT, BME680_I2C_ADDR, CHIP_ID, &device_id, 1);

        // Send device ID over UART
        printf("Device ID: 0x%02X\n", device_id);

        // Read temperature sequence
        // Read the filter settings
        uint8_t config_data;
        i2c_read(I2C_PORT, BME680_I2C_ADDR, CONFIG_REG, &config_data, 1);
        printf("Config data: 0x%02X\n", config_data);

        uint8_t filter = config_data & 0x0E; // Bits 2-4
        printf("Filter setting: %d\n", filter);

        // Read osrs_t (temperature oversampling)
        uint8_t osrs_t_data;
        i2c_read(I2C_PORT, BME680_I2C_ADDR, CTRL_MEAS, &osrs_t_data, 1);
        printf("OSRS_T data: 0x%02X\n", osrs_t_data);
        uint8_t osrs_t = osrs_t_data & 0xE0; // Bits 5-7
        printf("OSRS_T setting: %d\n", osrs_t);

        // Read temp_raw data
        uint8_t temp_raw_data[3];
        i2c_read(I2C_PORT, BME680_I2C_ADDR, TEMP_MSB, temp_raw_data, 3);

        // Print raw temperature data
        printf("Raw temperature data: 0x%02X 0x%02X 0x%02X\n", temp_raw_data[0], temp_raw_data[1], temp_raw_data[2]);

        int32_t temp_raw = ((int32_t)temp_raw_data[0] << 12) | ((int32_t)temp_raw_data[1] << 4) | ((int32_t)temp_raw_data[2] >> 4);
        printf("Raw temperature: %d\n", temp_raw);

        temp_calib_data calib_data = read_temp_cal(I2C_PORT, BME680_I2C_ADDR);
        double temp_comp = calculate_temp(temp_raw, calib_data);

        printf("Temperature: %.2f °C\n", temp_comp);
    }
}
