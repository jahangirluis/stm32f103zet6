/**
 ***********************************************************************************************************************
 * Copyright (c)2020, China Mobile Communications Group Co.,Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *use this file except in compliance with the License. You may obtain a copy of
 *the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 *distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *License for the specific language governing permissions and limitations under
 *the License.
 *
 * @file        onepos_protocol.c
 *
 * @brief       collect information of position to offer called by upper
 *
 * @details
 *
 * @revision
 * Date         Author          Notes
 * 2020-07-07   OneOs Team      First Version   
 ***********************************************************************************************************************
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <os_sem.h>
#include <os_task.h>
#include <os_memory.h>
#include <os_assert.h>
#include <cJSON.h>
#include "onepos_protocol.h"

#define ONEPOS_LOG_TAG "onepos.prot"
#define ONEPOS_LOG_LVL ONEPOS_LOG_INFO
#include <onepos_log.h>

/**
 ***********************************************************************************************************************
 * @brief           add subscription topic onepos protocol
 *
 * @param[in]       prot             onepos protocol
 * @param[in]       num              number of topic
 * @param[in]       topics           topics to add
 *
 * @return          os_err_t         add succ or failed
 ***********************************************************************************************************************
 */
os_err_t onepos_prot_add_topic(onepos_prot_t* prot, const os_uint32_t num, prot_sub_t *topics)
{
    prot_sub_t *ptopic = OS_NULL;

    OS_ASSERT(OS_NULL != prot);
    OS_ASSERT(OS_NULL != topics);

	if(num == 0)
	{
		ONEPOS_LOG_W("add 0 topic, adrr : 0x%x", topics);
		return OS_EOK;
	}
	
    ptopic = (prot_sub_t *)os_calloc(num + prot->sub_topic_num, sizeof(prot_sub_t));
    if(OS_NULL == ptopic)
    {
        ONEPOS_LOG_E("failed to creat ptopic.");
        return OS_ENOMEM;
    }
    else
    {
        os_mutex_recursive_lock(prot->lock, OS_WAIT_FOREVER);
		/* copy old topics */
        memcpy((void*)ptopic, (void*)(prot->prot_sub_topics), prot->sub_topic_num * sizeof(prot_sub_t));
		
		/* free old topics */
		if(OS_NULL != prot->prot_sub_topics)
			os_free((void*)prot->prot_sub_topics);
		
		/* add new topics */
        memcpy((void*)(ptopic + prot->sub_topic_num * sizeof(prot_sub_t)), (void*)topics, num * sizeof(prot_sub_t));
        
		prot->sub_topic_num += num;
		
        prot->prot_sub_topics = ptopic;
        os_mutex_recursive_unlock(prot->lock);
    }

	/* subscrip topics */
    for(os_uint32_t i = 0; i < prot->sub_topic_num; i ++)
    {
        os_mutex_recursive_lock(prot->lock, OS_WAIT_FOREVER);
        if (MQTTSubscribe(&prot->client, prot->prot_sub_topics[i].topic_name,
                    ONEPOS_MQTT_COMM_QOS, prot->prot_sub_topics[i].topic_cb) != SUCCESS)
        {
            os_mutex_recursive_unlock(prot->lock);
            ONEPOS_LOG_E("subscrip topic : %s failed.", prot->prot_sub_topics[i].topic_name);
            goto __exit;
        }
        os_mutex_recursive_unlock(prot->lock);
    }

    return OS_EOK;

    __exit:
        onepos_prot_disconnect(prot);
        return OS_ERROR;
	
}

/**
 ***********************************************************************************************************************
 * @brief           remove subscription topic onepos protocol
 *
 * @param[in]       prot             onepos protocol
 * @param[in]       num              number of topic
 * @param[in]       topics           topics to add
 *
 * @return          os_err_t         remove succ or failed
 ***********************************************************************************************************************
 */
os_err_t onepos_prot_remove_topic(onepos_prot_t* prot, const os_uint32_t num, prot_sub_t *topics)
{
    os_uint32_t index  = 0;
    prot_sub_t *ptopic = OS_NULL;

    OS_ASSERT(OS_NULL != prot);
    OS_ASSERT(OS_NULL != topics);

	if(num == 0)
	{
		ONEPOS_LOG_W("remove 0 topic, adrr : 0x%x", topics);
		return OS_EOK;
	}
	
	/* unsubscrip all topics */
    if(num == prot->sub_topic_num)
    {
        for(os_uint32_t i = 0; i < num; i ++)
        {
            os_mutex_recursive_lock(prot->lock, OS_WAIT_FOREVER);
            if (MQTTUnsubscribe(&prot->client, topics[i].topic_name) != SUCCESS)
            {
                ONEPOS_LOG_E("unsubscrip topic : %s failed.", topics[i].topic_name);
            }
            os_mutex_recursive_unlock(prot->lock);
        }
        os_mutex_recursive_lock(prot->lock, OS_WAIT_FOREVER);
        os_free(prot->prot_sub_topics);
        os_mutex_recursive_unlock(prot->lock);
    }
    else
    {
        ptopic = (prot_sub_t *)os_calloc(prot->sub_topic_num - num, sizeof(prot_sub_t));
        if(OS_NULL == ptopic)
        {
            ONEPOS_LOG_E("failed to creat ptopic.");
            return OS_ENOMEM;
        }
        else
        {
            for(os_uint32_t i = 0; i < prot->sub_topic_num; i ++)
            {
                for(os_uint32_t j = 0; j < num; j ++)
                {
                    if(0 != strcmp(prot->prot_sub_topics[i].topic_name, topics[j].topic_name))
                    {
                        memcpy(&ptopic[index ++], &prot->prot_sub_topics[i], sizeof(prot_sub_t));
                    }
                    else
                    {
                         if (MQTTUnsubscribe(&prot->client, topics[i].topic_name) != 0)
                        {
                            ONEPOS_LOG_E("unsubscrip topic : %s failed.", topics[i].topic_name);
                            goto __exit;
                        }
                    }
                }
            }

            os_mutex_recursive_lock(prot->lock, OS_WAIT_FOREVER);
            /* free old space */
            os_free(prot->prot_sub_topics);
            prot->prot_sub_topics = ptopic;
            prot->sub_topic_num = index;
            os_mutex_recursive_unlock(prot->lock);
        }
    }
    return OS_EOK;

    __exit:
        onepos_prot_disconnect(prot);
        return OS_ERROR;
	
}

extern int cycle(MQTTClient* c, Timer* timer);
static void onepos_prot_rec_task_func(void *parameter)
{
    onepos_prot_t *prot  = (onepos_prot_t *)parameter;
    Timer*         timer = prot->rec_timer;
	
    while (1)
    {
        if (PROT_IS_CONN(prot))
        {
            os_mutex_recursive_lock(prot->lock, OS_WAIT_FOREVER);
            TimerCountdownMS(timer, ONEPOS_WAIT_PUB_MSG_TIMEOUT); 
            cycle(&prot->client, timer);
            os_mutex_recursive_unlock(prot->lock);
        }
        else
        {
            os_task_msleep(ONEPOS_WAIT_MQTT_READY);
        }
    }
}

/**
 ***********************************************************************************************************************
 * @brief           onepos protocol creat
 *
 * @param[in]       prot_name        onepos protocol name
 *
 * @return          onepos_prot_t    
 ***********************************************************************************************************************
 */
onepos_prot_t* onepos_prot_creat(const char* prot_name)
{
    onepos_prot_t *prot = OS_NULL;
    char           name[OS_NAME_MAX] = {0,};

    OS_ASSERT(OS_NULL != prot_name);

    prot = (onepos_prot_t*)os_calloc(1, sizeof(onepos_prot_t));
    if(OS_NULL == prot)
    {
        ONEPOS_LOG_E("failed to creat onepos_prot.");
        goto __exit;
    }
    else
    {
        memcpy(prot->name, prot_name, OS_NAME_MAX);

        prot->send_buff = (char*)os_calloc(1, ONEPOS_COMM_SEND_BUFF_LEN);
        if(OS_NULL == prot->send_buff)
        {
            ONEPOS_LOG_E("failed to creat send buffer.");
            goto __exit;
        }

        prot->rec_buff = (char*)os_calloc(1, ONEPOS_COMM_REC_BUFF_LEN);
        if(OS_NULL == prot->rec_buff)
        {
            ONEPOS_LOG_E("failed to creat receive buffer.");
            goto __exit;
        }

        os_snprintf(name, OS_NAME_MAX, "%s_prot", prot_name);
        if (OS_NULL == os_task_find(name))
        {
            prot->rec_timer = (Timer*)os_calloc(1, sizeof(Timer));
            if(OS_NULL == prot->rec_timer)
            {
                ONEPOS_LOG_E("failed to creat receive timer.");
                goto __exit;
            }

            prot->lock = os_mutex_create(name, OS_TRUE);
            if(OS_NULL == prot->lock)
            {
                ONEPOS_LOG_E("failed to creat lock.");
                goto __exit;
            }
            
            prot->rec_task =
                os_task_create(name, onepos_prot_rec_task_func,
                            (void *)(prot), ONEPOS_REC_TASK_STACK_SIZE,
                            ONEPOS_REC_TASK_PRIORITY);
            if(OS_NULL == prot->lock)
            {
                ONEPOS_LOG_E("failed to creat task.");
                goto __exit;
            }

            os_task_set_time_slice(prot->rec_task, ONEPOS_REC_TASK_TIMESLICE);
        }
        else
        {
            ONEPOS_LOG_E("repetition to creat %s protocol.", prot_name);
            goto __exit;
        }  

        return prot;
    }

    __exit:
        if(OS_NULL != prot->send_buff)
        {
            os_free(prot->send_buff);
            prot->send_buff = (char*)OS_NULL;
        }

        if(OS_NULL != prot->rec_buff)
        {   
            os_free(prot->rec_buff);
            prot->rec_buff = (char*)OS_NULL;
        }
        
        if(OS_NULL != prot->rec_timer)
        {
            os_free(prot->rec_timer);
            prot->rec_timer = (Timer*)OS_NULL;
        }

        if (prot->lock)
		{
            os_mutex_destroy(prot->lock);
			prot->lock = (os_mutex_t*)OS_NULL;
		}	
        if (prot->rec_task)
		{
            os_task_destroy(prot->rec_task);
			prot->rec_task = (os_task_t*)OS_NULL;
		}
		if(OS_NULL != prot)
        {
            os_free(prot);
            prot = (onepos_prot_t*)OS_NULL;
        }
        return (onepos_prot_t*)OS_NULL;
}

/**
 ***********************************************************************************************************************
 * @brief           onepos protocol destroy
 *
 * @param[in]       prot            onepos protocol
 *
 * @return          os_err_t        destroy succ or failed
 ***********************************************************************************************************************
 */
os_err_t onepos_prot_destroy(onepos_prot_t* prot)
{
    OS_ASSERT(OS_NULL != prot);
    
    if(OS_NULL != prot->send_buff)
    {
        os_free(prot->send_buff);
        prot->send_buff = (char*)OS_NULL;
    }

    if(OS_NULL != prot->rec_buff)
    {   
        os_free(prot->rec_buff);
        prot->rec_buff = (char*)OS_NULL;
    }
    
    if(OS_NULL != prot->rec_timer)
    {
        os_free(prot->rec_timer);
        prot->rec_timer = (Timer*)OS_NULL;
    }

    if(prot->prot_sub_topics)
    {
        os_free(prot->prot_sub_topics);
        prot->sub_topic_num = 0;
        prot->prot_sub_topics = OS_NULL;
    }

    if (prot->lock)
    {
        os_mutex_destroy(prot->lock);
        prot->lock = OS_NULL;
    }

    if (prot->rec_task)
    {
        os_task_destroy(prot->rec_task);
        prot->rec_task = OS_NULL;
    }

    if(OS_NULL != prot)
    {
        os_free(prot);
        prot = (onepos_prot_t*)OS_NULL;
    }

    return OS_EOK;
}

/**
 ***********************************************************************************************************************
 * @brief           init onepos protocol connection data
 *
 * @param[in]       prot            onepos protocol
 * @param[in]       dev_id          onepos device id
 * @param[in]       passwd          onepos device password
 *
 * @return          os_err_t        init succ or failed
 ***********************************************************************************************************************
 */
os_err_t onepos_init_conn_data(onepos_prot_t* prot, char* dev_id, char* passwd)
{
    OS_ASSERT(OS_NULL != prot);
    OS_ASSERT(OS_NULL != dev_id);
	OS_ASSERT(OS_NULL != passwd);

    memset(prot->client_id, 0, ONEPOS_CLIENT_ID_LEN + 1);
    snprintf(prot->client_id, ONEPOS_CLIENT_ID_LEN + 1, "%s%s", dev_id,
            ONEPOS_MQTT_CLIENT_ID_SUFF);

    prot->con_data.MQTTVersion       = ONEPOS_MQTT_VERSION;
    prot->con_data.keepAliveInterval = ONEPOS_MQTT_COMM_ALIVE_TIME;
    prot->con_data.clientID.cstring  = prot->client_id;
    prot->con_data.username.cstring  = dev_id;
    prot->con_data.password.cstring  = passwd;
    prot->con_data.cleansession      = 1;
    prot->con_data.willFlag          = 0;
	
	return OS_EOK;
}

/**
 ***********************************************************************************************************************
 * @brief           init onepos protocol
 *
 * @param[in]       prot            onepos protocol
 * @param[in]       dev_id          onepos device id
 * @param[in]       passwd          onepos device password
 *
 * @return          os_err_t        init succ or failed
 ***********************************************************************************************************************
 */
os_err_t onepos_prot_init(onepos_prot_t* prot, char* dev_id, char* passwd)
{
    os_int32_t rc = -1;

	OS_ASSERT(OS_NULL != prot);
    OS_ASSERT(OS_NULL != dev_id);
	OS_ASSERT(OS_NULL != passwd);
	
    /* Init connect data */
    if(OS_EOK != onepos_init_conn_data(prot, dev_id, passwd))
    {
        ONEPOS_LOG_E("failed to init network.");
        return OS_ERROR;
    }

    /* Init network */
    rc = MQTTNetworkInit(&prot->network, ONEPOS_PLATFORM_ADDR,
                        ONEPOS_PLATFORM_PORT, OS_NULL);
    if(-1 == rc)
    {
        ONEPOS_LOG_E("failed to init network.");
        return OS_EIO;
    }
    prot->network.handle = (uintptr_t)-1;

    MQTTClientInit(&prot->client, &prot->network,
                    ONEPOS_PLATFORM_COMM_TIMEOUT, (unsigned char*)prot->send_buff,
                    ONEPOS_COMM_SEND_BUFF_LEN, (unsigned char*)prot->rec_buff,
                    ONEPOS_COMM_REC_BUFF_LEN);

    /* Init receive timer */
    TimerInit(prot->rec_timer);

    /* Start receive task */
    os_task_startup(prot->rec_task);

    return OS_EOK;
}

/**
 ***********************************************************************************************************************
 * @brief           onepos protocol disconnect
 *
 * @param[in]       prot             onepos protocol
 *
 * @return          os_err_t        disconnect succ or failed
 ***********************************************************************************************************************
 */
os_err_t onepos_prot_disconnect(onepos_prot_t* prot)
{
    OS_ASSERT(OS_NULL != prot);

   if (PROT_IS_CONN(prot))
   {
       for(os_uint32_t i = 0; i < prot->sub_topic_num; i ++)
       {
           os_mutex_recursive_lock(prot->lock, OS_WAIT_FOREVER);
           if (SUCCESS != MQTTUnsubscribe(&prot->client, prot->prot_sub_topics[i].topic_name))
           {
               ONEPOS_LOG_E("unsubscrip topic : %s failed.", prot->prot_sub_topics[i].topic_name);
           }
           os_mutex_recursive_unlock(prot->lock);
       }

        os_mutex_recursive_lock(prot->lock, OS_WAIT_FOREVER);
        MQTTDisconnect(&prot->client);
        memset(prot->send_buff, 0, ONEPOS_COMM_SEND_BUFF_LEN);
        memset(prot->rec_buff, 0, ONEPOS_COMM_REC_BUFF_LEN);
        os_mutex_recursive_unlock(prot->lock);
   }

    if ((uintptr_t)-1 != prot->network.handle && prot->network.disconnect)
    {
        prot->network.disconnect(&prot->network);
    }
	
   prot->network.handle = (uintptr_t)-1;

	return OS_EOK;
}

/**
 ***********************************************************************************************************************
 * @brief           onepos protocol connect
 *
 * @param[in]       prot             onepos protocol
 *
 * @return          os_err_t         connect succ or failed
 ***********************************************************************************************************************
 */
os_err_t onepos_prot_connect(onepos_prot_t* prot)
{
    os_int32_t rc = -1;

    OS_ASSERT(OS_NULL != prot);

    rc = prot->network.connect(&prot->network);
    if (0 != rc)
    {
        ONEPOS_LOG_E("establish network failed, check IP and PORT");
        goto __exit;
    }
    ONEPOS_LOG_D("establish network sucess");

    if ((rc = MQTTConnect(&prot->client, &(prot->con_data))) != 0) 
    {
        ONEPOS_LOG_E("protocol connect failed");
        goto __exit;
    }
    ONEPOS_LOG_D("protocol connected");

    /* subscrip topics */
    for(os_uint32_t i = 0; i < prot->sub_topic_num; i ++)
    {
        os_mutex_recursive_lock(prot->lock, OS_WAIT_FOREVER);
        if (MQTTSubscribe(&prot->client, prot->prot_sub_topics[i].topic_name,
                    ONEPOS_MQTT_COMM_QOS, prot->prot_sub_topics[i].topic_cb) != SUCCESS)
        {
            os_mutex_recursive_unlock(prot->lock);
            ONEPOS_LOG_E("subscrip topic : %s failed.", prot->prot_sub_topics[i].topic_name);
            goto __exit;
        }
        os_mutex_recursive_unlock(prot->lock);
    }

    return OS_EOK;

    __exit:
        onepos_prot_disconnect(prot);
        return OS_ERROR;
}

/**
 ***********************************************************************************************************************
 * @brief           deinit onepos protocol
 *
 * @param[in]       prot             onepos protocol
 *
 * @return          os_err_t        deinit succ or failed
 ***********************************************************************************************************************
 */
os_err_t onepos_prot_deinit(onepos_prot_t* prot)
{
    OS_ASSERT(OS_NULL != prot);

    if (PROT_IS_CONN(prot))
    {
        for(os_uint32_t i = 0; i < prot->sub_topic_num; i ++)
        {
            os_mutex_recursive_lock(prot->lock, OS_WAIT_FOREVER);
            if (SUCCESS != MQTTUnsubscribe(&prot->client, prot->prot_sub_topics[i].topic_name))
            {
                ONEPOS_LOG_E("unsubscrip topic : %s failed.", prot->prot_sub_topics[i].topic_name);
            }
            os_mutex_recursive_unlock(prot->lock);
        }
    }

    os_mutex_recursive_lock(prot->lock, OS_WAIT_FOREVER);
    os_task_suspend(prot->rec_task);
    os_mutex_recursive_unlock(prot->lock);

	memset(prot->send_buff, 0, ONEPOS_COMM_SEND_BUFF_LEN);
	memset(prot->rec_buff, 0, ONEPOS_COMM_REC_BUFF_LEN);

	MQTTClientDeInit(&(prot->client));
	
    TimerRelease(prot->rec_timer);

	return OS_EOK;
}


/**
 ***********************************************************************************************************************
 * @brief          publish massage to topic of mqtt
 *
 * @param[in]       prot             onepos protocol
 * @param[in]       topic            mqtt topic name
 * @param[in]       msg              message will be published
 * @param[in]       len              length message will be published
 *
 * @return          os_err_t
 * @retval          OS_EOK       publish message to the topic is successfully
 * @retval          OS_ERROR     publish message to the topic is failed
 ***********************************************************************************************************************
 */
os_err_t onepos_mqtt_msg_publish(onepos_prot_t *prot, const char *topic, char *buff, os_size_t len)
{
    os_err_t    ret     = OS_EOK;
    os_int32_t  rc      = 0;
    MQTTMessage message;

    OS_ASSERT(OS_NULL != prot);

    message.qos        = ONEPOS_MQTT_COMM_QOS;
    message.retained   = 0;
    message.payload    = buff;
    message.payloadlen = len;

    if (PROT_IS_CONN(prot) != OS_FALSE)
    {
#if defined(ONEPOS_PROTO_DEBUG)
        os_kprintf("prot : %s\r\n topic : %s\r\n len : %d\r\n", prot->name, topic, len);
#endif

        os_mutex_recursive_lock(prot->lock, OS_WAIT_FOREVER);

        rc = MQTTPublish(&prot->client, topic, &message);
        if (0 != rc)
        {
            ret = OS_ERROR;
            ONEPOS_LOG_E("Return code from MQTT publish is %d", rc);
        }

        os_mutex_recursive_unlock(prot->lock);
    }
    else
    {
        ONEPOS_LOG_W("onepos protocol is not concting.");
        ret = OS_ERROR;
    }

    return ret;
}
