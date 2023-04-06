#include <board.h>
#include <drv_cfg.h>
#include <arch_interrupt.h>
#include <os_memory.h>
#include <sensors/sensor.h>
#include <drv_log.h>
#include <string.h>
#include <atk_tflcd9341.h>

#ifdef OS_USING_SHELL
#include <shell.h>
#endif

#define DBG_TAG "at24cxx"

#define OS_AT24CXX_I2C_ADDR 0xA0 >> 1

static struct os_i2c_bus_device* at24cxx_i2c_bus;

static os_err_t write_regs(struct os_i2c_bus_device *bus, uint16_t reg, uint8_t *buf, uint8_t len)
{
    struct os_i2c_msg msgs;
    uint8_t        databuf[9];
    uint8_t        device_addr = 0;
    uint8_t        reg_addr    = 0;

    OS_ASSERT(len <= 8);

    if (reg > 255)
    {
        device_addr = OS_AT24CXX_I2C_ADDR | 0x01;
    }
    else
    {
        device_addr = OS_AT24CXX_I2C_ADDR;
    }
    reg_addr   = reg & 0xFF;
    databuf[0] = reg_addr;

    memcpy(&databuf[1], buf, len);

    msgs.addr  = device_addr;
    msgs.flags = OS_I2C_WR;
    msgs.buf   = databuf;
    msgs.len   = len + 1;

    if (os_i2c_transfer(bus, &msgs, 1) == 1)
    {
        return OS_EOK;
    }
    else
    {
        LOG_E(DBG_TAG, "Writing command error");
        return OS_ERROR;
    }
}

static os_err_t read_regs(struct os_i2c_bus_device *bus, uint16_t reg, uint8_t *buf, uint8_t len)
{
    struct os_i2c_msg msgs[2];
    uint8_t        device_addr = 0;
    uint8_t        reg_addr    = 0;

    if (reg > 255)
    {
        device_addr = OS_AT24CXX_I2C_ADDR | 0x01;
    }
    else
    {
        device_addr = OS_AT24CXX_I2C_ADDR;
    }
    reg_addr = reg & 0xFF;

    msgs[0].addr  = device_addr;
    msgs[0].flags = OS_I2C_WR;
    msgs[0].buf   = &reg_addr;
    msgs[0].len   = 1;

    msgs[1].addr  = device_addr;
    msgs[1].flags = OS_I2C_RD;
    msgs[1].buf   = buf;
    msgs[1].len   = len;

    if (os_i2c_transfer(bus, msgs, 2) == 2)
    {
        return OS_EOK;
    }
    else
    {
        LOG_E(DBG_TAG, "Reading command error");
        return OS_ERROR;
    }
}

void at24cxx_i2c_demo(void)
{
    int        count = 1;
    uint8_t read_buff[8];
    uint8_t write_buff[8];

    /* find i2c device  */
    at24cxx_i2c_bus = (struct os_i2c_bus_device *)os_device_find("soft_i2c1");
    if (at24cxx_i2c_bus == NULL)
    {
        LOG_E(DBG_TAG, "AT24CXX i2c invalid.");
        return;
    }
    os_pin_mode(key_table[0].pin, key_table[0].mode);
    os_pin_mode(key_table[1].pin, key_table[1].mode);

    lcd_clear(WHITE);
    lcd_show_string(30,70,16,"24C02 IIC TEST");
    lcd_show_string(30,130,16,"KEY1:Write  KEY0:Read");   //显示提示信息

    while(1)
    {
        if(!os_pin_read(key_table[0].pin))
        {
            memset(read_buff, 0, sizeof(read_buff));
            lcd_show_string(30,170,16,"Start Read 24C02.... ");
            read_regs(at24cxx_i2c_bus, 0, read_buff, sizeof(read_buff));
            lcd_show_string(30,170,16,"The Data Readed Is:  ");//提示传送完成
            lcd_show_string(30,210,16,read_buff);//显示读到的字符串
            os_task_msleep(100);
        }
        if(!os_pin_read(key_table[1].pin))
        {
            lcd_fill(0,170,239,319,WHITE);//清除半屏
            lcd_show_string(30,170,16,"Writing....");
            os_snprintf(write_buff, 8, "test %d", count++);
            write_regs(at24cxx_i2c_bus, 0, write_buff, sizeof(write_buff));
            lcd_show_string(130,170,16,"Finished!");//提示传送完成
            if (count>99) count = 1;
            os_task_msleep(100);
        }
    }
}
