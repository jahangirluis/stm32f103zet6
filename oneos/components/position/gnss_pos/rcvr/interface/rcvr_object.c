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
 * @file        rcvr_object.c
 *
 * @brief       gnss recevicer object function
 *
 * @revision
 * Date         Author          Notes
 * 2020-12-15   OneOS Team      First Version
 ***********************************************************************************************************************
 */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <os_task.h>
#include <os_assert.h>
#include <os_mutex.h>
#include <os_memory.h>
#include "onepos_common.h"
#include "rcvr_object.h"

#define ONEPOS_LOG_TAG "onepos.rcvr"
#define ONEPOS_LOG_LVL ONEPOS_LOG_INFO
#include <onepos_log.h>

#ifdef GNSS_USING_RCVR

static os_slist_node_t gs_rcvr_object_list      = {0};
static rcvr_object_t  *gs_rcvr_object_default   = OS_NULL;

static void rcvr_object_list_add(rcvr_object_t *self)
{
    os_schedule_lock();

    os_slist_init(&(self->list));

    if (OS_NULL == gs_rcvr_object_default)
    {
        gs_rcvr_object_default = self;
    }

    /* tail insertion */
    os_slist_add_tail(&(gs_rcvr_object_list), &(self->list));

    os_schedule_unlock();
}

static void rcvr_object_list_del(rcvr_object_t *self)
{
    OS_ASSERT(self != OS_NULL);

    os_slist_node_t *node  = OS_NULL;
    rcvr_object_t   *entry = OS_NULL;

    os_schedule_lock();

    for (node = &gs_rcvr_object_list; node; node = os_slist_next(node))
    {
        entry = os_slist_entry(node, rcvr_object_t, list);
        if (entry == self)
        {
            os_slist_del(&(gs_rcvr_object_list), &(self->list));

            if (gs_rcvr_object_default == self)
            {
                gs_rcvr_object_default = OS_NULL;
            }
            break;
        }
    }

    os_schedule_unlock();
}

static os_err_t rcvr_object_startup(rcvr_object_t *rcvr)
{
	os_err_t result = OS_EOK;
	
    OS_ASSERT(rcvr != OS_NULL);

	#ifdef RCVR_SUPP_PROT
    result = os_task_startup(rcvr->prot_parser->task);
	#endif
	
	return result;
}

static os_err_t rcvr_rx_done(os_device_t *dev, struct os_device_cb_info *info)
{
    rcvr_object_t *rcvr  = (rcvr_object_t*)info->data;
    if(rcvr != OS_NULL)
    {
        os_sem_post(rcvr->prot_parser->notice);
    }

    return OS_EOK;
}

rcvr_object_t *rcvr_object_get_by_dev(const os_device_t *dev)
{
    OS_ASSERT(dev != OS_NULL);

    os_slist_node_t *node  = OS_NULL;
    rcvr_object_t     *entry = OS_NULL;

    if (OS_NULL == gs_rcvr_object_list.next)
    {
        return OS_NULL;
    }

	os_schedule_lock();

    for (node = &gs_rcvr_object_list; node; node = os_slist_next(node))
    {
        entry = os_slist_entry(node, rcvr_object_t, list);
        if (entry && (entry->device == dev))
        {
            os_schedule_unlock();
            return entry;
        }
    }

    os_schedule_unlock();

    return OS_NULL;
}

#ifdef ONEPOS_RCVR_DEBUG
/**
 ***********************************************************************************************************************
 * @brief           dump hex format data to console device
 *
 * @param[in]       name            Name for hex object, it will show on log header
 * @param[in]       buf             Hex buffer
 * @param[in]       size            buffer size
 ***********************************************************************************************************************
 */
#if 0
static void rcvr_print_raw(const char *name, char *buf, os_size_t size)
{
#define __is_print(ch) ((unsigned int)((ch) - ' ') < 127u - ' ')
#define WIDTH_SIZE     32

    for (int i = 0; i < size; i += WIDTH_SIZE)
    {
        os_kprintf("[D/rcvr.prot] %s: %04X-%04X: ", name, i, i + WIDTH_SIZE);
        for (int j = 0; j < WIDTH_SIZE; j++)
        {
            if (i + j < size)
            {
                os_kprintf("%02X ", buf[i + j]);
            }
            else
            {
                os_kprintf("   ");
            }
            if ((j + 1) % 8 == 0)
            {
                os_kprintf(" ");
            }
        }
        os_kprintf("  ");
        for (int j = 0; j < WIDTH_SIZE; j++)
        {
            if (i + j < size)
            {
                os_kprintf("%c", __is_print(buf[i + j]) ? buf[i + j] : '.');
            }
        }
        os_kprintf("\r\n");
    }
}
#endif
#endif

rcvr_object_t *rcvr_object_get_by_name(const char *name)
{
    OS_ASSERT(name != OS_NULL);

    os_slist_node_t *node  = OS_NULL;
    rcvr_object_t     *entry = OS_NULL;

    if (OS_NULL == gs_rcvr_object_list.next)
    {
        return OS_NULL;
    }

    os_schedule_lock();

    for (node = &gs_rcvr_object_list; node; node = os_slist_next(node))
    {
        entry = os_slist_entry(node, rcvr_object_t, list);
        if (entry && (strncmp(entry->name, name, OS_NAME_MAX) == 0))
        {
            os_schedule_unlock();
            return entry;
        }
    }

    os_schedule_unlock();

    return OS_NULL;
}

rcvr_object_t *rcvr_object_get_default(void)
{
    if (OS_NULL == gs_rcvr_object_default)
    {
        ONEPOS_LOG_E("There are no default gnss recevicer in the system now");
    }

    return gs_rcvr_object_default;
}

void rcvr_object_set_default(rcvr_object_t *self)
{
    OS_ASSERT(self != OS_NULL);

    gs_rcvr_object_default = self;
}



#ifdef RCVR_SUPP_PROT
static void rcvr_prot_parse(void *parameter)
{
    char     	        ch                                         = 0xff;
    char                tmp[NMEA_SENTENCE_CHARS_MAX_LEN + 1]       = {0,};
    os_err_t            result                                     = OS_EOK;
    os_uint32_t         rc_num                                     = 0;
    rcvr_prot_t        *rcvr_prot                                  = OS_NULL;
    rcvr_object_t      *rcvr                                       = (rcvr_object_t*)parameter;

    OS_ASSERT(parameter);

    while(1)
    {	
		result = os_sem_wait(rcvr->prot_parser->notice, RCVR_RC_TIMEOUT);
        if(result == OS_EOK)
        {
            while(1 == rcvr_object_read(rcvr, &ch, 1))
            {                
                #ifdef GNSS_NMEA_0183_PROT
                /* 
                    No '$' in nmea sentence, if head is '$', and there is a '$' while receiving name data,
                    will give out the data between two '$'.
                */
                if(ch == NMEA_SENTENCE_START_CHAR)
                {
                    rc_num = 0;
                    tmp[rc_num ++] = ch;
                }
                else
                {
                    if(rc_num < NMEA_SENTENCE_CHARS_MAX_LEN)
                    {
                        tmp[rc_num ++] = ch;
                    }
                    else
                    {
                        rc_num  = 0; 
                        ONEPOS_LOG_E("over temp buff size, but no right nmea protocol sentence.");
                        continue;
                    }
                    if(NMEA_SENTENCE_END_CHAR == ch)
                    {	
                        #ifdef GNSS_NMEA_PROT_DEBUG
                        /* 
                            if using GNSS_NMEA_PROT_DEBUG, will print the sentence as a STRING Format, 
                            so add end char '\0' to the sentence. 
                        */
                        tmp[rc_num ++] = '\0';
                        #endif

                        rcvr_prot = &(rcvr->prot_parser->prot_table[RCVR_NMEA_0183_PROT]);
                        rcvr_prot->parse_func(tmp, (nmea_t*)(rcvr_prot->pos_data), rc_num);
                        #ifdef ONEPOS_RCVR_DEBUG
                        ONEPOS_LOG_I("rcvr : %s", rcvr->name);
                        display_nmea_pos_result((nmea_t*)rcvr_prot->pos_data);
                        #endif
                        rc_num = 0;
                        continue;
                    }
                }
                #endif
            }
        }
        else
        {
            ONEPOS_LOG_W("wait receiver data timeout, please check connect");
        }
   }
}
#endif

/**
 ***********************************************************************************************************************
 * @brief           Lock the recursive lock that protects the operation of the gnss receiver process 
 *
 * @param[in]       rcvr          A pointer to gnss receiver instance
 * 
 * @return          @see os_mutex_recursive_lock.
 ***********************************************************************************************************************
 */
os_err_t rcvr_object_op_lock(rcvr_object_t *rcvr)
{
    return os_mutex_recursive_lock(rcvr->op_lock, OS_WAIT_FOREVER);
}

/**
 ***********************************************************************************************************************
 * @brief           Unlock the recursive lock that protects the operation of the gnss receiver process
 *
 * @param[in]       rcvr          A pointer to gnss receiver instance
 * 
 * @return          @see os_mutex_recursive_unlock.
 ***********************************************************************************************************************
 */
os_err_t rcvr_object_op_unlock(rcvr_object_t *rcvr)
{
    return os_mutex_recursive_unlock(rcvr->op_lock);
}

/**
 ***********************************************************************************************************************
 * @brief           write data to the gnss receiver with the device
 *
 * @param[in]       rcvr            A pointer to gnss receiver instance
 * @param[in]       data            data to write
 * @param[in]       data_len        length of data to write
 * 
 * @return          write succ or faied
 ***********************************************************************************************************************
 */
os_err_t rcvr_object_send(rcvr_object_t *rcvr, const char* data, os_size_t data_len)
{
    os_err_t    result       = OS_EOK;
    os_uint32_t send_num     = 0;
    os_int32_t  re           = -1;
    os_uint32_t sending_num  = 0;
    char*       sending_data = OS_NULL;
    os_int32_t  send_time    = 0;
        
    OS_ASSERT(rcvr != OS_NULL);
    OS_ASSERT(data != OS_NULL);

    send_time = data_len / OS_SERIAL_TX_BUFSZ + 11;

    while(send_num < data_len && send_time > 0)
    {
        if((data_len - send_num > OS_SERIAL_TX_BUFSZ))
        {
            sending_num = OS_SERIAL_TX_BUFSZ;
        }
        else
        {
            sending_num = data_len - send_num;
        }
        
        sending_data = (char*)((os_uint32_t)data + send_num);
        
        rcvr_object_op_lock(rcvr);
        re = os_device_write_nonblock(rcvr->device, 0, sending_data, sending_num);
        rcvr_object_op_unlock(rcvr);

        if(re < 0)
        {
            ONEPOS_LOG_E("send data to receiver failed, already send %d bytes.", send_num);
            result = OS_EIO;
            break;
        }
        else
        {
            send_num += re;
        }
        send_time --;
    }
    if(send_num != data_len)
        ONEPOS_LOG_E("not send all data to receiver, only send %d bytes, all data %d bytes.", send_num, data_len);
    
    return result;
}

/**
 ***********************************************************************************************************************
 * @brief           read data to the gnss receiver into the device
 *
 * @param[in]       rcvr            pointer to gnss receiver instance
 * @param[in]       dst             dst to save data
 * @param[in]       size            length of data to will read
 * 
 * @return          read succ or faied
 ***********************************************************************************************************************
 */
os_size_t rcvr_object_read(rcvr_object_t *rcvr, char* dst, os_size_t size)
{
    os_size_t   result   = OS_EOK;
	
    OS_ASSERT(rcvr != OS_NULL);

    rcvr_object_op_lock(rcvr);
    
	result = os_device_read_nonblock(rcvr->device, 0, dst, size);
	
    rcvr_object_op_unlock(rcvr);

    return result;
}

/**
 ***********************************************************************************************************************
 * @brief           Init an instance of a gnss receiver object
 *
 * @param[in]       self            The pointer to gnss receiver instance
 * @param[in]       name            The gnss receiver instance name
 * @param[in]       device_name     Device name of the gnss receiver
 * @param[in]       buff_size       The buffer size for gnss receiver receive data  
 * 
 * @return          On success, return OS_EOK; on error, return a error code. 
 ***********************************************************************************************************************
 */
os_err_t rcvr_object_init(rcvr_object_t *self, const char *name, const char* device_name)
{
    char         name_buffer[OS_NAME_MAX + 1] = {0};
    os_err_t   	 result   = OS_EOK;
    os_device_t *rcvr_dev = OS_NULL;
	struct os_device_cb_info cb_info = 
    {
        .type = OS_DEVICE_CB_TYPE_RX,
		.data = (void*)self,
        .cb   = rcvr_rx_done,
    };
	
    OS_ASSERT(name != OS_NULL);
    OS_ASSERT(self != OS_NULL);
    OS_ASSERT(device_name != OS_NULL);
	
    memset(self, 0, sizeof(rcvr_object_t));

    /* Init rcvr and op_lock */
	snprintf(name_buffer, OS_NAME_MAX, "%s_lock", name);
	self->op_lock = os_mutex_create(name_buffer, OS_TRUE);
	if(self->op_lock == OS_NULL)
	{
		ONEPOS_LOG_E("%s mutex allocation failed!", name);
        result = OS_ENOMEM;
        goto __exit;
	}
	
    /* Init name */
    if (strlen(name) == 0 || rcvr_object_get_by_name(name) != OS_NULL)
    {
        ONEPOS_LOG_E("Failed init rcvr object, gnss receiver name error");
        return OS_ERROR;
    }

    strncpy(self->name, name, OS_NAME_MAX);

    /* Init device */
    if (strlen(device_name) == 0 || (rcvr_dev = os_device_find(device_name)) == OS_NULL)
    {
        ONEPOS_LOG_E("Failed init rcvr object, device name error");
        return OS_ERROR;
    }
	self->device = rcvr_dev;
	
    #ifdef RCVR_SUPP_PROT
    /* Init prot parser */
    self->prot_parser = (rcvr_prot_parse_t*)os_malloc(sizeof(rcvr_prot_parse_t));
    if(self->prot_parser == OS_NULL)
	{
		ONEPOS_LOG_E("%s_rcvr_prot_parser allocation failed!", name);
        result = OS_ENOMEM;
        goto __exit;
	}
	
	memset(self->prot_parser, 0, sizeof(rcvr_prot_parse_t));
	
    for(os_uint32_t index = 0; index < RCVR_PROT_MAX; index ++)
    {
        switch (index)
        {
			#ifdef GNSS_NMEA_0183_PROT
            case RCVR_NMEA_0183_PROT:
                self->prot_parser->prot_table[RCVR_NMEA_0183_PROT].parse_func = (rcvr_prot_par)nmea_prot_parse;
                self->prot_parser->prot_table[RCVR_NMEA_0183_PROT].pos_data = (void*)os_malloc(sizeof(nmea_t));
                memset((void*)self->prot_parser->prot_table[RCVR_NMEA_0183_PROT].pos_data, 0, sizeof(nmea_t));

                /* add nmea protocol parse complete call back function */
            break;
			#endif
			
            /* Add other protocol */
            default:
                ONEPOS_LOG_E("%s rcvr protocol parser init failed!", name);
                result = OS_ERROR;
                goto __exit;
        }
    }
    snprintf(name_buffer, OS_NAME_MAX, "%s_parser", name);
	self->prot_parser->notice = os_sem_create(name_buffer, 0, 1);
	if(self->prot_parser->notice == OS_NULL)
	{
		ONEPOS_LOG_E("%s_rcvr_prot_parser notice allocation failed!", name);
        result = OS_ENOMEM;
        goto __exit;
	}
	
    self->prot_parser->task = os_task_create(name_buffer,
                                  rcvr_prot_parse,
                                  (void *)self,
                                  RCVR_PROT_PARSE_TASK_STACK_SIZE,
                                  RCVR_PROT_PARSE_TASK_PRIORITY);
    if (self->prot_parser->task == OS_NULL)
    {
        ONEPOS_LOG_E("%s_rcvr_prot_parser task allocation failed!", name);
        result = OS_ENOMEM;
        goto __exit;
    }
	
	result = os_device_open(self->device);
    if (result != OS_EOK)
    {
        goto __exit;
    }

	result = os_device_control(self->device, OS_DEVICE_CTRL_SET_CB, &cb_info);
    if (result != OS_EOK)
    {
        ONEPOS_LOG_E("rcvr set device call back error.");
		goto __exit;
    }
	
    #endif
    rcvr_object_list_add(self);

    rcvr_object_startup(self);

__exit:
    if (result != OS_EOK)
    {   
		if (self->device)
        {
			memset(&cb_info, 0, sizeof(struct os_device_cb_info));
			os_device_control(self->device, OS_DEVICE_CTRL_SET_CB, &cb_info);
            os_device_close(self->device);
        }
		
        #ifdef RCVR_SUPP_PROT
        if(self->prot_parser)
        {
            
            if(OS_NULL != self->prot_parser->task)
            {
                os_task_destroy(self->prot_parser->task);
				self->prot_parser->task = OS_NULL;
            }
			
			if(OS_NULL != self->prot_parser->notice)
			{
				os_sem_destroy(self->prot_parser->notice);
				self->prot_parser->notice = OS_NULL;
			}
			
			if(OS_NULL != self->op_lock)
			{
				os_mutex_destroy(self->op_lock);
				self->op_lock = OS_NULL;
			}
			
			for(os_uint32_t index = 0; index < RCVR_PROT_MAX; index ++)
            {
                if(self->prot_parser->prot_table[index].pos_data)
                {
                    os_free(self->prot_parser->prot_table[index].pos_data);
					self->prot_parser->prot_table[index].pos_data = OS_NULL;
                }
            }
			
            os_free(self->prot_parser);
			self->prot_parser = OS_NULL;
        }
        #endif
    }

    return result;

}

/**
 ***********************************************************************************************************************
 * @brief           Deinit an instance of gnss receiver object
 *
 * @param[in]       rcvr          An instance of gnss receiver object to be deinit
 *
 * @retval          OS_EOK          Deinit successfully
 ***********************************************************************************************************************
 */
os_err_t rcvr_object_deinit(rcvr_object_t *rcvr)
{
	struct os_device_cb_info cb_info = {0,};
	
    OS_ASSERT(rcvr != OS_NULL);

    rcvr_object_list_del(rcvr);

    if (rcvr->device != OS_NULL)
    {
		memset(&cb_info, 0, sizeof(struct os_device_cb_info));
		os_device_control(rcvr->device, OS_DEVICE_CTRL_SET_CB, &cb_info);
        os_device_close(rcvr->device);
    }

    #ifdef RCVR_SUPP_PROT
    if(rcvr->prot_parser)
    {
		if(rcvr->prot_parser->task != OS_NULL)
		{
			os_task_destroy(rcvr->prot_parser->task);
		}
		
		if(OS_NULL != rcvr->prot_parser->notice)
		{
			os_sem_destroy(rcvr->prot_parser->notice);
			rcvr->prot_parser->notice = OS_NULL;
		}
		
		
        for(os_uint32_t index = 0; index < RCVR_PROT_MAX; index ++)
        {
            if(rcvr->prot_parser->prot_table[index].pos_data)
            {
                os_free(rcvr->prot_parser->prot_table[index].pos_data);
            }
        }
		
        os_free(rcvr->prot_parser);
		
		if(OS_NULL != rcvr->op_lock)
		{
			os_mutex_destroy(rcvr->op_lock);
			rcvr->op_lock = OS_NULL;
		}
    }
    #endif
    return OS_EOK;
}

/**
 ***********************************************************************************************************************
 * @brief           Destroy an instance of gnss receiver object
 *
 * @param[in]       rcvr          An instance of gnss receiver object to be destroy锛坢alloced锛�
 ***********************************************************************************************************************
 */

void rcvr_object_destroy(rcvr_object_t *rcvr)
{
    OS_ASSERT(rcvr != OS_NULL);

    os_free(rcvr);
}

#ifdef RCVR_SUPP_RESET
/**
 ***********************************************************************************************************************
 * @brief           Reset operation of the gnss receiver
 *
 * @param[in]       rcvr          An instance of gnss receiver object to be destroy锛坢alloced锛�
 * @param[in]       reset_type    Reset type of gnss receiver
 ***********************************************************************************************************************
 */
os_err_t rcvr_reset(rcvr_object_t *rcvr, rcvr_reset_type_t reset_type)
{
	os_err_t result = OS_EOK;
	
    OS_ASSERT(rcvr != OS_NULL);
    OS_ASSERT(IS_INVALID_RESET_TYPE(reset_type));

	if(rcvr->ops_table[RCVR_RESET_OPS])
	{
		rcvr_object_op_lock(rcvr);
		result = rcvr->ops_table[RCVR_RESET_OPS](rcvr, reset_type);
		rcvr_object_op_unlock(rcvr);
	}
	else
		result = OS_ERROR;
	
	return result;
}
#endif

#ifdef RCVR_SUPP_PROT
/**
 ***********************************************************************************************************************
 * @brief           Get positon result data of the gnss receiver
 *
 * @param[in]       rcvr          An instance of gnss receiver object to be destroy锛坢alloced锛�
 * @param[in]       buff          Buffer to save 
 * @param[in]       buf_size      Size of save buffer 
 * @param[in]       prot_type     Protocol type of gnss receiver
***********************************************************************************************************************
 */
os_err_t get_rcvr_data(rcvr_object_t *rcvr, void* buff, os_size_t buf_size, rcvr_prot_type_t prot_type)
{
    os_err_t  result = OS_EOK;
    void     *data   = OS_NULL;
    OS_ASSERT(rcvr != OS_NULL);
    OS_ASSERT(buff != OS_NULL);
    OS_ASSERT(buf_size > 0);
    OS_ASSERT(IS_INVALID_PROT_TYPE(prot_type));

    data = rcvr->prot_parser->prot_table[prot_type].pos_data;
    if(data)
    {
        rcvr_object_op_lock(rcvr);
        memcpy(buff, data, buf_size);
        rcvr_object_op_unlock(rcvr);
    }
    else
        result = OS_ERROR;

    return result;
}

/* format : ddmm.mmmm */
double rcvr_data_to_wgs84(onepos_com_float_t rcvr_data)
{
	long   ang      = 0;
	double result   = 0.f;
	double min      = 0.0f;
    double value    = tras_loca_float(rcvr_data);

	/* format : ddmm.mmmm */
	ang = (long)value / 100u;
	min = value - (long)(ang * 100u);
	
	result = ang + min / 60.0f;
	
	return result;
}

#endif
#endif /* GNSS_USING_RCVR */
