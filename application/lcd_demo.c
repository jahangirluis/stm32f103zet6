#include <board.h>
#include <drv_cfg.h>
#include <string.h>
#include <os_sem.h>
#include <os_memory.h>
#include <graphic/graphic.h>
#include <atk_tflcd9341.h>

#if 1
void lcd_demo(void)
{
    int i = 0,j=0,ival,nval;
    time_t now;
    unsigned short colors[4] = {GREEN,BLUE,YELLOW};
    char titlebuf[30];
    os_uint16_t endp[12][2]={{120,230},{160,218},{188,190},{200,150},{188,110},{160,82},{120,70},{80,82},{52,110},{40,150},{52,190},{80,218}};
    os_uint16_t diff[12][4]={{-2,4,2,8},{+1,4,+5,8},{+1,4,+5,8},{4,-4,8,0},{2,-2,6,2},{-2,-8,2,-4},{-2,-8,2,-4},{-2,-8,2,-4},{-2,-6,2,-2},{-4,-2,2,2},{-2,2,2,6},{-2,4,2,8}};

    os_pin_mode(key_table[0].pin, key_table[0].mode);
    ival = os_pin_read(key_table[i].pin);

    lcd_clear(WHITE);
    set_date(2023, 2, 27);
    set_time(8, 50, 10);
    lcd_color_draw_circle(120,150,106,BLUE);              //画表盘
    lcd_colorfill_draw_circle(120,150,105,GRAY);

    for (j = 0; j < 12; j++)   //画刻度
    {
        lcd_colorfill_draw_rectangle(endp[j][0]+diff[j][0],endp[j][1]+diff[j][1],endp[j][0]+diff[j][2],endp[j][1]+diff[j][3],BLUE);
    }

    nval = ival;

    j = 0;
    i = 1;
    while (1)
    {
        now = rtc_get();
        os_snprintf(titlebuf,sizeof(titlebuf),"%s", ctime(&now));
        lcd_show_string(10,10,16,titlebuf);  //显示时钟时间

        lcd_color_draw_line(120,150,endp[j][0],endp[j][1],YELLOW);   //画指针
        os_task_msleep(300);
        lcd_color_draw_line(120,150,endp[j][0],endp[j][1],GRAY);
        os_task_msleep(200);

        nval = os_pin_read(key_table[0].pin);   //检测按键
        if (nval != ival)
            i = -i;

        j = j+i;
        if (j<0)
            j = 11;
        if (j>=12)
            j = 0;
    }
}
#endif
