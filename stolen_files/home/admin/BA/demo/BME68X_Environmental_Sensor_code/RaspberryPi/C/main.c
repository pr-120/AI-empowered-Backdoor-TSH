/*
  Comple：make
  Run: ./bme68x
  
  This Demo is tested on Raspberry PI 3B+
  you can use I2C or SPI interface to test this Demo
  When you use I2C interface,the default Address in this demo is 0X77
  When you use SPI interface,PIN 27 define SPI_CS
*/
#include "bme68x.h"
#include <stdio.h>
#include <unistd.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>

//Raspberry 3B+ platform's default SPI channel
#define channel 0  

//Default write it to the register in one time
#define USESPISINGLEREADWRITE 0 

//This definition you use I2C or SPI to drive the bme68x
//When it is 1 means use I2C interface, When it is 0,use SPI interface
#define USEIIC 1

#define BME68X_VALID_DATA  UINT8_C(0xB0)


#if(USEIIC)
#include <string.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
//Raspberry 3B+ platform's default I2C device file
#define IIC_Dev  "/dev/i2c-1"

	
int fd;

void user_delay_us(uint32_t period, void *intf_ptr)
{
  usleep(period);
}

int8_t user_i2c_read(uint8_t reg_addr, uint8_t *data, uint32_t len,void *intf_ptr)
{
  write(fd, &reg_addr,1);
  read(fd, data, len);
  return 0;
}

int8_t user_i2c_write(uint8_t reg_addr, const uint8_t *data, uint32_t len,void *intf_ptr)
{
  int8_t *buf;
  buf = malloc(len +1);
  buf[0] = reg_addr;
  memcpy(buf +1, data, len);
  write(fd, buf, len +1);
  free(buf);
  return 0;
}
#else

void SPI_BME68X_CS_High(void)
{
	digitalWrite(27,1);
}

void SPI_BME68X_CS_Low(void)
{
	digitalWrite(27,0);
}

void user_delay_us(uint32_t period, void *intf_ptr)
{
  usleep(period);
}

int8_t user_spi_read(uint8_t reg_addr, uint8_t *data, uint32_t len,void *intf_ptr)
{
	int8_t rslt = 0;
	
	SPI_BME68X_CS_High();
	SPI_BME68X_CS_Low();
	
	wiringPiSPIDataRW(channel,&reg_addr,1);

	#if(USESPISINGLEREADWRITE)
    for(int i=0; i < len ; i++)
	{
	  wiringPiSPIDataRW(channel,data,1);
	  data++;
	}
	#else
	wiringPiSPIDataRW(channel,data,len);
	#endif
	
	SPI_BME68X_CS_High();
	
	return rslt;
}

int8_t user_spi_write(uint8_t reg_addr, const uint8_t *data, uint32_t len,void *intf_ptr)
{
	int8_t rslt = 0;

	SPI_BME68X_CS_High();
	SPI_BME68X_CS_Low();

	wiringPiSPIDataRW(channel,&reg_addr,1);
	
	#if(USESPISINGLEREADWRITE)
	for(int i = 0; i < len ; i++)
	{
		wiringPiSPIDataRW(channel,data,1);
		data++;
	}
	#else
	wiringPiSPIDataRW(channel,data,len);
	#endif
	
	SPI_BME68X_CS_High();
	
	return rslt;
}
#endif

void print_sensor_data(struct bme68x_data *comp_data)
{
#ifdef BME68X_USE_FPU
	printf("temperature:%0.2f*C   pressure:%0.2fhPa   humidity:%0.2f%%   Gas resistance:%0.2f ohm\r\n",comp_data->temperature, comp_data->pressure/100, comp_data->humidity, comp_data->gas_resistance         );
    printf("\r\b\r");
#else
	printf("temperature:%ld*C   pressure:%ldhPa   humidity:%ld%%   Gas resistance:%lu ohm\r\n",comp_data->temperature, comp_data->pressure/100, comp_data->humidity, comp_data->gas_resistance         );
    printf("\r\b\r");
#endif
}

int8_t stream_sensor_data_forced_mode(struct bme68x_dev *dev)
{
    int8_t rslt;
	uint8_t n_fields;
    uint32_t del_period;
	struct bme68x_conf conf;
    struct bme68x_heatr_conf heatr_conf;
    struct bme68x_data comp_data;

    /* Recommended mode of operation: Indoor navigation */
    conf.os_hum = BME68X_OS_1X;
    conf.os_pres = BME68X_OS_16X;
    conf.os_temp = BME68X_OS_2X;
    conf.filter = BME68X_FILTER_SIZE_15;

    rslt = bme68x_set_conf(&conf, dev);
    dev->delay_us(40*1000,dev->intf_ptr);
    
    heatr_conf.enable = BME68X_ENABLE;
    heatr_conf.heatr_temp = 300;
    heatr_conf.heatr_dur = 100;
    
    rslt = bme68x_set_heatr_conf(BME68X_FORCED_MODE, &heatr_conf, dev);
    dev->delay_us(40*1000,dev->intf_ptr);
    
    printf("Temperature           Pressure             Humidity             Gas resistance\r\n");
    /* Continuously stream sensor data */
    while (1) {
        rslt = bme68x_set_op_mode(BME68X_FORCED_MODE, dev);
        /* Wait for the measurement to complete and print data @25Hz */
        del_period = bme68x_get_meas_dur(BME68X_FORCED_MODE, &conf, dev) + (heatr_conf.heatr_dur * 1000);
        dev->delay_us(del_period*5, dev->intf_ptr);
        rslt = bme68x_get_data(BME68X_FORCED_MODE, &comp_data, &n_fields, dev);
        if(n_fields)
        {
            print_sensor_data(&comp_data);
        }
    }
    return rslt;
}


int8_t stream_sensor_data_parallel_mode(struct bme68x_dev *dev)
{
	int8_t rslt;
    uint8_t n_fields;
	struct bme68x_conf conf;
    struct bme68x_heatr_conf heatr_conf;
	struct bme68x_data comp_data[3];
    
    uint16_t temp_prof[10] = { 320, 100, 100, 100, 200, 200, 200, 320, 320, 320 };
    uint16_t mul_prof[10] = { 5, 2, 10, 30, 5, 5, 5, 5, 5, 5 };

	/* Recommended mode of operation: Indoor navigation */
	conf.os_hum = BME68X_OS_1X;
    conf.os_pres = BME68X_OS_16X;
    conf.os_temp = BME68X_OS_2X;
    conf.filter = BME68X_FILTER_SIZE_15;
	conf.odr = BME68X_ODR_62_5_MS;

	rslt = bme68x_set_conf(&conf, dev);
    dev->delay_us(40*1000,dev->intf_ptr);
    
    heatr_conf.enable = BME68X_ENABLE;
    heatr_conf.heatr_temp_prof = temp_prof;
    heatr_conf.heatr_dur_prof = mul_prof;

    heatr_conf.shared_heatr_dur = 140 - (bme68x_get_meas_dur(BME68X_PARALLEL_MODE, &conf, dev) / 1000);

    heatr_conf.profile_len = 10;
    
    rslt = bme68x_set_heatr_conf(BME68X_PARALLEL_MODE, &heatr_conf, dev);
    dev->delay_us(40*1000,dev->intf_ptr);
    
    rslt = bme68x_set_op_mode(BME68X_PARALLEL_MODE, dev);
    dev->delay_us(40*1000,dev->intf_ptr);

	printf("Temperature           Pressure             Humidity             Gas resistance\r\n");
	while (1) {
		/* Delay while the sensor completes a measurement */
		dev->delay_us(70*1000,dev->intf_ptr);
        rslt = bme68x_get_data(BME68X_FORCED_MODE, comp_data, &n_fields, dev);
        for (uint8_t i = 0; i < n_fields; i++)
        {
            if (comp_data[i].status == BME68X_VALID_DATA)
            {
                print_sensor_data(&comp_data[i]);
            }
        }
	}

	return rslt;
}

#if(USEIIC)
int main(int argc, char* argv[])
{
  struct bme68x_dev dev;
  static uint8_t dev_addr=BME68X_I2C_ADDR_HIGH;
  int8_t rslt = BME68X_OK;

  if ((fd = open(IIC_Dev, O_RDWR)) < 0) {
    printf("Failed to open the i2c bus %s", argv[1]);
    exit(1);
  }
  if (ioctl(fd, I2C_SLAVE, 0x77) < 0) {
    printf("Failed to acquire bus access and/or talk to slave.\n");
    exit(1);
  }
  //dev.dev_id = BME68X_I2C_ADDR_PRIM;//0x76
  dev.intf_ptr = &dev_addr; //0x77
  dev.intf = BME68X_I2C_INTF;
  dev.read = user_i2c_read;
  dev.write = user_i2c_write;
  dev.delay_us = user_delay_us;

  rslt = bme68x_init(&dev);
  printf("\r\n BME68X Init Result is:%d \r\n",rslt);
  stream_sensor_data_forced_mode(&dev);
  //stream_sensor_data_parallel_mode(&dev);
}
#else
int main(int argc, char* argv[])
{
  if(wiringPiSetup() < 0)
  {
    return 1;
  }
  
  pinMode (27,OUTPUT) ;
  
  SPI_BME68X_CS_Low();//once pull down means use SPI Interface
  
  wiringPiSPISetup(channel,2000000);

  struct bme68x_dev dev;
  int8_t rslt = BME68X_OK;

  dev.intf_ptr = 0;
  dev.intf = BME68X_SPI_INTF;
  dev.read = user_spi_read;
  dev.write = user_spi_write;
  dev.delay_us = user_delay_us;

  rslt = bme68x_init(&dev);
  printf("\r\n BME68X Init Result is:%d \r\n",rslt);
  stream_sensor_data_forced_mode(&dev);
  //stream_sensor_data_normal_mode(&dev);
}
#endif
