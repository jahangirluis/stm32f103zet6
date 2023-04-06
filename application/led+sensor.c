/*
 * Copyright (c) 2006-2020, OneOS Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-03-30     Jahangir       the first version
 */

#include <drv_cfg.h>
#include <os_clock.h>
#include <stdio.h>
#include <shell.h>
#include <sensor.h>
#include <board.h>
#include <os_task.h>

int sleep_time;

void led_sensor(void)
{
    struct os_sensor_data sensor_data;
    os_device_t *sensor = os_device_find("li_photo_diode");
    OS_ASSERT(sensor != NULL);
    os_device_open(sensor);

    struct os_sensor_info sensor_info;
    os_device_control(sensor, OS_SENSOR_CTRL_GET_INFO, &sensor_info);

    int i;
    for (i = 0; i < led_table_size; i++)
    {
        os_pin_mode(led_table[i].pin, PIN_MODE_OUTPUT);
    }

    while(1)
    {
        os_device_read_nonblock(sensor, 0, &sensor_data, sizeof(struct os_sensor_data));
        int light_strengh=sensor_data.data.light;
        sleep_time=light_strengh;
        os_pin_write(led_table[0].pin, led_table[0].active_level);
        os_pin_write(led_table[1].pin, !led_table[1].active_level);
        os_task_msleep(sleep_time);
        os_pin_write(led_table[0].pin, !led_table[0].active_level);
        os_pin_write(led_table[1].pin, led_table[1].active_level);
        os_task_msleep(sleep_time);
    }
}

void led_execute(void)
{


        while (1)
        {


        }
}
