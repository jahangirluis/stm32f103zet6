#include <board.h>
#include <string.h>

void uart_demo(void)
{
    int            ret, i, loops = 20;
    uint32_t       tx_cnt,rx_cnt;
    //unsigned char tx_buff[]="hello world\r\n";
    unsigned char rx_buff[OS_SERIAL_RX_BUFSZ];
    os_device_t   *uart;

    uart = os_device_find("uart4");
    OS_ASSERT(uart);

    os_device_open(uart);

    /* uart config */
    struct serial_configure config = OS_SERIAL_CONFIG_DEFAULT;
    config.baud_rate = BAUD_RATE_115200;
    ret = os_device_control(uart, OS_DEVICE_CTRL_CONFIG, &config);
    if (ret != 0)
    {
        return;
    }

    while(1)
    {
        i = 0;
        do
        {
            rx_cnt = os_device_read_block(uart, 0, &rx_buff[i], OS_SERIAL_RX_BUFSZ-i);
            if (rx_cnt <= 0)
            {
                os_kprintf("rx error: %s\r\n");
            }
            else {
                os_kprintf("rx char : %d\r\n",rx_buff[i]);
            }
            i+=rx_cnt;
        }
        while(rx_buff[i-1]!='\r');
        rx_buff[i]='\n';
        rx_buff[i+1]='\0';
        os_kprintf("-------rx:%s\n", rx_buff);

        os_task_msleep(30);   /*wait a while for recv*/

        tx_cnt = os_device_write_block(uart, 0, rx_buff, strlen(rx_buff));
        if (tx_cnt<=0) {
            os_kprintf("%d:tx error\r\n",i);
            continue;
        }
        else {
            os_kprintf("%d:tx success\r\n",i);
        }

        os_task_msleep(10);
    }
    os_device_close(uart);
}
