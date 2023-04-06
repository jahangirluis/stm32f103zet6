#include <board.h>
#include <os_task.h>
#include <drv_gpio.h>

#if 0
//直接用HAL库

#define delay_ms os_task_msleep
//下面的方式是通过直接操作HAL库函数方式读取IO
#define KEY0        HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_4)  //KEY0按键PE4
#define KEY1        HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_3)  //KEY1按键PE3
#define WK_UP       HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0)  //WKUP按键PA0

#define KEY0_PRES   1
#define KEY1_PRES   2
#define WKUP_PRES   3

//按键处理函数
//返回按键值
//mode:0,不支持连续按;1,支持连续按;
//0，没有任何按键按下
//1，WKUP按下 WK_UP
//注意此函数有响应优先级,KEY0>KEY1>WK_UP!!
os_uint8_t KEY_Scan(os_uint8_t mode)
{
    static os_uint8_t key_up=1;     //按键松开标志
    if(mode==1)key_up=1;    //支持连按
    if(key_up&&(KEY0==0||KEY1==0||WK_UP==1))
    {
        delay_ms(10);
        key_up=0;
        if(KEY0==0)       return KEY0_PRES;
        else if(KEY1==0)  return KEY1_PRES;
        else if(WK_UP==1) return WKUP_PRES;
    }else if(KEY0==1&&KEY1==1&&WK_UP==0)key_up=1;
    return 0;   //无按键按下
}

void key_demo(void)
{
    os_uint8_t key,led0,led1;
    GPIO_InitTypeDef GPIO_Initure;

    //LED初始化
    GPIO_Initure.Pin=GPIO_PIN_5;                //PB5
    GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;      //推挽输出
    GPIO_Initure.Pull=GPIO_PULLUP;              //上拉
    GPIO_Initure.Speed=GPIO_SPEED_FREQ_HIGH;    //高速
    HAL_GPIO_Init(GPIOB,&GPIO_Initure);

    GPIO_Initure.Pin=GPIO_PIN_5;                //PE5
    HAL_GPIO_Init(GPIOE,&GPIO_Initure);

    led0 = GPIO_PIN_RESET;
    led1 = GPIO_PIN_RESET;


    //按键初始化
    GPIO_Initure.Pin=GPIO_PIN_0;            //PA0
    GPIO_Initure.Mode=GPIO_MODE_INPUT;      //输入
    GPIO_Initure.Pull=GPIO_PULLDOWN;        //下拉
    GPIO_Initure.Speed=GPIO_SPEED_FREQ_HIGH;//高速
    HAL_GPIO_Init(GPIOA,&GPIO_Initure);

    GPIO_Initure.Pin=GPIO_PIN_3|GPIO_PIN_4; //PE3,4
    GPIO_Initure.Mode=GPIO_MODE_INPUT;      //输入
    GPIO_Initure.Pull=GPIO_PULLUP;          //上拉
    GPIO_Initure.Speed=GPIO_SPEED_FREQ_HIGH;//高速
    HAL_GPIO_Init(GPIOE,&GPIO_Initure);

    while(1)
    {
        key=KEY_Scan(0);            //按键扫描
        switch(key)
        {
            case KEY0_PRES:
                led0 = !led0;
                HAL_GPIO_WritePin(GPIOB,GPIO_PIN_5,led0);     //LED0
                //LED0=!LED0;
                break;
            case KEY1_PRES:
                led1 = !led1;
                HAL_GPIO_WritePin(GPIOE,GPIO_PIN_5,led1);     //LED1
                //LED1=!LED1;
                break;
            case WKUP_PRES:
                led0 = !led0;
                HAL_GPIO_WritePin(GPIOB,GPIO_PIN_5,led0);     //LED0
                led1 = !led1;
                HAL_GPIO_WritePin(GPIOE,GPIO_PIN_5,led1);     //LED1
                //LED0=!LED0;
                //LED1=!LED1;
                break;
            default:
                delay_ms(10);
        }
        delay_ms(10);
    }
}
#endif

#if 1
//使用OneOS设备管理
//#define KEYWORK_IRQ_MODE

static void key_callback(int key_pin)
{
    os_kprintf("----------------------pin_callback:%d\r\n", key_pin);
    switch(key_pin)
    {
        case GET_PIN(E, 4):
            os_pin_write(led_table[0].pin,!os_pin_read(led_table[0].pin));
            break;
        case GET_PIN(E, 3):
            os_pin_write(led_table[1].pin,!os_pin_read(led_table[1].pin));
            break;
        case GET_PIN(A, 0):
            os_pin_write(led_table[0].pin,!os_pin_read(led_table[0].pin));
            os_pin_write(led_table[1].pin,!os_pin_read(led_table[1].pin));
            break;
    }
}

#ifdef KEYWORK_IRQ_MODE
/*中断模式*/
static void pin_callback(void *args)
{
    key_callback((int)(unsigned long)args);
}

void key_demo(void)
{
    int i, j;

    for (i = 0; i < led_table_size; i++)
    {
        os_pin_mode(led_table[i].pin, PIN_MODE_OUTPUT);
    }

    for (i = 0; i < key_table_size; i++)
    {
        os_pin_mode(key_table[i].pin, key_table[i].mode);
        /*中断模式设置 */
        os_pin_attach_irq(key_table[i].pin, key_table[i].irq_mode, pin_callback, (void *)key_table[i].pin);
        os_pin_irq_enable(key_table[i].pin, PIN_IRQ_ENABLE);
    }

    while(1)
    {
        os_task_msleep(200);
    }
/*
    for (i = 0; i < key_table_size; i++)
    {
        os_pin_mode(key_table[i].pin, PIN_MODE_DISABLE);
        os_kprintf("disable <%u> pin[%d] : %d\r\n",
                   (unsigned int)os_tick_get_value(),
                   key_table[i].pin,
                   os_pin_read(key_table[i].pin));
        os_task_msleep(1000);
    }
*/
}
#else
/*l轮询模式*/
    void key_demo(void)
    {
        int i, val;

        for (i = 0; i < led_table_size; i++)
        {
            os_pin_mode(led_table[i].pin, PIN_MODE_OUTPUT);
        }

        for (i = 0; i < key_table_size; i++)
        {
            os_pin_mode(key_table[i].pin, key_table[i].mode);
        }

        while(1)
        {
            for (i = 0; i < key_table_size; i++)
            {
               val = os_pin_read(key_table[i].pin);
               switch(key_table[i].mode)
               {
                   case PIN_MODE_INPUT_PULLUP:
                       if(!val) key_callback(key_table[i].pin);
                       break;
                   case PIN_MODE_INPUT_PULLDOWN:
                       if(val) key_callback(key_table[i].pin);
                       break;
               }
            }
            os_task_msleep(10);
        }
    /*
        for (i = 0; i < key_table_size; i++)
        {
            os_pin_mode(key_table[i].pin, PIN_MODE_DISABLE);
            os_kprintf("disable <%u> pin[%d] : %d\r\n",
                       (unsigned int)os_tick_get_value(),
                       key_table[i].pin,
                       os_pin_read(key_table[i].pin));
            os_task_msleep(1000);
        }
    */
    }
#endif
#endif

