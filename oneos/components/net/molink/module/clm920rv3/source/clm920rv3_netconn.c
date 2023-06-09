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
 * @file        clm920rv3_netconn.c
 *
 * @brief       clm920rv3 module link kit netconnect api
 *
 * @revision
 * Date         Author          Notes
 * 2020-03-25   OneOS Team      First Version
 ***********************************************************************************************************************
 */

#include "clm920rv3_netconn.h"
#include "clm920rv3.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef CLM920RV3_USING_NETCONN_OPS

#define MO_LOG_TAG "clm920rv3.netconn"
#define MO_LOG_LVL  MO_LOG_INFO
#include "mo_log.h"

#define CLM920RV3_SEND_MAX_SIZE (1460)
#define CLM920RV3_RESP_DEF_SIZE (256)
#define CLM920RV3_CONN_ID_NULL  (-1)

#define CLM920RV3_NETCONN_MQ_NAME     "clm920rv3_nc_mq"
#ifndef CLM920RV3_NETCONN_MQ_MSG_SIZE
#define CLM920RV3_NETCONN_MQ_MSG_SIZE (sizeof(mo_notconn_msg_t))
#endif /* CLM920RV3_NETCONN_MQ_MSG_SIZE */
#ifndef CLM920RV3_NETCONN_MQ_MSG_MAX
#define CLM920RV3_NETCONN_MQ_MSG_MAX  (5)
#endif /* CLM920RV3_NETCONN_MQ_MSG_MAX */

#define SET_EVENT(socket, event) (((socket + 1) << 16) | (event))

#define CLM920RV3_EVENT_CONN_OK   (1L << 0)
#define CLM920RV3_EVENT_SEND_OK   (1L << 1)
#define CLM920RV3_EVENT_RECV_OK   (1L << 2)
#define CLM920RV3_EVNET_CLOSE_OK  (1L << 3)
#define CLM920RV3_EVENT_CONN_FAIL (1L << 4)
#define CLM920RV3_EVENT_DOMAIN_OK (1L << 5)
#define CLM920RV3_EVENT_PDP_ACT   (1L << 6)

static os_err_t clm920rv3_lock(os_mutex_t *mutex)
{
    return os_mutex_recursive_lock(mutex, OS_WAIT_FOREVER);
}

static os_err_t clm920rv3_unlock(os_mutex_t *mutex)
{
    return os_mutex_recursive_unlock(mutex);
}

static mo_netconn_t *clm920rv3_netconn_alloc(mo_object_t *module)
{
    mo_clm920rv3_t *clm920rv3 = os_container_of(module, mo_clm920rv3_t, parent);

    for (int i = 0; i < CLM920RV3_NETCONN_NUM; i++)
    {
        if (NETCONN_STAT_NULL == clm920rv3->netconn[i].stat)
        {
            /* TODO: Remove after adding the API of setting local port  */
            /* NOTE: CLM920RV3 module dosen't support auto allocate udp port */
            clm920rv3->netconn[i].local_port = i + 1;

            return &clm920rv3->netconn[i];
        }
    }

    ERROR("Moduel %s alloc netconn failed!", module->name);

    return OS_NULL;
}

static mo_netconn_t *clm920rv3_get_netconn_by_id(mo_object_t *module, os_int32_t connect_id)
{
    OS_ASSERT(OS_NULL != module);

    if (1 > connect_id || CLM920RV3_NETCONN_NUM < connect_id)
    {
        ERROR("%s INVALID connet_id:[%d]!", __func__, connect_id);
        return OS_NULL;
    }

    mo_clm920rv3_t *clm920rv3 = os_container_of(module, mo_clm920rv3_t, parent);

    return &clm920rv3->netconn[connect_id - 1];
}

os_err_t clm920rv3_netconn_get_info(mo_object_t *module, mo_netconn_info_t *info)
{
    mo_clm920rv3_t *clm920rv3 = os_container_of(module, mo_clm920rv3_t, parent);

    info->netconn_array = clm920rv3->netconn;
    info->netconn_nums  = sizeof(clm920rv3->netconn) / sizeof(clm920rv3->netconn[0]);

    return OS_EOK;
}

os_err_t clm920rv3_pdp_act(mo_clm920rv3_t *clm920rv3)
{
    char tmp_data[20] = {0};
    char APN[10]      = {0};

    at_parser_t *parser = &clm920rv3->parent.parser;

    char resp_buff[CLM920RV3_RESP_DEF_SIZE] = {0};

    at_resp_t resp = {.buff = resp_buff, .buff_size = sizeof(resp_buff), .timeout = OS_TICK_PER_SECOND};

    clm920rv3_lock(&clm920rv3->netconn_lock);

    /* Use AT+COPS? to query current Network Operator */
    os_err_t result = at_parser_exec_cmd(parser, &resp, "AT+COPS?");
    if (result != OS_EOK)
    {
        goto __exit;
    }

    if (at_resp_get_data_by_kw(&resp, "+COPS:", "+COPS: %*[^\"]\"%[^\"]", tmp_data) <= 0)
    {
        result = OS_ERROR;
        goto __exit;
    }

    switch (atoi(tmp_data + 3))
    {
    case 0:  /* 46000 */
    case 2:  /* 46002 */
    case 4:  /* 46004 */
    case 7:  /* 46007 */
        strncpy(APN, "cmnet", strlen("cmnet"));
        break;

    case 1:  /* 46001 */
    case 6:  /* 46006 */
    case 9:  /* 46009 */
        strncpy(APN, "3gnet", strlen("3gnet"));
        break;

    case 3:  /* 46003 */
    case 5:  /* 46005 */
    case 11: /* 46011 */
        strncpy(APN, "ctnet", strlen("ctnet"));
        break;

    default:
        ERROR("%s Get apn failed", __func__);
        result = OS_ERROR;
        goto __exit;
    }

    result = at_parser_exec_cmd(parser, &resp, "AT+QIPCSGP=1,1,\"%s\"", APN);
    if (result != OS_EOK)
    {
        goto __exit;
    }

    resp.timeout = 10 * OS_TICK_PER_SECOND;

    result = at_parser_exec_cmd(parser, &resp, "AT+QIPDEACT=1");
    if (result != OS_EOK)
    {
        goto __exit;
    }

    /* PDP context active process, recommend timeout:12s */
    os_event_recv(&clm920rv3->netconn_evt,
                   CLM920RV3_EVENT_PDP_ACT,
                   OS_EVENT_OPTION_OR | OS_EVENT_OPTION_CLEAR,
                   OS_NO_WAIT,
                   OS_NULL);

    resp.timeout  = 10 * OS_TICK_PER_SECOND;

    result = at_parser_exec_cmd(parser, &resp, "AT+QIPACT=1");
    if (result != OS_EOK)
    {
        goto __exit;
    }

    result = os_event_recv(&clm920rv3->netconn_evt,
                           CLM920RV3_EVENT_PDP_ACT,
                           OS_EVENT_OPTION_OR | OS_EVENT_OPTION_CLEAR,
                           12 * OS_TICK_PER_SECOND,
                           OS_NULL);

__exit:
    clm920rv3_unlock(&clm920rv3->netconn_lock);

    if (result != OS_EOK)
    {
        ERROR("Set CLM920RV3 PDP act failed");
    }
    else
    {
        DEBUG("Set CLM920RV3 PDP act success");
    }

    return result;
}

mo_netconn_t *clm920rv3_netconn_create(mo_object_t *module, mo_netconn_type_t type)
{
    mo_clm920rv3_t *clm920rv3 = os_container_of(module, mo_clm920rv3_t, parent);

    clm920rv3_lock(&clm920rv3->netconn_lock);

    mo_netconn_t *netconn = clm920rv3_netconn_alloc(module);

    if (OS_NULL == netconn)
    {
        clm920rv3_unlock(&clm920rv3->netconn_lock);
        return OS_NULL;
    }

    netconn->mq = os_mq_create(CLM920RV3_NETCONN_MQ_NAME,
                               CLM920RV3_NETCONN_MQ_MSG_SIZE,
                               CLM920RV3_NETCONN_MQ_MSG_MAX);
    if (OS_NULL == netconn->mq)
    {
        ERROR("%s message queue create failed, no enough memory.", module->name);
        clm920rv3_unlock(&clm920rv3->netconn_lock);
        return OS_NULL;
    }

    netconn->stat = NETCONN_STAT_INIT;
    netconn->type = type;

    clm920rv3_unlock(&clm920rv3->netconn_lock);

    return netconn;
}

os_err_t clm920rv3_netconn_destroy(mo_object_t *module, mo_netconn_t *netconn)
{
    at_parser_t *parser = &module->parser;
    os_err_t     result = OS_ERROR;

    DEBUG("Module %s in %d netconn status", module->name, netconn->stat);

    char resp_buff[CLM920RV3_RESP_DEF_SIZE] = {0};

    at_resp_t resp = {.buff = resp_buff, .buff_size = sizeof(resp_buff), .timeout = 10 * OS_TICK_PER_SECOND};

    switch (netconn->stat)
    {
    case NETCONN_STAT_INIT:
        break;
    case NETCONN_STAT_CONNECT:
        result = at_parser_exec_cmd(parser, &resp, "AT+QIPCLOSE=%d", netconn->connect_id);
        if (result != OS_EOK)
        {
            ERROR("Module %s destroy %s netconn failed",
                      module->name,
                      (netconn->type == NETCONN_TYPE_TCP) ? "TCP" : "UDP");
            return result;
        }
        break;
    default:
        /* add handler when we need */
        break;
    }

    if (netconn->stat != NETCONN_STAT_NULL)
    {
        mo_netconn_mq_destroy(netconn->mq);
        netconn->mq = OS_NULL;
    }

    INFO("Module %s netconn id %d destroyed", module->name, netconn->connect_id);

    netconn->stat        = NETCONN_STAT_NULL;
    netconn->type        = NETCONN_TYPE_NULL;
    netconn->remote_port = 0;
    inet_aton("0.0.0.0", &netconn->remote_ip);

    return OS_EOK;
}

os_err_t clm920rv3_netconn_connect(mo_object_t *module, mo_netconn_t *netconn, ip_addr_t addr, os_uint16_t port)
{
    at_parser_t *parser = &module->parser;
    mo_clm920rv3_t *clm920rv3 = os_container_of(module, mo_clm920rv3_t, parent);

    char resp_buff[CLM920RV3_RESP_DEF_SIZE] = {0};

    at_resp_t resp = {.buff = resp_buff, .buff_size = sizeof(resp_buff), .timeout = 150 * OS_TICK_PER_SECOND};

    char remote_ip[IPADDR_MAX_STR_LEN + 1] = {0};

    strncpy(remote_ip, inet_ntoa(addr), IPADDR_MAX_STR_LEN);

    os_uint32_t event = SET_EVENT(netconn->connect_id, CLM920RV3_EVENT_CONN_OK | CLM920RV3_EVENT_CONN_FAIL);

    os_event_recv(&clm920rv3->netconn_evt, event, OS_EVENT_OPTION_OR | OS_EVENT_OPTION_CLEAR, OS_NO_WAIT, OS_NULL);

    os_err_t result = OS_EOK;

    clm920rv3_lock(&clm920rv3->netconn_lock);

    switch (netconn->type)
    {
    case NETCONN_TYPE_TCP:
        result = at_parser_exec_cmd(parser,
                                    &resp,
                                    "AT+QIPOPEN=1,%d,\"TCP\",\"%s\",%d,0,1",
                                    netconn->connect_id,
                                    remote_ip,
                                    port);
        break;
    case NETCONN_TYPE_UDP:
        result = at_parser_exec_cmd(parser,
                                    &resp,
                                    "AT+QIPOPEN=1,%d,\"UDP\",\"%s\",%d,0,1",
                                    netconn->connect_id,
                                    remote_ip,
                                    port);
        break;
    default:
        ERROR("Module %s connect to %s:%d failed!", module->name, remote_ip, port);
        result = OS_ERROR;
        goto __exit;
    }

    if (result != OS_EOK)
    {
        goto __exit;
    }

    /* AT manual ref:5s, guide manual ref:12s */
    result = os_event_recv(&clm920rv3->netconn_evt,
                           SET_EVENT(netconn->connect_id, 0),
                           OS_EVENT_OPTION_OR | OS_EVENT_OPTION_CLEAR,
                           12 * OS_TICK_PER_SECOND,
                           OS_NULL);
    if (result != OS_EOK)
    {
        ERROR("Module %s netconn id %d wait connect event timeout!", module->name, netconn->connect_id);
        goto __exit;
    }

    result = os_event_recv(&clm920rv3->netconn_evt,
                           CLM920RV3_EVENT_CONN_OK | CLM920RV3_EVENT_CONN_FAIL,
                           OS_EVENT_OPTION_OR | OS_EVENT_OPTION_CLEAR,
                           OS_TICK_PER_SECOND,
                           &event);
    if (result != OS_EOK)
    {
        ERROR("Module %s netconn id %d wait connect result timeout!", module->name, netconn->connect_id);
        goto __exit;
    }

    if (event & CLM920RV3_EVENT_CONN_FAIL)
    {
        result = OS_ERROR;
        ERROR("Module %s netconn id %d connect failed!", module->name, netconn->connect_id);
    }

__exit:
    clm920rv3_unlock(&clm920rv3->netconn_lock);

    if (OS_EOK == result)
    {
        ip_addr_copy(netconn->remote_ip, addr);
        netconn->remote_port = port;
        netconn->stat        = NETCONN_STAT_CONNECT;

        DEBUG("Module %s connect to %s:%d successfully!", module->name, remote_ip, port);
    }
    else
    {
        ERROR("Module %s connect to %s:%d failed!", module->name, remote_ip, port);
    }

    return result;
}

os_size_t clm920rv3_netconn_send(mo_object_t *module, mo_netconn_t *netconn, const char *data, os_size_t size)
{
    at_parser_t *parser     = &module->parser;
    os_err_t     result     = OS_EOK;
    os_size_t    sent_size  = 0;
    os_size_t    curr_size  = 0;
    os_int32_t   connect_id = CLM920RV3_CONN_ID_NULL;
    os_size_t    cnt        = 0;

    mo_clm920rv3_t *clm920rv3 = os_container_of(module, mo_clm920rv3_t, parent);

    clm920rv3_lock(&clm920rv3->netconn_lock);

    char resp_buff[CLM920RV3_RESP_DEF_SIZE] = {0};

    at_resp_t resp = {.buff      = resp_buff,
                      .buff_size = sizeof(resp_buff),
                      .timeout   = 10 * OS_TICK_PER_SECOND};

    while (sent_size < size)
    {
        if (netconn->stat != NETCONN_STAT_CONNECT)
        {
            ERROR("Module %s netconn %d isn't in the CONNECT state, send data fail", parser->name, netconn->connect_id);
            result = OS_ERROR;
            goto __exit;
        }

        if (size - sent_size < CLM920RV3_SEND_MAX_SIZE)
        {
            curr_size = size - sent_size;
        }
        else
        {
            curr_size = CLM920RV3_SEND_MAX_SIZE;
        }

        at_parser_set_end_mark(parser, ">", 1);

        result = at_parser_exec_cmd(parser, &resp, "AT+QIPSEND=%d,%d", netconn->connect_id, curr_size);
        if (result != OS_EOK)
        {
            goto __exit;
        }

        if (at_parser_send(parser, data + sent_size, curr_size) != curr_size)
        {
            result = OS_ERROR;
            ERROR("Drv or hardware send data fail, some data is missing.\r\n");
            goto __exit;
        }

        at_parser_set_end_mark(parser, OS_NULL, 0);

        result = at_parser_exec_cmd(parser, &resp, "");
        if (result != OS_EOK)
        {
            goto __exit;
        }

        if (at_resp_get_data_by_kw(&resp, "+QIPSEND:", "+QIPSEND: %d,%d", &connect_id, &cnt) <= 0 && cnt != curr_size)
        {
            goto __exit;
        }

        sent_size += curr_size;
    }

__exit:

    at_parser_set_end_mark(parser, OS_NULL, 0);

    clm920rv3_unlock(&clm920rv3->netconn_lock);

    if (result != OS_EOK)
    {
        ERROR("Module %s connect id %d send %d bytes data failed", module->name, netconn->connect_id, size);
        return 0;
    }

    return sent_size;
}

static void urc_connect_func(struct at_parser *parser, const char *data, os_size_t size)
{
    OS_ASSERT(OS_NULL != parser);
    OS_ASSERT(OS_NULL != data);

    mo_object_t *module = os_container_of(parser, mo_object_t, parser);
    mo_clm920rv3_t *clm920rv3 = os_container_of(module, mo_clm920rv3_t, parent);

    os_int32_t connect_id = 0;
    os_int32_t result     = 0;

    sscanf(data, "+QIPOPEN: %d,%d", &connect_id , &result);

    if (0 == result)
    {
        os_event_send(&clm920rv3->netconn_evt, SET_EVENT(connect_id, CLM920RV3_EVENT_CONN_OK));
    }
    else
    {
        os_event_send(&clm920rv3->netconn_evt, SET_EVENT(connect_id, CLM920RV3_EVENT_CONN_FAIL));
    }
}

os_err_t clm920rv3_netconn_gethostbyname(mo_object_t *module, const char *domain_name, ip_addr_t *addr)
{
    /* CLM920RV3 gethostbyname function just for test use, not officially support for now. */
    OS_ASSERT(OS_NULL != domain_name);
    OS_ASSERT(OS_NULL != addr);

    at_parser_t *parser = &module->parser;
    os_err_t     result = OS_EOK;
    char resp_buff[CLM920RV3_RESP_DEF_SIZE] = {0};

    at_resp_t resp = {.buff = resp_buff, .buff_size = sizeof(resp_buff), .timeout = OS_TICK_PER_SECOND};
    mo_clm920rv3_t *clm920rv3 = os_container_of(module, mo_clm920rv3_t, parent);

    clm920rv3_lock(&clm920rv3->netconn_lock);

    os_event_recv(&clm920rv3->netconn_evt,
                  CLM920RV3_EVENT_DOMAIN_OK,
                  OS_EVENT_OPTION_OR | OS_EVENT_OPTION_CLEAR,
                  OS_NO_WAIT,
                  OS_NULL);

    clm920rv3->netconn_data = addr;

    result = at_parser_exec_cmd(parser, &resp, "AT+CDNS=\"%s\"", domain_name);
    if (result != OS_EOK)
    {
        goto __exit;
    }

    result = os_event_recv(&clm920rv3->netconn_evt,
                           CLM920RV3_EVENT_DOMAIN_OK,
                           OS_EVENT_OPTION_OR | OS_EVENT_OPTION_CLEAR,
                           (3 + 1) * OS_TICK_PER_SECOND,
                           OS_NULL);

__exit:
    clm920rv3->netconn_data = OS_NULL;

    clm920rv3_unlock(&clm920rv3->netconn_lock);

    return result;
}

static void urc_close_func(struct at_parser *parser, const char *data, os_size_t size)
{
    OS_ASSERT(OS_NULL != parser);
    OS_ASSERT(OS_NULL != data);

    os_int32_t connect_id = 0;

    sscanf(data, "+QIPCLOSEURC: %d", &connect_id);

    mo_object_t *module = os_container_of(parser, mo_object_t, parser);

    mo_netconn_t *netconn = clm920rv3_get_netconn_by_id(module, connect_id);
    if (OS_NULL == netconn)
    {
        ERROR("Module %s receive error close urc data of connect %d", module->name, connect_id);
        return;
    }

    WARN("Module %s receive close urc data of connect %d", module->name, connect_id);

    mo_netconn_pasv_close_notice(netconn);

    return;
}

static void urc_recv_func(struct at_parser *parser, const char *data, os_size_t size)
{
    OS_ASSERT(OS_NULL != parser);
    OS_ASSERT(OS_NULL != data);

    os_int32_t connect_id = 0;
    os_int32_t data_size  = 0;

    sscanf(data, "RECV FROM: %d,%*[^,],%*d,%d", &connect_id, &data_size);

    os_int32_t timeout = data_size > 10 ? data_size : 10;

    mo_object_t *module = os_container_of(parser, mo_object_t, parser);

    mo_netconn_t *netconn = clm920rv3_get_netconn_by_id(module, connect_id);
    if (OS_NULL == netconn)
    {
        ERROR("Module %s receive error recv urc data of connect %d", module->name, connect_id);
        return;
    }

    char *recv_buff    = os_calloc(1, data_size);
    char  temp_buff[8] = {0};
    if (recv_buff == OS_NULL)
    {
        /* read and clean the coming data */
        ERROR("alloc recv buff %d bytes fail, no enough memory", data_size);
        os_size_t temp_size    = 0;
        while (temp_size < data_size)
        {
            if (data_size - temp_size > sizeof(temp_buff))
            {
                at_parser_recv(parser, temp_buff, sizeof(temp_buff), timeout);
            }
            else
            {
                at_parser_recv(parser, temp_buff, data_size - temp_size, timeout);
            }
            temp_size += sizeof(temp_buff);
        }

        /* handle "\r\n" */
        at_parser_recv(parser, temp_buff, 2, timeout);

        return;
    }

    if (at_parser_recv(parser, recv_buff, data_size, timeout) != data_size)
    {
        ERROR("Module %s netconnt id %d recv %d bytes data failed!", module->name, netconn->connect_id, data_size);
        os_free(recv_buff);

        return;
    }
    at_parser_recv(parser, temp_buff, 2, timeout);

    mo_netconn_data_recv_notice(netconn, recv_buff, data_size);

    return;
}

static void urc_pdpact_func(struct at_parser *parser, const char *data, os_size_t size)
{
    OS_ASSERT(OS_NULL != parser);
    OS_ASSERT(OS_NULL != data);

    mo_object_t    *module    = os_container_of(parser, mo_object_t, parser);
    mo_clm920rv3_t *clm920rv3 = os_container_of(module, mo_clm920rv3_t, parent);

    char ip[IPADDR_MAX_STR_LEN +1] = {0};
    os_int32_t context_id   = 0;
    os_int32_t context_type = 0;

    if (0 >= sscanf(data, "+QIPACTURC: %d,%d,\"%[^\"]", &context_id, &context_type, ip))
    {
        ERROR("%s get QIPACTURC info failed.", __func__);
        return;
    }

    if (0 == strcmp("0.0.0.0", ip))
    {
        WARN("context (%d) is deactivated.", context_id);
    }
    else
    {
        os_event_send(&clm920rv3->netconn_evt, CLM920RV3_EVENT_PDP_ACT);
        INFO("%s context (%d) is activated.", __func__, context_id);
    }
}

static void urc_cdns_func(struct at_parser *parser, const char *data, os_size_t size)
{
    OS_ASSERT(OS_NULL != parser);
    OS_ASSERT(OS_NULL != data);

    mo_object_t *module = os_container_of(parser, mo_object_t, parser);
    mo_clm920rv3_t *clm920rv3 = os_container_of(module, mo_clm920rv3_t, parent);

    int j = 0;

    for (int i = 0; i < size; i++)
    {
        if (*(data + i) == '.')
            j++;
    }
    /* There would be several dns result, we just pickup one */
    if (3 == j)
    {
        char recvip[IPADDR_MAX_STR_LEN + 1] = {0};

        sscanf(data, "+CDNS: %s", recvip);
        recvip[IPADDR_MAX_STR_LEN] = '\0';

        inet_aton(recvip, (ip_addr_t *)clm920rv3->netconn_data);

        os_event_send(&clm920rv3->netconn_evt, CLM920RV3_EVENT_DOMAIN_OK);
    }
    else
    {
        DEBUG("Not required dns URC data, not processed");
    }
}

static at_urc_t gs_urc_table[] = {
    {.prefix = "+QIPOPEN:",     .suffix = "\r\n", .func = urc_connect_func},
    {.prefix = "+QIPCLOSEURC:", .suffix = "\r\n", .func = urc_close_func  },
    {.prefix = "+QIPACTURC:",   .suffix = "\r\n", .func = urc_pdpact_func },
    {.prefix = "RECV FROM:",    .suffix = "\r\n", .func = urc_recv_func   },
    {.prefix = "+CDNS:",        .suffix = "\r\n", .func = urc_cdns_func   },
};

os_err_t clm920rv3_netconn_init(mo_clm920rv3_t *module)
{
    /* Init module netconn array */
    memset(module->netconn, 0, sizeof(module->netconn));
    for (int i = 0; i < CLM920RV3_NETCONN_NUM; i++)
    {
        module->netconn[i].connect_id = i + 1;
    }

    /* Set netconn urc table */
    at_parser_t *parser = &(module->parent.parser);
    at_parser_set_urc_table(parser, gs_urc_table, sizeof(gs_urc_table) / sizeof(gs_urc_table[0]));

    /* active pdp context */
    return clm920rv3_pdp_act(module);
}

#endif /* CLM920RV3_USING_NETCONN_OPS */
