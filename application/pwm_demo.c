#include <drv_cfg.h>
#include <device.h>
#include <os_clock.h>
#include <os_memory.h>
#include <stdlib.h>
#include <shell.h>
#include <timer/clocksource.h>
#include <drv_gpio.h>

//此示例需用杜邦线连接PB8和PA7来测试中断产生
//同时因为这里的PWM_TIM4 channel 3直接设置为PB8输出，因而蜂鸣器会同时响起，且PWM占空比导致不同的声音效果
//另外，直接连PB5可亮LED0,连PE5可亮LED1

static void pwm_pin_callback(void *args)
{
    int pin = (int)(unsigned long)args;

    os_kprintf("<%d>----------------------pin:%d value:%d\r\n", (int)os_tick_get(), pin, os_pin_read(pin));
}

void pwm_demo(void)
{
    uint32_t pin;
    GPIO_InitTypeDef GPIO_Initure;
    int i;

    char *dev_name="pwm_tim4";

    os_device_t *pwm_dev = OS_NULL;

    struct os_pwm_configuration *config;

    config = os_calloc(1, sizeof(struct os_pwm_configuration));

    pin = GET_PIN(B, 5);
    os_pin_mode(pin, PIN_MODE_INPUT_PULLUP);
    os_pin_attach_irq(pin, PIN_IRQ_MODE_RISING_FALLING, pwm_pin_callback, (void *)pin);
    os_pin_irq_enable(pin, PIN_IRQ_ENABLE);

    pwm_dev = os_device_find(dev_name);
    if (pwm_dev == OS_NULL)
    {
        os_kprintf("pwm sample run failed! can't find %s device!\r\n", dev_name);
        return;
    }

    config->channel = 3;
    config->period = 5000000;
    i = 1000000;

    os_device_control(pwm_dev, OS_PWM_CMD_SET_PERIOD, config);
    os_device_control(pwm_dev, OS_PWM_CMD_ENABLE, config);

    while(1)
    {
        if (i>config->period)  i = 1000000;

        config->pulse = i;
        os_device_control(pwm_dev, OS_PWM_CMD_SET_PULSE, config);
        os_task_msleep(1000);
        i = i+1000000;
    }
}
