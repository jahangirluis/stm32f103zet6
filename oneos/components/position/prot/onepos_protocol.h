/**
 ***********************************************************************************************************************
 * Copyright (c)2020, China Mobile Communications Group Co.,Ltd.
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
 * @file        onepos_protocol.h
 *
 * @revision
 * Date         Author          Notes
 * 2020-07-08   OneOs Team      First Version
 ***********************************************************************************************************************
 */
#ifndef __ONEPOS_PROTOCOL_H__
#define __ONEPOS_PROTOCOL_H__
#include <MQTTClient.h>
#include "onepos_common.h"
/**
 ***********************************************************************************************************************
 * @def         ONEPOS_MQTT_VERSION
 *
 * @brief       mqtt protocol version
 *
 * @param       3       3 = version 3.1
 * @param       4       4 = version 3.1.1
 ***********************************************************************************************************************
 */
#define ONEPOS_MQTT_VERSION 4
/**
 ***********************************************************************************************************************
 * @def         ONEPOS_PLATFORM_ADDR
 *
 * @brief       onepos mqtt server address
 ***********************************************************************************************************************
 */
#define ONEPOS_PLATFORM_ADDR  "218.201.45.140"

/**
 ***********************************************************************************************************************
 * @def         ONEPOS_PLATFORM_POST
 *
 * @brief       onepos mqtt server port
 ***********************************************************************************************************************
 */
#define ONEPOS_PLATFORM_PORT 11883
/**
 ***********************************************************************************************************************
 * @def         ONEPOS_PLATFORM_COMM_TIMEOUT
 *
 * @brief       Communication timeout with the onepos platform(unit : millsecond)
 ***********************************************************************************************************************
 */
#define ONEPOS_PLATFORM_COMM_TIMEOUT 2000
/**
 ***********************************************************************************************************************
 * @def         ONEPOS_MQTT_COMM_ALIVE_TIME
 * @brief       keep alive time while mqtt communication
 ***********************************************************************************************************************
 */
#define ONEPOS_MQTT_COMM_ALIVE_TIME 50u
/**
 ***********************************************************************************************************************
 * @def         ONEPOS_MQTT_COMM_QOS
 *
 * @brief       Quality of Service while mqtt communication
 *
 * @param       QOS0       At most once
 * @param       QOS1       At least once
 * @param       QOS2       Once and only once
 ***********************************************************************************************************************
 */
#define ONEPOS_MQTT_COMM_QOS QOS0

#define ONEPOS_MQTT_RECONN_DELAY 1000   

#define POS_TOPIC_PRE "pos/"

#if defined(ONEPOS_DEVICE_ID) && defined(ONEPOS_PASSWORD)
#define ONEPOS_DEVICE_ID_LEN            20
#define ONEPOS_PASSWORD_LEN             32
#define ONEPOS_CLIENT_ID_LEN            ONEPOS_DEVICE_ID_LEN + 15
#define ONEPOS_MQTT_TOPIC_STRLEN        50

/**
 ***********************************************************************************************************************
 * @def         ONEPOS_MQTT_CLIENT_ID_SUFF
 *
 * @brief       MQTT Client ID suffix
 ***********************************************************************************************************************
 */
#define ONEPOS_MQTT_CLIENT_ID_SUFF "@md5@1599644924"

#else
#error "Pls setting the usrname and password in ENV tools."
#endif

/**
 ***********************************************************************************************************************
 * @enum        onepos_conf_err_code_t
 *
 * @brief       onepos config error code
 ***********************************************************************************************************************
 */
typedef enum
{
    ONEPOS_CONFIG_SUCC = 0,
    ONEPOS_CONFIG_FAIL = 10001
} onepos_conf_err_code_t;

/**
 ***********************************************************************************************************************
 * @enum        onepos_comm_err_code_t
 *
 * @brief       onepos Supported Communication Error Code
 ***********************************************************************************************************************
 */
typedef enum
{
    ONEPOS_COMM_SUCC     = 0,
    ONEPOS_NULL_POSITION = 10000,
    ONEPOS_COMM_FAIL     = 10010,
    ONEPOS_OVER_LIMIT    = 11000,
    /* Add others communication error code */
} onepos_comm_err_code_t;

typedef struct onepos_prot_sub
{
    char            topic_name[ONEPOS_MQTT_TOPIC_STRLEN];
    messageHandler topic_cb;
}prot_sub_t;

typedef struct onepos_prot 
{
    char                    name[OS_NAME_MAX];
    os_uint32_t             sub_topic_num;
    prot_sub_t             *prot_sub_topics;
    Network                 network;
    MQTTClient              client; 
    MQTTPacket_connectData  con_data;
    char                    client_id[ONEPOS_CLIENT_ID_LEN];
    char*                   rec_buff;
    char*                   send_buff;
    os_task_t              *rec_task;
    os_mutex_t             *lock;
    Timer                  *rec_timer;
} onepos_prot_t;

#define PROT_IS_CONN(prot) (0 == MQTTIsConnected(&(prot->client)) ? OS_FALSE : OS_TRUE)

#define ONEPOS_WAIT_MQTT_COMM_BUSY    20        // ms
#define ONEPOS_WAIT_MQTT_READY        50        // ms
#define ONEPOS_WAIT_PUB_MSG_TIMEOUT   30        // ms

#define ONEPOS_REC_TASK_PRIORITY      24
#define ONEPOS_REC_TASK_TIMESLICE     3
#define ONEPOS_REC_TASK_STACK_SIZE    2048

extern os_err_t onepos_prot_deinit(onepos_prot_t* prot);
extern os_err_t onepos_prot_connect(onepos_prot_t* prot);
extern os_err_t onepos_prot_destroy(onepos_prot_t* prot);
extern os_err_t onepos_prot_disconnect(onepos_prot_t* prot);
extern onepos_prot_t* onepos_prot_creat(const char* prot_name);
extern os_err_t onepos_prot_init(onepos_prot_t* prot, char* dev_id, char* passwd);
extern os_err_t onepos_init_conn_data(onepos_prot_t* prot, char* dev_id, char* passwd);
extern os_err_t onepos_prot_add_topic(onepos_prot_t* prot, const os_uint32_t num, prot_sub_t *topics);
extern os_err_t onepos_prot_remove_topic(onepos_prot_t* prot, const os_uint32_t num, prot_sub_t *topics);
extern os_err_t onepos_mqtt_msg_publish(onepos_prot_t *prot, const char *topic, char *buff, os_size_t len);
#endif /* __ONEPOS_PROTOCOL_H__ */
