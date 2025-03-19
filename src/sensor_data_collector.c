#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

#include "sensor_data_collector.h"

#define SENSOR_THREAD_PRIORITY 7
#define SENSOR_THREAD_STACK_SIZE 1024

#ifdef CONFIG_SIMULATED_SENSOR
static const struct device *get_simulated_sensor(void)
{
    const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(sensor_sim));

    if (dev == NULL)
    {
        printk("\nError : no device found.\n");
        return NULL;
    }

    if (!device_is_ready(dev))
    {
        printk("\nError : Device \"%s\" is not ready.\n", dev->name);            
        return NULL;
    }

    printk("\nDevice \"%s\" is ready.\n", dev->name);

    return dev;
}
#else
static const struct device *get_bme280_sensor(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(bme280));
	//const struct device *const dev = DEVICE_DT_GET_ANY(bosch_bme280);

	if (dev == NULL) {
		/* No such node, or the node does not have status "okay". */
		printk("\nError: no device found.\n");
		return NULL;
	}

	if (!device_is_ready(dev)) {
		printk("\nError: Device \"%s\" is not ready; "
		       "check the driver initialization logs for errors.\n",
		       dev->name);
		return NULL;
	}

	printk("Found device \"%s\", getting sensor data\n", dev->name);
	return dev;
}


#endif

int sensor_data_collector(void)
{
#ifdef CONFIG_SIMULATED_SENSOR    

    const struct device *sim_dev = get_simulated_sensor();

	if (sim_dev == NULL) {
		return 0;
	}

	while (1) {
		sensorsreadings value;
		sensor_sample_fetch(sim_dev);
		// Simulated sensor
		sensor_channel_get(sim_dev, SENSOR_CHAN_AMBIENT_TEMP, &value.temp);
		sensor_channel_get(sim_dev, SENSOR_CHAN_PRESS, &value.press);
		sensor_channel_get(sim_dev, SENSOR_CHAN_HUMIDITY, &value.humidity);
		sensor_channel_get(sim_dev, SENSOR_CHAN_ACCEL_XYZ, value.acc);
        sensor_channel_get(sim_dev, SENSOR_CHAN_GAS_RES, &value.gas_res);

		printk("Sensor Thread Reporting!\n");
		printk("AX: %d.%06d; AY: %d.%06d; AZ: %d.%06d\n", value.acc[0].val1,
		       value.acc[0].val2, value.acc[1].val1, value.acc[1].val2, value.acc[2].val1,
		       value.acc[2].val2);
		printk("T: %d.%06d; P: %d.%06d; H: %d.%06d, G: %d.%06d\n\n", value.temp.val1, value.temp.val2,
		       value.press.val1, value.press.val2, value.humidity.val1,
		       value.humidity.val2,value.gas_res);
								   
		k_sleep(K_SECONDS(CONFIG_APP_CONTROL_SAMPLING_INTERVAL_S));
	}
#else
const struct device *dev_bme280 = get_bme280_sensor();
	if (dev_bme280 == NULL) {
		return 0;
	}
	
	while (1) {
		sensorsreadings value;
		sensor_sample_fetch(dev_bme280);
		
		sensor_channel_get(dev_bme280, SENSOR_CHAN_AMBIENT_TEMP, &value.temp);
		sensor_channel_get(dev_bme280, SENSOR_CHAN_PRESS, &value.press);
		sensor_channel_get(dev_bme280, SENSOR_CHAN_HUMIDITY, &value.humidity);
				

		printk("Sensor Thread Reporting!\n");		
		printk("T: %d.%06d; P: %d.%06d; H: %d.%06d\n", value.temp.val1,
		       value.temp.val2, value.press.val1, value.press.val2, value.humidity.val1,
		       value.humidity.val2);
				
		k_sleep(K_SECONDS(CONFIG_APP_CONTROL_SAMPLING_INTERVAL_S));
	}

#endif
}


K_THREAD_DEFINE(sensor_data_collector_id, SENSOR_THREAD_STACK_SIZE, sensor_data_collector, NULL,
    NULL, NULL, SENSOR_THREAD_PRIORITY, 0, 1000);