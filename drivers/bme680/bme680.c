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
double calculate_temp(uint32_t temp_adc, temp_calib_data calib_data) {
    double var1 = (((double)temp_adc / 16384.0) - ((double)calib_data.par_t1 / 1024.0)) * (double)calib_data.par_t2;
    double var2 = ((((double)temp_adc / 131072.0) - ((double)calib_data.par_t1 / 8192.0)) *
                  (((double)temp_adc / 131072.0) - ((double)calib_data.par_t1 / 8192.0))) *
                  ((double)calib_data.par_t3 * 16.0);

    double t_fine = var1 + var2;
    double temp_comp = t_fine / 5120.0;

    return temp_comp;
}