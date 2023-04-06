#include <device.h>
#include <unistd.h>
#include <drv_log.h>
#include <drv_cfg.h>

/*
*   adc and dac should config one channel,and connet together;
*   adc1:IN1,PA1 ADC3:IN6,PF8 DAC:OUT1,PA4
*   需要用杜邦线连接ADC和DAC的引脚
*/
void adc_dac_demo(void)
{
    int   adc_channel;
    int   dac_channel;
    int   voltage_output = 0;
    int   voltage_input = 0;
    char *adc_name;
    char *dac_name;
    os_err_t ret = OS_EOK;
    os_device_t *adc;
    os_device_t *dac;

    adc_name = "adc1";
    dac_name = "dac";
    adc_channel = 1;
    dac_channel = 1;

    adc = os_device_find(adc_name);
    if (adc == OS_NULL)
    {
        os_kprintf("invalid adc device %s.\r\n", adc_name);
        return;
    }

    os_device_open(adc);

    dac = os_device_find(dac_name);
    if (dac == OS_NULL)
    {
        os_device_close(adc);
        os_kprintf("invalid dac device %s.\r\n", dac_name);
        return;
    }

    os_device_open(dac);

    os_device_control(adc, OS_ADC_CMD_ENABLE, OS_NULL);
    os_device_control(dac, OS_DAC_CMD_ENABLE, (void *)dac_channel);

    while (1)
    {
        os_device_write_nonblock(dac, dac_channel, &voltage_output, sizeof(voltage_output));
        os_task_msleep(10);
        ret = os_device_read_nonblock(adc, adc_channel, &voltage_input, sizeof(voltage_input));
        os_kprintf("dac output %d mv. adc input %d mv\r\n", voltage_output, voltage_input);
        voltage_output += 550;
        if(voltage_output >3300)
            voltage_output = 0;

        os_task_msleep(400);
    }
    os_device_close(adc);
    os_device_close(dac);
}
