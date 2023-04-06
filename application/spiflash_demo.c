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

#define W25Q_SPI_DEVICE_NAME    "spi2"         /* W25Q128接入的SPI设备名 */
struct os_spi_device* spi_dev_w25q;             /* W25Q128接入的SPI设备 */
#define WRITE_READ_ADDR 0x000000                /* W25Q128读取写入的地址(24bits) */

/* 写入W25Q128的数据内容 */
const os_uint8_t write_string[] = {"This is SPI test."};
#define WRITE_LEN sizeof(write_string)                      /* 写入W25Q128的数据长度 */

/**
 * @brief       w25q_erase
 * @param       addr : 要擦除的地址
 * @retval      无
 */
void w25q_erase(os_uint32_t addr)
{
    os_uint8_t send_buf[6],recv_buf=0;

    /* 先擦除要写入的地址 */
    send_buf[0] = 0x06;             /* 写使能命令 */
    send_buf[1] = 0x05;             /* 读取状态寄存器命令 */
    send_buf[2] = 0x20;             /* 擦除命令 */
    send_buf[3] = (os_uint8_t)(addr >> 16); /* 指定擦除的地址(24bits) */
    send_buf[4] = (os_uint8_t)(addr >> 8);
    send_buf[5] = (os_uint8_t)(addr);
    recv_buf |= 1 << 0;
    os_pin_write(OS_SPI_FLASH_CS_PIN, PIN_LOW);     /* 使能W25Q128片选 */
    os_spi_send(spi_dev_w25q, send_buf, 1);         /* 发送写使能命令 */
    do {                                                    /* 等待空闲 */
        os_spi_send_then_recv(spi_dev_w25q, &send_buf[1], 1, &recv_buf, 1);
    } while((recv_buf & 0x01) == 0x01);
    os_spi_send(spi_dev_w25q, &send_buf[2], 4);     /* 擦除指定地址的数据 */
    do {                                                    /* 等待空闲 */
        os_spi_send_then_recv(spi_dev_w25q, &send_buf[1], 1, &recv_buf, 1);
    } while((recv_buf& 0x01) == 0x01);
    os_pin_write(OS_SPI_FLASH_CS_PIN, PIN_HIGH);    /* 释放W25Q128片选 */
}

/**
 * @brief       w25q_write
 * @param       addr : 要写入的地址   buf:写入数据首地址  len:写入数据长度
 * @retval      无
 */
void w25q_write(os_uint32_t addr,os_uint8_t *buf,os_uint32_t len)
{
    os_uint8_t send_buf[6],recv_buf;
    /* 往指定地址写入数据 */
    send_buf[0] = 0x06;      /* 写使能命令 */
    send_buf[1] = 0x02;      /* 写页命令 */
    /* 指定写入的地址(24bits) */
    send_buf[2] = (os_uint8_t)(addr >> 16);
    send_buf[3] = (os_uint8_t)(addr >> 8);
    send_buf[4] = (os_uint8_t)(addr);
    send_buf[5] = 0x05;      /* 读取状态寄存器命令 */
    recv_buf |= 1 << 0;

    /* 使能W25Q128片选 */
    os_pin_write(OS_SPI_FLASH_CS_PIN, PIN_LOW);
    os_spi_send(spi_dev_w25q, &send_buf[0], 1);
    do {    /* 等待空闲 */
        os_spi_send_then_recv(spi_dev_w25q, &send_buf[5],1, &recv_buf, 1);
    } while((recv_buf & 0x01) == 0x01);
    os_spi_send_then_send(spi_dev_w25q, &send_buf[1],4, buf, len);
    do {    /* 等待空闲 */
        os_spi_send_then_recv(spi_dev_w25q, &send_buf[5],1, &recv_buf, 1);
    } while((recv_buf & 0x01) == 0x01);
    /* 释放W25Q128片选 */
    os_pin_write(OS_SPI_FLASH_CS_PIN, PIN_HIGH);

}

/** 通过SPI从W25Q128读取数据
 * @brief       w25q_read
 * @param       addr : 要读取的地址   buf:读取数据首地址  len:读取数据长度
 * @retval      无
 */
void w25q_read(os_uint32_t addr,os_uint8_t *buf,os_uint32_t len)
{
    os_uint8_t send_buf[4],recv_buf;

    send_buf[0] = 0x03; /* W25Q128读取命令 */
    /* 指定读取的地址(24bits) */
    send_buf[1] = (os_uint8_t)(addr >> 16);
    send_buf[2] = (os_uint8_t)(addr >> 8);
    send_buf[3] = (os_uint8_t)(addr);

    os_pin_write(OS_SPI_FLASH_CS_PIN, PIN_LOW); /* 使能W25Q128片选 */
    /* 读取W25Q128指定地址中指定长度的数据 */
    os_spi_send_then_recv(spi_dev_w25q, send_buf,4, buf, len);
    /* 释放W25Q128片选 */
    os_pin_write(OS_SPI_FLASH_CS_PIN, PIN_HIGH);
}

/**
 * @brief       usrt_task
 * @param       parameter : 传入参数(未用到)
 * @retval      无
 */
void spiflash_demo(void)
{
    os_uint8_t key = 0;
    os_uint8_t *recv_buf;


    for (os_uint8_t i=0; i<key_table_size; i++)
    {
        os_pin_mode(key_table[i].pin, key_table[i].mode);
    }

    os_pin_mode(OS_SPI_FLASH_CS_PIN,PIN_MODE_OUTPUT);

    recv_buf = (os_uint8_t*)os_malloc(WRITE_LEN);
    OS_ASSERT(recv_buf);

    spi_dev_w25q = (struct os_spi_device* )os_device_find(W25Q_SPI_DEVICE_NAME);
    OS_ASSERT(spi_dev_w25q);

    lcd_color_show_string(30,  70, 16, "SPI FLASH TEST", RED);
    lcd_color_show_string(30, 110, 16, "KEY_UP:Erase", RED);
    lcd_color_show_string(30, 130, 16, "KEY1  :Write", RED);
    lcd_color_show_string(30, 150, 16, "KEY0  :Read", RED);

    while(1)
    {
        if(!os_pin_read(key_table[0].pin))
        {
            lcd_color_show_string(30, 210, 16, "Reading....", BLUE);
            w25q_read(WRITE_READ_ADDR,recv_buf,WRITE_LEN);
            os_kprintf("Read \"%s\" from W25Q128 by SPI.\r\n", recv_buf);
            lcd_color_show_string(30, 210, 16, "W25Q128 Read Data:     ", BLUE);
            lcd_color_show_string(30, 230, 16, (char*)recv_buf, BLUE);
            os_free(recv_buf);
        }
        if(!os_pin_read(key_table[1].pin))
        {
            lcd_color_show_string(30, 210, 16, "Writing....", BLUE);
            w25q_erase(WRITE_READ_ADDR); /* 先擦除要写入的地址空间 */
            w25q_write(WRITE_READ_ADDR,(os_uint8_t *)write_string,WRITE_LEN); /*写入数据*/
            os_kprintf("Write \"%s\" to W25128Q by SPI.\r\n", write_string);
            lcd_color_show_string(30, 210, 16, "W25Q128 Write Finished!", BLUE);
            lcd_color_show_string(30, 230, 16, "                       ", BLUE);
        }
        if(os_pin_read(key_table[2].pin))
        {
            lcd_color_show_string(30, 210, 16, "Erasing....", BLUE);
            w25q_erase(WRITE_READ_ADDR);
            os_kprintf("W25128Q Erase Finished!\r\n", write_string);
            lcd_color_show_string(30, 210, 16, "W25Q128 Erase Finished!", BLUE);
            lcd_color_show_string(30, 230, 16, "                       ", BLUE);
        }
        os_task_msleep(100);
    }
}
