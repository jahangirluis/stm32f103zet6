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
 * @file        me3630_w_ifconfig.c
 *
 * @brief       me3630_w_ifconfig.c module api
 *
 * @revision
 * Date         Author          Notes
 * 2020-12-1   OneOS Team      First Version
 ***********************************************************************************************************************
 */

#include <mo_ipaddr.h>
#include "me3630_w_general.h"
#include "me3630_w_netserv.h"
#include "me3630_w_ifconfig.h"

#include <stdlib.h>
#include <string.h>

#define MO_LOG_TAG "me3630_w.ifconfig"
#define MO_LOG_LVL  MO_LOG_INFO
#include "mo_log.h"

#ifdef ME3630_W_USING_IFCONFIG_OPS

os_err_t me3630_w_ifconfig(mo_object_t *self)
{
    char ipaddr[IPADDR_MAX_STR_LEN + 1] = {0};
    if (me3630_w_get_ipaddr(self, ipaddr) != OS_EOK)
    {
        memset(ipaddr, 0, sizeof(ipaddr));
    }

    char imei[MO_IMEI_LEN + 1] = {0};
    if (me3630_w_get_imei(self, imei, sizeof(imei)) != OS_EOK)
    {
        memset(imei, 0, sizeof(imei));
    }

    char iccid[MO_ICCID_LEN + 1] = {0};
    if (me3630_w_get_iccid(self, iccid, sizeof(iccid)) != OS_EOK)
    {
        memset(iccid, 0, sizeof(iccid));
    }

    os_uint8_t rssi = 0;
    os_uint8_t ber  = 0;

    if (me3630_w_get_csq(self, &rssi, &ber) != OS_EOK)
    {
        rssi = 0;
        ber  = 0;
    }

    os_kprintf("\r\nLIST AT MODULE INFORMATIONS\r\n");
    for (int i = 0; i < 40; i++)
    {
        os_kprintf("--");
    }

    os_kprintf("\r\n");
    os_kprintf("Module Name    : %s\r\n", self->name);
    os_kprintf("IMEI   Number  : %s\r\n", imei);
    os_kprintf("ICCID  Number  : %s\r\n", iccid);
    os_kprintf("Signal Quality : rssi(%d), ber(%d)\r\n", rssi, ber);
    os_kprintf("IPv4   Address : %s\r\n", strlen(ipaddr) ? ipaddr : "0.0.0.0");

    for (int i = 0; i < 40; i++)
    {
        os_kprintf("--");
    }
    os_kprintf("\r\n");

    return OS_EOK;
}

os_err_t me3630_w_get_ipaddr(mo_object_t *self, char ip[])
{
#define IPADDR_MAX_STR_LEN_ME3630 64
    at_parser_t *parser = &self->parser;
    os_int8_t    ucid   = -1;
    os_int8_t    len    = -1;
    int          i      = 2;
    char ipaddr[IPADDR_MAX_STR_LEN_ME3630] = {0};

    char resp_buff[512] = {0};

    at_resp_t resp = {.buff = resp_buff, .buff_size = sizeof(resp_buff), .timeout = 10 * OS_TICK_PER_SECOND};

    os_err_t result = at_parser_exec_cmd(parser, &resp, "AT+CGPADDR");
    if (result != OS_EOK)
    {
        ERROR("Get ip address fail: AT+CGPADDR cmd exec fail.");
        goto __exit;
    }

    /* Response for ex: +CGPADDR:0,100.113.120.235 */
    do
    {
        memset(ipaddr, 0, sizeof(ipaddr));
        if (at_resp_get_data_by_line(&resp, i , "+CGPADDR: %d,%s", &ucid, ipaddr) <= 0)
        {
            ERROR("Get ip address: parse resp fail.");
            result = OS_ERROR;
            goto __exit;
        }
        i = i + 1;
        len = strlen(ipaddr);
        if(i > 6)
        {
            break;
        }
    }
    while(len == 0 || ipaddr[0] == '0' );

    if ((len < IPADDR_MIN_STR_LEN) || (len > IPADDR_MAX_STR_LEN))
    {
        ERROR("IP address size [%d] error.", len);
        result = OS_ERROR;
        goto __exit;
    }
    else
    {
        strncpy(ip, ipaddr,len);
        DEBUG("IP address: %s", ip);
    }

__exit:

    return result;
}

#endif /* ME3630_W_USING_IFCONFIG_OPS */

