#include <board.h>
#include <os_task.h>

#if 0
//直接用HAL库
#define delay_ms os_task_msleep
void led_demo(void)
{
    GPIO_InitTypeDef GPIO_Initure;

    GPIO_Initure.Pin=GPIO_PIN_5;                //PB5
    GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;      //推挽输出
    GPIO_Initure.Pull=GPIO_PULLUP;              //上拉
    GPIO_Initure.Speed=GPIO_SPEED_FREQ_HIGH;    //高速
    HAL_GPIO_Init(GPIOB,&GPIO_Initure);

    GPIO_Initure.Pin=GPIO_PIN_5;                //PE5
    HAL_GPIO_Init(GPIOE,&GPIO_Initure);

    //while(1)
    //{
        HAL_GPIO_WritePin(GPIOB,GPIO_PIN_5,GPIO_PIN_SET);     //LED0对应引脚PB5拉低，亮，等同于LED0(0)
        /*
        HAL_GPIO_WritePin(GPIOE,GPIO_PIN_5,GPIO_PIN_SET);       //LED1对应引脚PE5拉高，灭，等同于LED1(1)
        delay_ms(500);                                          //延时500ms
        HAL_GPIO_WritePin(GPIOB,GPIO_PIN_5,GPIO_PIN_SET);       //LED0对应引脚PB5拉高，灭，等同于LED0(1)
        HAL_GPIO_WritePin(GPIOE,GPIO_PIN_5,GPIO_PIN_RESET);     //LED1对应引脚PE5拉低，亮，等同于LED1(0)
        delay_ms(500);                                          //延时500ms
        os_kprintf("running \r\n");
        */
    //}
}
#endif

#if 1
//使用OneOS设备管理
void led_demo(void)
{
    int i = 0;

    for (i = 0; i < led_table_size; i++)
    {
        os_pin_mode(led_table[i].pin, PIN_MODE_OUTPUT);
    }

    while (1)
    {
        os_pin_write(led_table[0].pin, led_table[0].active_level);

        os_pin_write(led_table[1].pin, !led_table[1].active_level);
        //os_task_msleep(10);
        os_pin_write(led_table[0].pin, !led_table[0].active_level);
        os_pin_write(led_table[1].pin, led_table[1].active_level);
        //os_task_msleep(10);
        os_kprintf("running \r\n");

    }
}
#endif
