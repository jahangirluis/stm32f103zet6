/*
 * Copyright (c) 2006-2020, OneOS Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-03-30     Jahangir       the first version
 */
#include <atk_tflcd9341.h>
#include <board.h>
#include <os_task.h>
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

void timer_end();

void timer(void)
{
    int h=0;
    int m=0;
    int s=10;

    while(1)
    {
        lcd_show_num(30,150,h,2,16);
        lcd_show_num(30,170,m,2,16);
        lcd_show_num(30,190,s,2,16);
        s--;
        if(s<0)
        {
            m--;
            s=59;
        }
        if(m<0)
        {
            if(h!=0)m=59,h--;
        }
        if(h==0 && m==0 && s==0)
        {
            timer_end();
        }
        os_task_msleep(1000);
    }

}

void timer_end()
{
    os_pin_write(buzzer_table[0].pin, buzzer_table[0].active_level);
}
