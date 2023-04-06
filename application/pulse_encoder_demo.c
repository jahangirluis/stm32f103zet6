#include <device.h>
#include "drv_gpio.h"
#include <os_errno.h>

void pulse_encoder_demo(void)
{
    os_err_t     ret  = OS_EOK;
    os_int32_t count = 0;

    char *dev_name="encoder_tim3";
    os_pulse_encoder_device_t *pulse_encoder_dev = OS_NULL;

    pulse_encoder_dev = (os_pulse_encoder_device_t *)os_device_find(dev_name);
    if (pulse_encoder_dev == OS_NULL)
    {
        os_kprintf("pulse encoder sample run failed! can't find %s device!\r\n", dev_name);
        return;
    }

    ret = os_pulse_encoder_enable(pulse_encoder_dev);

    for (int i = 0; i <= 100; i++)
    {
        os_task_msleep(500);
        ret = os_pulse_encoder_read(pulse_encoder_dev, &count);
        os_kprintf("get count %d\r\n", count);
    }

}
