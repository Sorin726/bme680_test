#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "drivers/bme680/bme680.h"

// Here a class can be used and its constructor to set the i2c port
// Also a destructor to close the i2c port
// And posibility to use the PIO as an alternative to I2C port
int i2c_read(i2c_inst_t *i2c_port, uint8_t device_address, uint8_t register_address, uint8_t *data, size_t length) {
    // Write the register address to the device
    // uint8_t reg_addr = register_address;

    int status = i2c_write_blocking(i2c_port, device_address, &register_address, 1, true);
    if (status < 0) {
        return status; // Return error code
    }

    // Read the data from the device
    status = i2c_read_blocking(i2c_port, device_address, data, length, false);
    return status; // Return number of bytes read or error code
}

int i2c_write(i2c_inst_t *i2c_port, uint8_t device_address, uint8_t register_address, uint8_t data, size_t length) {
    // Write the register address to the device
    uint8_t buffer[length+1];
    buffer[0] = register_address;
    buffer[1] = data;
    int status = i2c_write_blocking(i2c_port, device_address, buffer, length+1, true);
    if (status < 0) {
        return status; // Return error code
    }

    return 0;
}

// Function the temperature calibration values
temp_calib_data read_temp_cal(i2c_inst_t *i2c_port, uint8_t device_address) {
    uint8_t par_t1_lsb, par_t1_msb, par_t2_lsb, par_t2_msb, par_t3;
    i2c_read(i2c_port, device_address, PAR_T1_LSB, &par_t1_lsb, 1);
    i2c_read(i2c_port, device_address, PAR_T1_MSB, &par_t1_msb, 1);
    i2c_read(i2c_port, device_address, PAR_T2_LSB, &par_t2_lsb, 1);
    i2c_read(i2c_port, device_address, PAR_T2_MSB, &par_t2_msb, 1);
    i2c_read(i2c_port, device_address, PAR_T3, &par_t3, 1);

    // Combine the calibration values
    uint16_t par_t1 = (par_t1_msb << 8) | par_t1_lsb;
    uint16_t par_t2 = (par_t2_msb << 8) | par_t2_lsb;

    temp_calib_data calib_data = {par_t1, par_t2, par_t3};
    return calib_data;
}

// Function to read temperature values
uint32_t read_temp(i2c_inst_t *i2c_port, uint8_t device_address) {
    uint8_t temp_adc[3];
    i2c_read(i2c_port, device_address, TEMP_MSB, temp_adc, 3);

    // Combine the raw temperature data
    int32_t temp_raw = ((int32_t)temp_adc[0] << 12) | ((int32_t)temp_adc[1] << 4) | ((int32_t)temp_adc[2] >> 4);
    return temp_raw;
}

// Function to calculate temperature
double calculate_t_fine(uint32_t temp_adc, temp_calib_data calib_data) {
    double var1 = (((double)temp_adc / 16384.0) - ((double)calib_data.par_t1 / 1024.0)) * (double)calib_data.par_t2;
    double var2 = ((((double)temp_adc / 131072.0) - ((double)calib_data.par_t1 / 8192.0)) *
                  (((double)temp_adc / 131072.0) - ((double)calib_data.par_t1 / 8192.0))) *
                  ((double)calib_data.par_t3 * 16.0);

    double t_fine = var1 + var2;

    return t_fine;
}

// Function to calculate temperature in degree Celsius
// It can be replaced with an inline function
double calculate_temp(double t_fine) {
    double temp_comp = t_fine / 5120.0;
    return temp_comp;
}

// Function the humidity calibration values
hum_calib_data read_hum_cal(i2c_inst_t *i2c_port, uint8_t device_address) {
    uint8_t par_h1_h2_lsb, par_h1_msb, par_h2_msb, par_h3, par_h4, par_h5, par_h6, par_h7;
    i2c_read(i2c_port, device_address, PAR_H1_H2_LSB, &par_h1_h2_lsb, 1);
    i2c_read(i2c_port, device_address, PAR_H1_MSB, &par_h1_msb, 1);
    i2c_read(i2c_port, device_address, PAR_H2_MSB, &par_h2_msb, 1);
    i2c_read(i2c_port, device_address, PAR_H3, &par_h3, 1);
    i2c_read(i2c_port, device_address, PAR_H4, &par_h4, 1);
    i2c_read(i2c_port, device_address, PAR_H5, &par_h5, 1);
    i2c_read(i2c_port, device_address, PAR_H6, &par_h6, 1);
    i2c_read(i2c_port, device_address, PAR_H7, &par_h7, 1);

    // Calculate par_h1_lsb and par_h2 lsb from the register
    uint8_t par_h1_lsb = par_h1_h2_lsb & PAR_H1_LSB_MSK;
    uint8_t par_h2_lsb = par_h1_h2_lsb >> 4;

    // Combine the calibration values
    uint16_t par_h1 = ((uint16_t)par_h1_msb << 4) | (uint16_t)par_h1_lsb;
    uint16_t par_h2 = ((uint16_t)par_h2_msb << 4) | (uint16_t)par_h2_lsb;

    hum_calib_data calib_data = {par_h1, par_h2, par_h3, par_h4, par_h5, par_h6, par_h7};
    return calib_data;
}

// Function to read humidity adc values
uint16_t read_hum(i2c_inst_t *i2c_port, uint8_t device_address) {
    uint8_t hum_adc[2];
    i2c_read(i2c_port, device_address, HUM_MSB, hum_adc, 2);

    // Combine the raw humidity data
    uint16_t hum_raw = ((uint16_t)hum_adc[0] << 8) | (uint16_t)hum_adc[1];
    return hum_raw;
}

// Function to calculate humidity
double calculate_hum(uint16_t hum_adc, hum_calib_data calib_data, double temp_comp) {
    // hum_adc is the 16-bit humidity ADC (MSB<<8 | LSB)
    uint16_t par_h1 = calib_data.par_h1;
    uint16_t par_h2 = calib_data.par_h2;
    int8_t par_h3 = calib_data.par_h3;
    int8_t par_h4 = calib_data.par_h4;
    int8_t par_h5 = calib_data.par_h5;
    uint8_t par_h6 = calib_data.par_h6;
    int8_t par_h7 = calib_data.par_h7;

    double var1 = hum_adc - (((double)par_h1 * 16.0) + (((double)par_h3 / 2.0) * temp_comp));
    double var2 = var1 * (((double)par_h2 / 262144.0) *
                 (1.0 + (((double)par_h4 / 16384.0) * temp_comp) +
                      (((double)par_h5 / 1048576.0) * temp_comp * temp_comp)));
    double var3 = (double)par_h6 / 16384.0;
    double var4 = (double)par_h7 / 2097152.0;
    double hum_comp = var2 + ((var3 + (var4 * temp_comp)) * var2 * var2);

    return hum_comp;
}

// Function to read pressure calibration values
press_calib_data read_press_cal(i2c_inst_t *i2c_port, uint8_t device_address) {
    uint8_t par_p1_lsb, par_p1_msb, par_p2_lsb, par_p2_msb, par_p3;
    uint8_t par_p4_lsb, par_p4_msb, par_p5_lsb, par_p5_msb, par_p6;
    uint8_t par_p7, par_p8_lsb, par_p8_msb, par_p9_lsb, par_p9_msb, par_p10;

    i2c_read(i2c_port, device_address, PAR_P1_LSB, &par_p1_lsb, 1);
    i2c_read(i2c_port, device_address, PAR_P1_MSB, &par_p1_msb, 1);
    i2c_read(i2c_port, device_address, PAR_P2_LSB, &par_p2_lsb, 1);
    i2c_read(i2c_port, device_address, PAR_P2_MSB, &par_p2_msb, 1);
    i2c_read(i2c_port, device_address, PAR_P3, &par_p3, 1);
    i2c_read(i2c_port, device_address, PAR_P4_LSB, &par_p4_lsb, 1);
    i2c_read(i2c_port, device_address, PAR_P4_MSB, &par_p4_msb, 1);
    i2c_read(i2c_port, device_address, PAR_P5_LSB, &par_p5_lsb, 1);
    i2c_read(i2c_port, device_address, PAR_P5_MSB, &par_p5_msb, 1);
    i2c_read(i2c_port, device_address, PAR_P6, &par_p6, 1);
    i2c_read(i2c_port, device_address, PAR_P7, &par_p7, 1);
    i2c_read(i2c_port, device_address, PAR_P8_LSB, &par_p8_lsb, 1);
    i2c_read(i2c_port, device_address, PAR_P8_MSB, &par_p8_msb, 1);
    i2c_read(i2c_port, device_address, PAR_P9_LSB, &par_p9_lsb, 1);
    i2c_read(i2c_port, device_address, PAR_P9_MSB, &par_p9_msb, 1);
    i2c_read(i2c_port, device_address, PAR_P10, &par_p10, 1);

    // Combine the calibration values
    uint16_t par_p1 = (par_p1_msb << 8) | par_p1_lsb;
    uint16_t par_p2 = (par_p2_msb << 8) | par_p2_lsb;
    uint16_t par_p4 = (par_p4_msb << 8) | par_p4_lsb;
    uint16_t par_p5 = (par_p5_msb << 8) | par_p5_lsb;
    uint16_t par_p8 = (par_p8_msb << 8) | par_p8_lsb;
    uint16_t par_p9 = (par_p9_msb << 8) | par_p9_lsb;

    press_calib_data calib_data = {par_p1, par_p2, par_p3, par_p4, par_p5, par_p6, par_p7, par_p8, par_p9, par_p10};
    return calib_data;

}

// Function to read pressure value
uint32_t read_press(i2c_inst_t *i2c_port, uint8_t device_address) {
    uint8_t press_adc[3];
    i2c_read(i2c_port, device_address, PRESS_MSB, press_adc, 3);

    // Combine the raw pressure data
    uint32_t press_raw = ((uint32_t)press_adc[0] << 12) | ((uint32_t)press_adc[1] << 4) | ((uint32_t)press_adc[2] >> 4);
    return press_raw;
}

// Function to calculate pressure
double calculate_press(uint32_t press_adc, press_calib_data calib_data, double t_fine) {
    // press_adc is the 20-bit pressure ADC (MSB<<12 | LSB<<4 | XLSB>>4)
    uint16_t par_p1 = calib_data.par_p1;
    int16_t par_p2 = calib_data.par_p2;
    int8_t par_p3 = calib_data.par_p3;
    int16_t par_p4 = calib_data.par_p4;
    int16_t par_p5 = calib_data.par_p5;
    int8_t par_p6 = calib_data.par_p6;
    int8_t par_p7 = calib_data.par_p7;
    int16_t par_p8 = calib_data.par_p8;
    int16_t par_p9 = calib_data.par_p9;
    uint8_t par_p10 = calib_data.par_p10;

    double var1 = ((double)t_fine / 2.0) - 64000.0;
    double var2 = var1 * var1 * ((double)par_p6 / 131072.0);
    var2 = var2 + (var1 * (double)par_p5 * 2.0);
    var2 = (var2 / 4.0) + ((double)par_p4 * 65536.0);
    var1 = (((double)par_p3 * var1 * var1) / 16384.0 +
            ((double)par_p2 * var1)) / 524288.0;
    var1 = (1.0 + (var1 / 32768.0)) * (double)par_p1;

    double press_comp = 1048576.0 - (double)press_adc;
    press_comp = ((press_comp - (var2 / 4096.0)) * 6250.0) / var1;
    var1 = ((double)par_p9 * press_comp * press_comp) / 2147483648.0;
    var2 = press_comp * ((double)par_p8 / 32768.0);
    double var3 = (press_comp / 256.0) * (press_comp / 256.0) *
                (press_comp / 256.0) * ((double)par_p10 / 131072.0);
    press_comp = press_comp + (var1 + var2 + var3 +
                ((double)par_p7 * 128.0)) / 16.0;
    
    return press_comp;
}