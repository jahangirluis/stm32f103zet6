
#include <board.h>
#include <os_clock.h>
#include <shell.h>
#include <string.h>
#include <infrared/infrared.h>
#include <dlog.h>
#include <graphic/graphic.h>
#include <atk_tflcd9341.h>

#define DBG_TAG "infrared_recv_test"

#define INF_READ_BLOCK
#define INF_WRITE_BLOCK
//char cmdstr[21][8] = {"ERROR","POWER","UP","PLAY","ALIENTEK","RIGHT","LEFT","VOL-","DOWN","VOL+","1","2","3","4","5","6","7","8","9","0","DELETE"};

void infrared_recv_demo(void)
{
    os_device_t *infrared;
    struct os_infrared_info info;
    int infrared_rx_count = 0;
    os_tick_t cur_time,pre_time= 0;
    char *str;

    infrared = os_device_find("atk_rmt");
    OS_ASSERT(infrared);

    os_device_open(infrared);

    lcd_clear(WHITE);
    lcd_show_string(20,120,24,"Press the key:");

    while (1)
    {
#ifdef INF_READ_BLOCK
        os_device_read_block(infrared, 0, &info, sizeof(info));
#else
        if (os_device_read_nonblock(infrared, 0, &info, sizeof(info)) != sizeof(info))
        {
            os_task_msleep(100);
            continue;
        }
#endif
        cur_time = os_tick_get();

        // one command, twice send
        if ((cur_time-pre_time)>20)
        {
            switch(info.data)
            {
            case 0:str="ERROR";break;
            case 69:str="POWER";break;
            case 70:str="UP";break;
            case 64:str="PLAY";break;
            case 71:str="ALIENTEK";break;
            case 67:str="RIGHT";break;
            case 68:str="LEFT";break;
            case 7:str="VOL-";break;
            case 21:str="DOWN";break;
            case 9:str="VOL+";break;
            case 22:str="1";break;
            case 25:str="2";break;
            case 13:str="3";break;
            case 12:str="4";break;
            case 24:str="5";break;
            case 94:str="6";break;
            case 8:str="7";break;
            case 28:str="8";break;
            case 90:str="9";break;
            case 66:str="0";break;
            case 74:str="DELETE";break;
            }
            lcd_clear(WHITE);
            lcd_show_string(120-strlen(str)*12/2,120,24,str);

            LOG_I(DBG_TAG,"(%d) data:%d key: %s", ++infrared_rx_count, info.data,str);
        }
        pre_time = cur_time;
    }
}
