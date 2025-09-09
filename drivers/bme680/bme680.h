#ifndef BME680_H
#define BME680_H

#define BME680_I2C_ADDR 0x77
#define STATUS_REG 0x73
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

// Temperature calibration registers
#define PAR_T1_LSB 0xE9
#define PAR_T1_MSB 0xEA
#define PAR_T2_LSB 0x8A
#define PAR_T2_MSB 0x8B
#define PAR_T3 0x8C

// Humidity calibration registers
#define PAR_H1_H2_LSB 0xE2 // Note: This register contains bits for both H1 and H2
#define PAR_H1_LSB_MSK 0x0F
#define PAR_H2_MSB_MSK 0xF0
#define PAR_H1_MSB 0xE3
#define PAR_H2_MSB 0xE1
#define PAR_H3 0xE4
#define PAR_H4 0xE5
#define PAR_H5 0xE6
#define PAR_H6 0xE7
#define PAR_H7 0xE8

// Structure to hold temperature calibration data
typedef struct {
    uint16_t par_t1;
    uint16_t par_t2;
    uint8_t par_t3;
} temp_calib_data;

// Structure to hold humidity calibration data
typedef struct {
    uint16_t par_h1;
    uint16_t par_h2;
    uint8_t par_h3;
    uint8_t par_h4;
    uint8_t par_h5;
    uint8_t par_h6;
    uint8_t par_h7;
} hum_calib_data;

int i2c_read(i2c_inst_t *i2c_port, uint8_t device_address, uint8_t register_address, uint8_t *data, size_t length);
int i2c_write(i2c_inst_t *i2c_port, uint8_t device_address, uint8_t register_address, uint8_t data, size_t length);
temp_calib_data read_temp_cal(i2c_inst_t *i2c_port, uint8_t device_address);
uint32_t read_temp(i2c_inst_t *i2c_port, uint8_t device_address);
double calculate_temp(uint32_t temp_adc, temp_calib_data calib_data);
hum_calib_data read_hum_cal(i2c_inst_t *i2c_port, uint8_t device_address);
uint16_t read_hum(i2c_inst_t *i2c_port, uint8_t device_address);
double calculate_hum(uint16_t hum_adc, hum_calib_data calib_data, double temp);

#endif // BME680_H