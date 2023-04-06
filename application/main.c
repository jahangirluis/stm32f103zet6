/**
 ***********************************************************************************************************************
 * Copyright (c) 2020, China Mobile Communications Group Co.,Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
 * an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations under the License.
 *
 * @file        main.c
 *
 * @brief       User application entry
 *
 * @revision
 * Date         Author          Notes
 * 2020-11-20   OneOS Team      First Version
 ***********************************************************************************************************************
 */

#include <board.h>
#include <os_task.h>
#include <atk_tflcd9341.h>
#include <time.h>
#include <sensor.h>
#include <drv_gpio.h>
#include <clocksource.h>
#include <drv_cfg.h>
#include <string.h>
#include <os_sem.h>
#include <os_memory.h>
#include <graphic/graphic.h>
#include <infrared/infrared.h>

extern void led_demo(void);
extern void beep_demo(void);
extern void key_demo(void);
extern void timer_demo(void);
extern void lcd_demo(void);
extern void infrared_recv_demo(void);
extern void sensor_light_demo(void);
extern void pwm_demo(void);
extern void adc_dac_demo(void);
extern void dht11_demo(void);
extern void at24cxx_i2c_demo(void);
extern void uart_demo(void);
extern void mpc6050_demo(void);
extern void led_sensor(void);
extern void timer(void);

extern os_uint8_t DHT11_Init(void); //初始化DHT11
extern os_uint8_t DHT11_Read_Data(os_uint8_t *temp, os_uint8_t *humi); //读取温湿度
extern os_uint8_t DHT11_Read_Byte(void); //读出一个字节
extern os_uint8_t DHT11_Read_Bit(void); //读出一个位
extern os_uint8_t DHT11_Check(void); //检测是否存在DHT11
extern void DHT11_Rst(void); //复位DHT11

#define DBG_TAG "infrared_recv_test"
#define INF_READ_BLOCK
#define INF_WRITE_BLOCK

static void user_task(void *parameter)
{
#if 0
    led_demo();
#endif
#if 0
    beep_demo();
#endif
#if 1
    key_demo();
#endif
#if 0
    timer_demo();
#endif
#if 0
    lcd_demo();
#endif
#if 0
    infrared_recv_demo();
#endif
#if 0
    sensor_light_demo();
#endif
#if 0
    pwm_demo();
#endif
#if 0
    adc_dac_demo();
#endif
#if 0
    dht11_demo();
#endif
#if 0
    at24cxx_i2c_demo();
#endif
#if 0
    uart_demo();
#endif
#if 0
    mpc6050_demo();
#endif
#if 0
    led_sensor();
#endif
#if 0
    timer();
#endif

    while (1)
    {
        os_kprintf("running....\r\n");
        os_task_msleep(300);
    }

}

int scene = 0; //当前场景编号，0为主界面，1为番茄钟界面，2为设置界面
os_task_t *lightSensor;
os_task_t *showNowTime;
os_task_t *tomatoClock;
os_task_t *infraListen;

static void showNowTimeFunc(void *parameter)
{
    //显示欢迎文字
    lcd_show_string(80, 50, 16, "Welcome to");
    lcd_show_string(30, 80, 16, "Tomato Smart Desk Lamp!");

    //显示功能帮助
    lcd_show_string(15, 200, 16, "Press Key0 to Tomato Clock");
    lcd_show_string(30, 220, 16, "Press Key1 to Settings");
    //lcd_show_string(30,240,16,"Press Key0 to Tomato Clock");

    //初始化DHT11
    int dhtStatu = DHT11_Init();

    while (1)
    {
        //获取系统时间
        time_t tmpcal_ptr;
        struct tm *tmp_ptr = NULL;
        time(&tmpcal_ptr); //获取时间戳
        tmp_ptr = localtime(&tmpcal_ptr); //转换成本地时间
        int year = 1900 + tmp_ptr->tm_year;
        int month = 1 + tmp_ptr->tm_mon;
        int day = tmp_ptr->tm_mday;
        int hour = tmp_ptr->tm_hour;
        int min = tmp_ptr->tm_min;
        int sec = tmp_ptr->tm_sec;

        //处理并显示日期
        char date[11];
        date[0] = year / 1000 + '0';
        date[1] = year / 100 % 10 + '0';
        date[2] = year / 10 % 10 + '0';
        date[3] = year % 10 + '0';
        date[4] = date[7] = '-';
        date[5] = month / 10 + '0';
        date[6] = month % 10 + '0';
        date[8] = day / 10 + '0';
        date[9] = day % 10 + '0';
        date[10] = '\0';
        lcd_show_string(80, 130, 16, date);

        //处理并显示时间
        char timet[9];
        timet[0] = hour / 10 + '0';
        timet[1] = hour % 10 + '0';
        timet[2] = timet[5] = ':';
        timet[3] = min / 10 + '0';
        timet[4] = min % 10 + '0';
        timet[6] = sec / 10 + '0';
        timet[7] = sec % 10 + '0';
        timet[8] = '\0';
        lcd_show_string(90, 150, 16, timet);

        //处理并显示温湿度

        os_uint8_t t = 0;
        os_uint8_t temperature;
        os_uint8_t humidity;
        if (dhtStatu)
        {
            lcd_show_string(80, 180, 16, "DHT11 ERROR");
        }
        else
        {
            DHT11_Read_Data(&temperature, &humidity);        //读取温湿度值
            char th[18];
            th[0] = 'T';
            th[1] = 'e';
            th[2] = 'm';
            th[3] = 'p';
            th[4] = ':';
            th[5] = temperature / 10 + '0';
            th[6] = temperature % 10 + '0';
            th[7] = 'C';
            th[8] = ' ';
            th[9] = 'H';
            th[10] = 'u';
            th[11] = 'm';
            th[12] = 'i';
            th[13] = ':';
            th[14] = humidity / 10 + '0';
            th[15] = humidity % 10 + '0';
            th[16] = '%';
            th[17] = '\0';
            lcd_show_string(50, 180, 16, th);
        }
    }

}

static void lightSensorFunc(void *parameter)
{
    struct os_sensor_data sensor_data;
    os_device_t *sensor = os_device_find("li_photo_diode");
    OS_ASSERT(sensor != NULL);
    os_device_open(sensor);

    struct os_sensor_info sensor_info;
    os_device_control(sensor, OS_SENSOR_CTRL_GET_INFO, &sensor_info);

    int i;
    for (i = 0; i < led_table_size; i++)
    {
        os_pin_mode(led_table[i].pin, PIN_MODE_OUTPUT);
    }

    int sleep_time;
    while (1)
    {
        os_device_read_nonblock(sensor, 0, &sensor_data, sizeof(struct os_sensor_data));
        int light_strengh = sensor_data.data.light;
        sleep_time = light_strengh;
        os_pin_write(led_table[0].pin, led_table[0].active_level);
        os_pin_write(led_table[1].pin, !led_table[1].active_level);
        os_task_msleep(sleep_time);
        os_pin_write(led_table[0].pin, !led_table[0].active_level);
        os_pin_write(led_table[1].pin, led_table[1].active_level);
        os_task_msleep(sleep_time);
    }
}

static void keyListenFunc(void *parameter)
{
    int i, val;

    for (i = 0; i < key_table_size; i++)
    {
        os_pin_mode(key_table[i].pin, key_table[i].mode);
    }

    while (1)
    {
        for (i = 0; i < key_table_size; i++)
        {
            val = os_pin_read(key_table[i].pin);
            switch (key_table[i].mode)
            {
            case PIN_MODE_INPUT_PULLUP:
                if (!val)
                    key_callback(key_table[i].pin);
                break;
            case PIN_MODE_INPUT_PULLDOWN:
                if (val)
                    key_callback(key_table[i].pin);
                break;
            }
        }
        os_task_msleep(100);
    }
}

void drawClock(int hour, int minute, int second)
{

    int i = 0, j = 0, ival, nval;
    float tmp = 0.8;
    time_t now;
    unsigned short colors[4] = { GREEN, BLUE, YELLOW };
    char titlebuf[30];
    os_uint16_t endp[12][2] = { { 120, 230 }, { 160, 218 }, { 188, 190 }, { 200, 150 }, { 188, 110 }, { 160, 82 }, {
            120, 70 }, { 80, 82 }, { 52, 110 }, { 40, 150 }, { 52, 190 }, { 80, 218 } };

    //second group
    os_uint16_t endp_second[60][2] = { { 120, 70 }, { 128, 71 }, { 137, 72 }, { 145, 74 }, { 152, 77 }, { 160, 82 }, {
            167, 85 }, { 174, 91 }, { 179, 96 }, { 185, 103 }, { 188, 110 }, { 193, 117 }, { 196, 125 }, { 198, 133 }, {
            199, 142 }, { 200, 150 }, { 199, 158 }, { 198, 167 }, { 196, 175 }, { 193, 183 }, { 188, 190 },
            { 185, 197 }, { 179, 204 }, { 174, 209 }, { 167, 215 }, { 160, 218 }, { 152, 223 }, { 145, 226 },
            { 137, 228 }, { 128, 229 }, { 120, 230 }, { 112, 229 }, { 103, 228 }, { 95, 226 }, { 88, 223 }, { 80, 218 },
            { 73, 215 }, { 66, 209 }, { 61, 204 }, { 55, 197 }, { 52, 190 }, { 47, 183 }, { 44, 175 }, { 42, 167 }, {
                    41, 158 }, { 40, 150 }, { 41, 142 }, { 42, 133 }, { 44, 125 }, { 47, 117 }, { 52, 110 },
            { 55, 103 }, { 61, 96 }, { 66, 91 }, { 73, 85 }, { 80, 82 }, { 88, 77 }, { 95, 74 }, { 103, 72 },
            { 112, 71 } };
    //minute group''
    os_uint16_t endp_minute[60][2] = { { 120, 96 }, { 125, 97 }, { 131, 98 }, { 137, 99 }, { 141, 101 }, { 147, 105 }, {
            151, 107 }, { 156, 111 }, { 159, 114 }, { 163, 119 }, { 165, 123 }, { 169, 128 }, { 171, 133 },
            { 172, 139 }, { 173, 145 }, { 174, 150 }, { 173, 155 }, { 172, 161 }, { 171, 167 }, { 169, 172 },
            { 165, 177 }, { 163, 181 }, { 159, 186 }, { 156, 189 }, { 151, 193 }, { 147, 195 }, { 141, 199 },
            { 137, 201 }, { 131, 202 }, { 125, 203 }, { 120, 204 }, { 115, 203 }, { 109, 202 }, { 103, 201 },
            { 99, 199 }, { 93, 195 }, { 89, 193 }, { 84, 189 }, { 81, 186 }, { 77, 181 }, { 75, 177 }, { 71, 172 }, {
                    69, 167 }, { 68, 161 }, { 67, 155 }, { 66, 150 }, { 67, 145 }, { 68, 139 }, { 69, 133 },
            { 71, 128 }, { 75, 123 }, { 77, 119 }, { 81, 114 }, { 84, 111 }, { 89, 107 }, { 93, 105 }, { 99, 101 }, {
                    103, 99 }, { 109, 98 }, { 115, 97 } };

    //hour group
    os_uint16_t endp_hour[12][2] = { { 120, 123 }, { 133, 127 }, { 143, 137 }, { 147, 150 }, { 143, 163 }, { 133, 173 },
            { 120, 177 }, { 107, 173 }, { 97, 163 }, { 93, 150 }, { 97, 137 }, { 107, 127 } };

    os_uint16_t diff[12][4] = { { -2, 4, 2, 8 }, { +1, 4, +5, 8 }, { +1, 4, +5, 8 }, { 4, -4, 8, 0 }, { 2, -2, 6, 2 }, {
            -2, -8, 2, -4 }, { -2, -8, 2, -4 }, { -2, -8, 2, -4 }, { -2, -6, 2, -2 }, { -4, -2, 2, 2 }, { -2, 2, 2, 6 },
            { -2, 4, 2, 8 } };

    os_pin_mode(key_table[0].pin, key_table[0].mode);
    ival = os_pin_read(key_table[i].pin);

    //lcd_clear(WHITE);
    //set_date(2023, 2, 27);
    //set_time(8, 50, 10);
    //lcd_color_draw_circle(120, 150, 106, WHITE);              //画表盘
    lcd_colorfill_draw_circle(120, 150, 105, BLACK);

    for (j = 0; j < 12; j++)   //画刻度
    {
        lcd_colorfill_draw_rectangle((endp[j][0] + diff[j][0]), (endp[j][1] + diff[j][1]), (endp[j][0] + diff[j][2]),
                (endp[j][1] + diff[j][3]), WHITE);
    }

    lcd_color_draw_line(120, 150, endp_second[second][0], endp_second[second][1], BLUE); //画second指针
    lcd_color_draw_line(120, 150, endp_minute[minute][0], endp_minute[minute][1], YELLOW); //画minute指针
    lcd_color_draw_line(120, 150, endp_hour[hour][0], endp_hour[hour][1], GREEN); //画hour指针

    //nval = ival;

    //j = 0;
    //i = 1;

    //second minute hour
    /*
     int is60second = 0, minute = 0, hour = 0;

     while (1)
     {
     now = rtc_get();   //digital time
     //os_snprintf(titlebuf, sizeof(titlebuf), "%s", ctime(&now));
     //lcd_show_string(10, 10, 16, titlebuf);  //显示时钟时间

     //is 60 second

     if (is60second == 60)
     {
     is60second = 0;
     ++minute;
     if (minute == 60)
     {
     minute = 0;
     ++hour;
     if (hour == 12)
     {
     hour = 0;
     }
     if (hour - 1 >= 0)
     {
     lcd_color_draw_line(120, 150 , endp_hour[hour - 1][0] , endp_hour[hour - 1][1] ,
     BLACK);
     }
     else
     {
     lcd_color_draw_line(120, 150 , endp_hour[11][0] , endp_hour[11][1] , BLACK);
     }
     }
     if (minute - 1 >= 0)
     {
     lcd_color_draw_line(120, 150 , endp_minute[minute - 1][0] , endp_minute[minute - 1][1] ,
     BLACK);
     }
     else
     {
     lcd_color_draw_line(120, 150 , endp_minute[59][0] , endp_minute[59][1] , BLACK);
     }
     }

     lcd_color_draw_line(120, 150 , endp_second[is60second][0] , endp_second[is60second][1] , BLUE); //画second指针

     lcd_color_draw_line(120, 150 , endp_minute[minute][0] , endp_minute[minute][1] , YELLOW);
     lcd_color_draw_line(120, 150 , endp_hour[hour][0] , endp_hour[hour][1] , GREEN);

     os_task_msleep(1000);
     lcd_color_draw_line(120, 150 , endp_second[is60second][0] , endp_second[is60second][1] , BLACK);
     lcd_color_draw_line(120, 150 , endp_minute[minute][0] , endp_minute[minute][1] , YELLOW);
     lcd_color_draw_line(120, 150 , endp_hour[hour][0], endp_hour[hour][1], GREEN);
     //防覆盖
     ++is60second;

     nval = os_pin_read(key_table[0].pin);   //检测按键
     if (nval != ival)
     i = -i;

     j = j + i;

     if (j < 0)
     j = 59;

     if (j == 60)
     j = 0;
     }
     */

}

int timing = 0;
int settingTime = 0;
int hour = 0, minute = 0, second = 0;
int j = 0;
static void tomatoClockFunc()
{
    //title
    lcd_show_string(70, 10, 16, "Tomato Clock");
    lcd_show_string(50, 30, 16, "Fighting Everyday!");

    //help
    lcd_show_string(30, 220, 16, "Press Key0 to Start Timing");
    lcd_show_string(30, 240, 16, "Press Key1 to Set Time");
    lcd_show_string(30, 260, 16, "Press Key_up to Back");

    //timingStr

    char timingStr[9];
    timingStr[0] = hour / 10 + '0';
    timingStr[1] = hour % 10 + '0';
    timingStr[2] = timingStr[5] = ':';
    timingStr[3] = minute / 10 + '0';
    timingStr[4] = minute % 10 + '0';
    timingStr[6] = second / 10 + '0';
    timingStr[7] = second % 10 + '0';
    timingStr[8] = '\0';
    lcd_show_string(90, 130, 16, timingStr);

    lcd_show_num(150, 140, hour, 2, 16);
    lcd_show_num(150, 160, minute, 2, 16);
    lcd_show_num(150, 180, second, 2, 16);
    lcd_show_num(150, 200, j, 1, 16);

    if (settingTime == 1)
    {
        lcd_show_string(50, 50, 16, "***Setting Time***");
    }
}

static void infraListenFunc(void *parameter)
{

    os_device_t *infrared;
    struct os_infrared_info info;
    int infrared_rx_count = 0;
    os_tick_t cur_time, pre_time = 0;
    int num;

    infrared = os_device_find("atk_rmt");
    OS_ASSERT(infrared);
    os_device_open(infrared);

    while (1)
    {
        settingTime = 1;
        os_device_read_block(infrared, 0, &info, sizeof(info));
        cur_time = os_tick_get();
        // one command, twice send
        if ((cur_time - pre_time) > 20)
        {
            switch (info.data)
            {
            case 22:
                num = 1;

                break;
            case 25:
                num = 2;
                break;
            case 13:
                num = 3;
                break;
            case 12:
                num = 4;
                break;
            case 24:
                num = 5;
                break;
            case 94:
                num = 6;
                break;
            case 8:
                num = 7;
                break;
            case 28:
                num = 8;
                break;
            case 90:
                num = 9;
                break;
            case 66:
                num = 0;
                break;
            case 74:
                //str = "DELETE";
                break;
            }

        }
        lcd_show_num(30, 120, num, 1, 16);
        lcd_show_num(30, 140, hour, 2, 16);
        lcd_show_num(30, 160, minute, 2, 16);
        lcd_show_num(30, 180, second, 2, 16);
        lcd_show_num(30, 200, j, 1, 16);

        switch (j)
        {
        case 0:
            hour = num * 10;
            break;
        case 1:
            hour += num;
            break;
        case 2:
            minute = num * 10;
            break;
        case 3:
            minute += num;
            break;
        case 4:
            second = num * 10;
            break;
        case 5:
            second += num;
            break;
        }
        j++;
        if (j >= 6)
        {
            os_task_destroy(infraListen);
        }
        os_task_msleep(200);
    }
}

void key_callback(int key_pin)
{
    switch (key_pin)
    {
    case GET_PIN(E, 4): //KEY0
        if (scene == 0)
        {
            os_task_suspend(showNowTime); //挂起显示时间任务
            lcd_clear(WHITE);
            scene = 1;
            tomatoClock = os_task_create("tomatoClock", tomatoClockFunc, NULL, 1024, 11);
            OS_ASSERT(tomatoClock);
            os_task_startup(tomatoClock);
            //drawClock(1, 0, 0);
        }
        else if (scene == 1)
        {
            timing = 1;
        }

        break;
    case GET_PIN(E, 3): //KEY1
        if (scene == 0)
        {

        }
        else if (scene == 1)
        {
            settingTime = 1;
            infraListen = os_task_create("infraListen", infraListenFunc, NULL, 1024, 5);
            OS_ASSERT(infraListen);
            os_task_startup(infraListen);
        }
        break;
    case GET_PIN(A, 0): //KEY_UP
        os_kprintf("3");
        break;
    }
}

int main(void)
{
    /*
     os_task_t *task;
     task = os_task_create("user", user_task, NULL, 16384, 30);
     OS_ASSERT(task);
     os_task_startup(task);
     */

    set_date(2023, 2, 27);
    set_time(8, 50, 10);
    //启动光感调节系统
    lightSensor = os_task_create("lightSensor", lightSensorFunc, NULL, 1024, 10);
    os_task_startup(lightSensor);

    //显示实时时间
    showNowTime = os_task_create("showNowTime", showNowTimeFunc, NULL, 1024, 30);
    os_task_startup(showNowTime);

    //监听按键
    os_task_t *keyListen;
    OS_ASSERT(keyListen);
    keyListen = os_task_create("keyListen", keyListenFunc, NULL, 1024, 15);
    os_task_startup(keyListen);

    return 0;
}

