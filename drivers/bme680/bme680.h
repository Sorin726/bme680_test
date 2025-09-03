#ifndef BME680_H
#define BME680_H

#define BME680_I2C_ADDR 0x77
#define RESET_REG 0xE0
#define CHIP_ID 0xD0
#define CONFIG_REG 0x75
#define CTRL_MEAS 0x74
#define CTRL_HUM 0x72
#define CTRL_GAS_1 0x71
#define CTRL_GAS_0 0x70
#define HUM_LSB 0x26
#define HUM_MSB 0x25
#define TEMP_XLSB 0x24
#define TEMP_LSB 0x23
#define TEMP_MSB 0x22

#define PAR_T1_LSB 0xE9
#define PAR_T1_MSB 0xEA
#define PAR_T2_LSB 0x8A
#define PAR_T2_MSB 0x8B
#define PAR_T3 0x8C

// Structure to hold temperature calibration data
typedef struct {
    uint16_t par_t1;
    uint16_t par_t2;
    uint8_t par_t3;
} temp_calib_data;

int i2c_read(i2c_inst_t *i2c_port, uint8_t device_address, uint8_t register_address, uint8_t *data, size_t length);
int i2c_write(i2c_inst_t *i2c_port, uint8_t device_address, uint8_t register_address, uint8_t data, size_t length);
temp_calib_data read_temp_cal(i2c_inst_t *i2c_port, uint8_t device_address);
uint32_t read_temp(i2c_inst_t *i2c_port, uint8_t device_address);
double calculate_temp(uint32_t temp_adc, temp_calib_data calib_data);

#endif // BME680_H