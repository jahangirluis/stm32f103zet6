#include <drv_cfg.h>
#include <device.h>
#include <os_errno.h>
#include <os_clock.h>
#include <stdlib.h>
#include <timer/clocksource.h>
#include <timer/clockevent.h>
#include <shell.h>
#include <dlog.h>
#include <drv_gpio.h>

#define DBG_TAG "clockevent_test"

static void timeout_callback(os_clockevent_t *ce)
{
   LOG_I(DBG_TAG, "clockevent timeout callback");
}

void timer_demo(void)
{
    int i,one_shot_mode=0;

    os_clockevent_t *ce;

    ce = (os_clockevent_t *)os_device_find("tim8");
    if (ce == OS_NULL)
    {
        LOG_E(DBG_TAG, "invalid clockevent device");
        return;
    }

    os_device_open((os_device_t *)ce);
    os_pin_mode(key_table[0].pin, key_table[0].mode);  //使用KEY0切换模式，ONESHOT或PERIOD

    os_clockevent_register_isr(ce, timeout_callback);

    /* sync tick */
    os_task_msleep(1);

    while(1)
    {
        if (!os_pin_read(key_table[0].pin))
        {
            one_shot_mode = !one_shot_mode;
            if (one_shot_mode)
            {
                //one shot mode
                LOG_I(DBG_TAG, "clockevent oneshot start...");
                os_clockevent_start_oneshot(ce, NSEC_PER_SEC);
                os_task_msleep(2000);
                os_clockevent_stop(ce);
                LOG_I(DBG_TAG, "clockevent onshot stop.");
            }
            else
            {
                /* 2.period mode */
                LOG_I(DBG_TAG, "clockevent period start...");
                os_clockevent_start_period(ce, NSEC_PER_SEC);
                os_task_msleep(3300);
                os_clockevent_stop(ce);
                LOG_I(DBG_TAG, "clockevent period sstop.");
            }
        }
        os_task_msleep(50);
    }
}

