#include <board.h>
#include <os_task.h>
#include <drv_gpio.h>
#include <atk_tflcd9341.h>
#include <clocksource.h>

#if 1

#define delay_ms os_task_msleep
#define delay_us(u) os_clocksource_ndelay(u*NSEC_PER_USEC)

//IO方向设置
//#define DHT11_IO_IN()  {GPIOG->CRH&=0XFFFF0FFF;GPIOG->CRH|=8<<12;}
//#define DHT11_IO_OUT() {GPIOG->CRH&=0XFFFF0FFF;GPIOG->CRH|=3<<12;}

//IO操作函数
//#define DHT11_DQ_OUT    PGout(11)//数据端口 PG11
//#define DHT11_DQ_IN     PGin(11) //数据端口 PG11

os_uint8_t DHT11_Init(void);//初始化DHT11
os_uint8_t DHT11_Read_Data(os_uint8_t *temp,os_uint8_t *humi);//读取温湿度
os_uint8_t DHT11_Read_Byte(void);//读出一个字节
os_uint8_t DHT11_Read_Bit(void);//读出一个位
os_uint8_t DHT11_Check(void);//检测是否存在DHT11
void DHT11_Rst(void);//复位DHT11
int dht11_pin = GET_PIN(G, 11);

//复位DHT11
void DHT11_Rst(void)
{
    os_pin_mode(dht11_pin,PIN_MODE_OUTPUT);
    //DHT11_IO_OUT();     //设置为输出
    os_pin_write(dht11_pin,0);
    //DHT11_DQ_OUT=0;     //拉低DQ

    delay_ms(20);       //拉低至少18ms
    os_pin_write(dht11_pin,1);
    //DHT11_DQ_OUT=1;     //DQ=1
    delay_us(30);       //主机拉高20~40us
}

//等待DHT11的回应
//返回1:未检测到DHT11的存在
//返回0:存在
os_uint8_t DHT11_Check(void)
{
    os_uint8_t retry=0;

    os_pin_mode(dht11_pin,PIN_MODE_INPUT_PULLUP);
    //DHT11_IO_IN();      //设置为输入
    while (os_pin_read(dht11_pin)&&retry<100)//DHT11会拉低40~80us
    {
        retry++;
        delay_us(1);
    };
    if(retry>=100)return 1;
    else retry=0;
    while (!os_pin_read(dht11_pin)&&retry<100)//DHT11拉低后会再次拉高40~80us
    {
        retry++;
        delay_us(1);
    };
    if(retry>=100)return 1;
    return 0;
}

//从DHT11读取一个位
//返回值：1/0
os_uint8_t DHT11_Read_Bit(void)
{
    os_uint8_t retry=0;
    while(os_pin_read(dht11_pin)&&retry<100)//等待变为低电平
    {
        retry++;
        delay_us(1);
    }
    retry=0;
    while(!os_pin_read(dht11_pin)&&retry<100)//等待变高电平
    {
        retry++;
        delay_us(1);
    }
    delay_us(40);//等待40us
    if(os_pin_read(dht11_pin))return 1;
    else return 0;
}

//从DHT11读取一个字节
//返回值：读到的数据
os_uint8_t DHT11_Read_Byte(void)
{
    os_uint8_t i,dat;
    dat=0;
    for (i=0;i<8;i++)
    {
        dat<<=1;
        dat|=DHT11_Read_Bit();
    }
    return dat;
}

//从DHT11读取一次数据
//temp:温度值(范围:0~50°)
//humi:湿度值(范围:20%~90%)
//返回值：0,正常;1,读取失败
os_uint8_t DHT11_Read_Data(os_uint8_t *temp,os_uint8_t *humi)
{
    os_uint8_t buf[5];
    os_uint8_t i;
    DHT11_Rst();
    if(DHT11_Check()==0)
    {
        for(i=0;i<5;i++)//读取40位数据
        {
            buf[i]=DHT11_Read_Byte();
        }
        if((buf[0]+buf[1]+buf[2]+buf[3])==buf[4])
        {
            *humi=buf[0];
            *temp=buf[2];
        }
    }else return 1;
    return 0;
}

//初始化DHT11的IO口 DQ 同时检测DHT11的存在
//返回1:不存在
//返回0:存在
os_uint8_t DHT11_Init(void)
{
    DHT11_Rst();
    return DHT11_Check();
}

void dht11_demo(void)
{
    os_uint8_t t=0;
    os_uint8_t temperature;
    os_uint8_t humidity;

    lcd_clear(WHITE);
    lcd_show_string(30, 100, 16,"dht11 demo");
    while(DHT11_Init()) //DHT11初始化
    {
        lcd_show_string(30,130,16,"DHT11 Error");
        delay_ms(200);
    }
    lcd_show_string(30,130,16,"DHT11 OK");
    lcd_show_string(30,150,16,"Temp:  C");
    lcd_show_string(30,170,16,"Humi:  %");
    while(1)
    {
        if(t%10==0)//每100ms读取一次
        {
            DHT11_Read_Data(&temperature,&humidity);        //读取温湿度值
            lcd_show_num(30+40,150,temperature,2,16);        //显示温度
            lcd_show_num(30+40,170,humidity,2,16);           //显示湿度
        }
        delay_ms(10);
        t++;
    }

}

#endif
