#include <board.h>
#include <os_task.h>

#if 0
//直接用HAL库
#define delay_ms os_task_msleep
void beep_demo(void)
{
    GPIO_InitTypeDef GPIO_Initure;

    //蜂鸣器初始化
    GPIO_Initure.Pin=GPIO_PIN_8;            //PB8
    GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;  //推挽输出
    GPIO_Initure.Pull=GPIO_PULLUP;          //上拉
    GPIO_Initure.Speed=GPIO_SPEED_FREQ_HIGH;//高速
    HAL_GPIO_Init(GPIOB,&GPIO_Initure);
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_RESET); //蜂鸣器对应引脚GPIOB8拉低

    //LED初始化
    GPIO_Initure.Pin=GPIO_PIN_5;                //PB5
    GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;      //推挽输出
    GPIO_Initure.Pull=GPIO_PULLUP;              //上拉
    GPIO_Initure.Speed=GPIO_SPEED_FREQ_HIGH;    //高速
    HAL_GPIO_Init(GPIOB,&GPIO_Initure);

    GPIO_Initure.Pin=GPIO_PIN_5;                //PE5
    HAL_GPIO_Init(GPIOE,&GPIO_Initure);

    while(1)
    {
        HAL_GPIO_WritePin(GPIOB,GPIO_PIN_5,GPIO_PIN_RESET);     //DS0拉低，亮   等同LED0=0;
        HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_SET);       //BEEP引脚拉低， 等同BEEP=0;
        HAL_GPIO_WritePin(GPIOE,GPIO_PIN_5,GPIO_PIN_SET);       //LED1对应引脚PE5拉高，灭，等同于LED1(1)
        delay_ms(300);                                          //延时300ms
        HAL_GPIO_WritePin(GPIOB,GPIO_PIN_5,GPIO_PIN_SET);       //DS0拉高，灭   等同LED0=1;
        HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_RESET);     //BEEP引脚拉高， 等同BEEP=1;
        HAL_GPIO_WritePin(GPIOE,GPIO_PIN_5,GPIO_PIN_RESET);       //LED1对应引脚PE5拉低，亮，等同于LED1(0)
        delay_ms(300);                                          //延时300ms                                         //延时500ms
        os_kprintf("running \r\n");
    }
}
#endif

#if 1
//使用OneOS设备管理
void beep_demo(void)
{
    int i = 0;

    for (i = 0; i < buzzer_table_size; i++)
    {
        os_pin_mode(buzzer_table[i].pin, PIN_MODE_OUTPUT);
    }

    for (i = 0; i < led_table_size; i++)
    {
        os_pin_mode(led_table[i].pin, PIN_MODE_OUTPUT);
    }


    while (1)
    {
        os_pin_write(led_table[0].pin, led_table[0].active_level);
        os_pin_write(led_table[1].pin, !led_table[1].active_level);
        os_pin_write(buzzer_table[0].pin, !buzzer_table[0].active_level);
        os_task_msleep(300);
        os_pin_write(led_table[0].pin, !led_table[0].active_level);
        os_pin_write(led_table[1].pin, led_table[1].active_level);
        os_pin_write(buzzer_table[0].pin, buzzer_table[0].active_level);
        os_task_msleep(300);

        os_kprintf("running \r\n");
    }
}
#endif
