#include <drv_cfg.h>
#include <os_clock.h>
#include <stdio.h>
#include <string.h>
#include <shell.h>
#include <sensor.h>
#include <mpu6xxx.h>
#include <atk_tflcd9341.h>
extern uint8_t mpu_dmp_init(void);
extern uint8_t mpu_dmp_get_data(float *pitch,float *roll,float *yaw);

#define BUF_SIZE 30
static void pad_buf(char *buf,int size)
{
    int i = strlen(buf);
    for(;i<size-2;i++)
        buf[i] = ' ';
    buf[size-2]='\r';
    buf[size-1]='\n';
}

os_device_t   *uartdev;

static int uart_report_init(void)
{
    int ret;

    uartdev = os_device_find("uart4");
    OS_ASSERT(uartdev);

    os_device_open(uartdev);

    /* uart config */
    struct serial_configure config = OS_SERIAL_CONFIG_DEFAULT;
    config.baud_rate = 500000;//BAUD_RATE_115200;
    ret = os_device_control(uartdev, OS_DEVICE_CTRL_CONFIG, &config);
    if (ret != 0)
    {
        return -1;
    }
    return 0;
}
char niming_report = 0;
//传送数据给匿名四轴上位机软件(V2.6版本)

//发送加速度传感器数据和陀螺仪数据
//aacx,aacy,aacz:x,y,z三个方向上面的加速度值
//gyrox,gyroy,gyroz:x,y,z三个方向上面的陀螺仪值
void usart_niming_report_6data(struct sensor_3_axis acce,struct sensor_3_axis gyro)
{
    os_uint8_t send_buf[16];
    os_uint8_t i,tx_cnt;
    send_buf[15]=0;  //校验数置零
    send_buf[0]=0X88;   //帧头
    send_buf[1]=0XA1;    //功能字：自定义帧,0XA1
    send_buf[2]=12;    //数据长度
    send_buf[3]=(acce.x>>8)&0XFF;
    send_buf[4]=acce.x&0XFF;
    send_buf[5]=(acce.y>>8)&0XFF;
    send_buf[6]=acce.y&0XFF;
    send_buf[7]=(acce.z>>8)&0XFF;
    send_buf[8]=acce.z&0XFF;
    send_buf[9]=(gyro.x>>8)&0XFF;
    send_buf[10]=gyro.x&0XFF;
    send_buf[11]=(gyro.y>>8)&0XFF;
    send_buf[12]=gyro.y&0XFF;
    send_buf[13]=(gyro.z>>8)&0XFF;
    send_buf[14]=gyro.z&0XFF;
    for(i=0;i<15;i++)send_buf[15]+=send_buf[i];   //计算校验和
    tx_cnt = os_device_write_block(uartdev, 0, send_buf, 16);
    if (tx_cnt<=0) {
        os_kprintf("%d:tx error\r\n",tx_cnt);
    }
    else {
       os_kprintf("%d:tx success\r\n",i);
    }
}

//通过串口1上报结算后的姿态数据给电脑
//aacx,aacy,aacz:x,y,z三个方向上面的加速度值
//gyrox,gyroy,gyroz:x,y,z三个方向上面的陀螺仪值
//roll:横滚角.单位0.01度。 -18000 -> 18000 对应 -180.00  ->  180.00度
//pitch:俯仰角.单位 0.01度。-9000 - 9000 对应 -90.00 -> 90.00 度
//yaw:航向角.单位为0.1度 0 -> 3600  对应 0 -> 360.0度

void usart_niming_report_euler(struct sensor_3_axis acce,struct sensor_3_axis gyro,short pitch,short roll,short yaw)
{
    os_uint8_t send_buf[32];
    os_uint8_t i,tx_cnt;
    send_buf[31]=0;  //校验数置零
    send_buf[0]=0X88;   //帧头
    send_buf[1]=0XAF;    //功能字：飞控显示帧,0XAF
    send_buf[2]=28;    //数据长度
    send_buf[3]=(acce.x>>8)&0XFF;
    send_buf[4]=acce.x&0XFF;
    send_buf[5]=(acce.y>>8)&0XFF;
    send_buf[6]=acce.y&0XFF;
    send_buf[7]=(acce.z>>8)&0XFF;
    send_buf[8]=acce.z&0XFF;
    send_buf[9]=(gyro.x>>8)&0XFF;
    send_buf[10]=gyro.x&0XFF;
    send_buf[11]=(gyro.y>>8)&0XFF;
    send_buf[12]=gyro.y&0XFF;
    send_buf[13]=(gyro.z>>8)&0XFF;
    send_buf[14]=gyro.z&0XFF;
    for(i=15;i<21;i++)send_buf[i]=0;//清0
    send_buf[21]=(roll>>8)&0XFF;
    send_buf[22]=roll&0XFF;
    send_buf[23]=(pitch>>8)&0XFF;
    send_buf[24]=pitch&0XFF;
    send_buf[25]=(yaw>>8)&0XFF;
    send_buf[26]=yaw&0XFF;
    for(i=27;i<31;i++)send_buf[i]=0;//清0
    for(i=0;i<31;i++)send_buf[31]+=send_buf[i];   //计算校验和
    tx_cnt = os_device_write_block(uartdev, 0, send_buf, 32);
    if (tx_cnt<=0) {
        os_kprintf("%d:tx error\r\n",tx_cnt);
    }
    else {
       os_kprintf("%d:tx success\r\n",i);
    }
}

void mpc6050_demo(void)
{
    struct mpu6xxx_3axes accel, gyro;
    float temp;
    char buf[BUF_SIZE];
    os_device_t *accedev,*gyrodev,*tempdev;
    struct os_sensor_data acce_sensor_data,gyro_sensor_data,temp_sensor_data;
    float pitch,roll,yaw;
    uint8_t ret;
    int i=0;

    accedev = os_device_find("acce_mpu6050");
    OS_ASSERT(accedev);
    gyrodev = os_device_find("gyro_mpu6050");
    OS_ASSERT(gyrodev);
    tempdev = os_device_find("temp_mpu6050");
    OS_ASSERT(tempdev);

    os_device_open(accedev);
    os_device_open(gyrodev);
    os_device_open(tempdev);

    lcd_clear(WHITE);
    lcd_show_string(30, 50, 16,"mpu6050 demo");

    if(ret = mpu_dmp_init())
    {
        os_snprintf(buf,BUF_SIZE,"MPU6050 DMP INIT Error:%d\r\n",ret);
        lcd_show_string(30,70,16,buf);
        while(1) os_task_msleep(200);
    }
    else {
        os_snprintf(buf,BUF_SIZE,"MPU6050 DMP INIT OK\r\n",ret);
        lcd_show_string(30,70,16,buf);
    }

    if(ret = uart_report_init())
    {
        os_snprintf(buf,BUF_SIZE,"UART INIT Error:%d\r\n",ret);
        lcd_show_string(30,90,16,buf);
        while(1) os_task_msleep(200);
    }
    else {
        os_snprintf(buf,BUF_SIZE,"UART INIT OK\r\n",ret);
        lcd_show_string(30,90,16,buf);
    }

    os_pin_mode(key_table[0].pin, key_table[0].mode);

    while(1)
    {
        if(!os_pin_read(key_table[0].pin)) niming_report=!niming_report;
        if(niming_report)lcd_show_string(30,110,16,"UPLOAD ON ");
                    else lcd_show_string(30,110,16,"UPLOAD OFF");

        os_device_read_nonblock(accedev, 0, &acce_sensor_data, sizeof(struct os_sensor_data));
        os_device_read_nonblock(gyrodev, 0, &gyro_sensor_data, sizeof(struct os_sensor_data));
#if 0
        os_snprintf(buf,BUF_SIZE,"    acce: %d, %d, %d", acce_sensor_data.data.acce.x, acce_sensor_data.data.acce.y, acce_sensor_data.data.acce.z);
        pad_buf(buf,BUF_SIZE);
        lcd_show_string(0,130,16,buf);
        os_snprintf(buf,BUF_SIZE,"    gyro: %d, %d, %d", gyro_sensor_data.data.gyro.x/100, gyro_sensor_data.data.gyro.y/100, gyro_sensor_data.data.gyro.z/100); /*100multiple*/
        pad_buf(buf,BUF_SIZE);
        lcd_show_string(0,150,16,buf);
#else

#endif
        ret = mpu_dmp_get_data(&pitch,&roll,&yaw);
        if(ret==0)
        {
            if (niming_report)
            {
                usart_niming_report_6data(acce_sensor_data.data.acce,gyro_sensor_data.data.gyro);
                usart_niming_report_euler(acce_sensor_data.data.acce,gyro_sensor_data.data.gyro,(int)(roll*100),(int)(pitch*100),(int)(yaw*10));
            }
#if 0
            os_snprintf(buf,BUF_SIZE,"    pitch: %s%d.%d", pitch>0?"":"-",(int)(abs(pitch)), (int)(abs(pitch*1000))%1000);
            pad_buf(buf,BUF_SIZE);
            lcd_show_string(0,190,16,buf);
            os_snprintf(buf,BUF_SIZE,"    roll: %s%d.%d", roll>0?"":"-",(int)(abs(roll)), (int)(abs(roll*1000))%1000);
            pad_buf(buf,BUF_SIZE);
            lcd_show_string(0,210,16,buf);
            os_snprintf(buf,BUF_SIZE,"    yaw: %s%d.%d", yaw>0?"":"-",(int)(abs(yaw)), (int)(abs(yaw*1000))%1000);
            pad_buf(buf,BUF_SIZE);
            lcd_show_string(0,230,16,buf);
#endif
        }
        i++;
        if(i%100==0)
        {
            os_device_read_nonblock(tempdev, 0, &temp_sensor_data, sizeof(struct os_sensor_data));
            os_snprintf(buf,BUF_SIZE,"    temp = %d.%d", (int)(temp_sensor_data.data.temp/1000), (int)(temp_sensor_data.data.temp % 1000));/*1000multiple*/
            pad_buf(buf,BUF_SIZE);
            lcd_show_string(0,170,16,buf);
        }
        os_task_msleep(1);
    }

}
