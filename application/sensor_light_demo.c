#include <drv_cfg.h>
#include <os_clock.h>
#include <stdio.h>
#include <shell.h>
#include <sensor.h>

void sensor_light_demo(void)
{
    struct os_sensor_data sensor_data;
    os_device_t *sensor = os_device_find("li_photo_diode");
    OS_ASSERT(sensor != NULL);
    os_device_open(sensor);

    struct os_sensor_info sensor_info;
    os_device_control(sensor, OS_SENSOR_CTRL_GET_INFO, &sensor_info);

    while(1)
    {
        os_device_read_nonblock(sensor, 0, &sensor_data, sizeof(struct os_sensor_data));

        if (sensor_info.unit == OS_SENSOR_UNIT_MLUX)
        {
            os_kprintf("sensor light (%d.%03d)\r\n", sensor_data.data.light / 1000, sensor_data.data.light % 1000);
        }

        else if (sensor_info.unit == OS_SENSOR_UNIT_LUX)
        {
            os_kprintf("sensor light (%d)\r\n", sensor_data.data.light);
        }
        else if (sensor_info.unit == OS_SENSOR_UNIT_RAW)
        {
            os_kprintf("sensor light raw(%d) mV\r\n", sensor_data.data.light);
        }
        else
        {
            os_kprintf("invalid unit\r\n");
        }
        os_task_msleep(1000);
    }
}
