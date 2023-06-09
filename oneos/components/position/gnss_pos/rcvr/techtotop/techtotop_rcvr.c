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
 * @file        zkw_rcvr.c
 *
 * @brief       zkw gnss recevicer function
 *
 * @revision
 * Date         Author          Notes
 * 2020-12-17   OneOS Team      First Version
 ***********************************************************************************************************************
 */

#include <stdio.h>
#include <string.h>

#include "rcvr_object.h"

#define ONEPOS_LOG_TAG "onepos.techtotop_rcvr"
#define ONEPOS_LOG_LVL ONEPOS_LOG_INFO
#include <onepos_log.h>

#ifdef GNSS_USING_TECHTOTOP

#ifdef RCVR_SUPP_RESET
os_err_t techtotop_rcvr_reset(rcvr_object_t* rcvr, ...)
{
    va_list             var_arg;
    os_int32_t			tmp	 				  = 0;
    rcvr_reset_type_t   start_type            = RCVR_MIN_START_TYPE;
    os_err_t            result                = OS_ERROR;

    OS_ASSERT(OS_NULL != rcvr);
	 
	va_start(var_arg, rcvr);
    tmp = va_arg(var_arg, os_int32_t);
    va_end(var_arg);	
	
	start_type = (rcvr_reset_type_t)tmp;

    switch(start_type)
    {
        case RCVR_COLD_START:
            result = rcvr_object_send(rcvr, "$CCSIR,3,1*4A\r\n", 15);    
        break;

        case RCVR_WARM_START:
            result = rcvr_object_send(rcvr, "$CCSIR,3,2*49\r\n", 15);
        break;

        case RCVR_HOT_START:
            result = rcvr_object_send(rcvr, "$CCSIR,3,3*48\r\n", 15);
        break;

        default:
            ONEPOS_LOG_W("receiver reset error.");
        break;
    }
    return result;
}
#endif

rcvr_object_t *techtotop_rcvr_creat(void)
{
    rcvr_object_t* rcvr = (rcvr_object_t*)os_malloc(sizeof(rcvr_object_t));

    if(OS_NULL == rcvr)
    {
        ONEPOS_LOG_E("creat techtotop receiver is ERROR, no enough memory.");
        return (rcvr_object_t*)OS_NULL;
    }
    
    if(OS_EOK != rcvr_object_init(rcvr, "techtotop", TECHTOTOP_DEV_NAME))
    {
        rcvr_object_deinit(rcvr);
		rcvr_object_destroy(rcvr);
        ONEPOS_LOG_E("init techtotop receiver is ERROR.");
        return (rcvr_object_t*)OS_NULL;
    }

    #ifdef RCVR_SUPP_RESET
    rcvr->ops_table[RCVR_RESET_OPS] = (rcvr_ops_pfunc)techtotop_rcvr_reset;
    #endif

    return rcvr;
}

#endif /* GNSS_USING_TECHTOTOP */


