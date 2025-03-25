#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
//
#include <zephyr/bluetooth/conn.h>
#include <bluetooth/services/nsms.h>
//
#include "sensor_data_collector.h"

#define SENSOR_THREAD_PRIORITY 7
#define SENSOR_THREAD_STACK_SIZE 1024

#define BUF_SIZE		64

K_MSGQ_DEFINE(sensor_readings_queue, sizeof(sensorsreadings), 16, 4);

BT_NSMS_DEF(nsms_env, "Environmental", false, "Unknown", BUF_SIZE);

static struct bt_conn *current_conn = NULL;

static void connected(struct bt_conn *conn, uint8_t err)
{
	current_conn = bt_conn_ref(conn);
	printk("connected in sensor_data_collector...\n");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (current_conn) {
		bt_conn_unref(current_conn);
		current_conn = NULL;
	}
	printk("disconnected in sensor_data_collector...\n");
}

BT_CONN_CB_DEFINE(conn_callbacks1) = {
	.connected = connected,
	.disconnected = disconnected,

};

static bool send_sensor_value(const struct sensor_value *val, size_t size, const char *channel)
{
	float float_data[size];
	char buf[BUF_SIZE];

	int len = sprintf(buf, "%s ", channel);

	for (size_t i = 0; i < size; i++) {
		float_data[i] = sensor_value_to_float(&val[i]);

		len += sprintf(buf + len, "%.6f ", (double)float_data[i]);
	}

	bt_nsms_set_status(&nsms_env, buf);
	
	return true;
}

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

		printk("Sensor Thread Reporting!\n");		
		printk("T: %d.%06d; P: %d.%06d; H: %d.%06d\n\n", value.temp.val1, value.temp.val2,
		       value.press.val1, value.press.val2, value.humidity.val1,
		       value.humidity.val2);

		if (current_conn != NULL) {
			send_sensor_value(&value.temp, 1, "temp");
			send_sensor_value(&value.press, 1, "press");
			send_sensor_value(&value.humidity, 1, "humidity");		
		}

		int ret = k_msgq_put(&sensor_readings_queue, &value, K_NO_WAIT);
		if (ret) {
			printk("Failed to put data into message queue\n");
		}

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

		if (current_conn != NULL) {
			send_sensor_value(&value.temp, 1, "temp");
			send_sensor_value(&value.press, 1, "press");
			send_sensor_value(&value.humidity, 1, "humidity");	
		}

		int ret = k_msgq_put(&sensor_readings_queue, &value, K_NO_WAIT);
		if (ret) {
			printk("Failed to put data into message queue\n");
		}

		k_sleep(K_SECONDS(CONFIG_APP_CONTROL_SAMPLING_INTERVAL_S));
	}

#endif
}

struct k_msgq *get_sensor_readings_queue(void)
{
	return &sensor_readings_queue;
}

K_THREAD_DEFINE(sensor_data_collector_id, SENSOR_THREAD_STACK_SIZE, sensor_data_collector, NULL,
    NULL, NULL, SENSOR_THREAD_PRIORITY, 0, 1000);