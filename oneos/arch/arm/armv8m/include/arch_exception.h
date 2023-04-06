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
 * @file        arch_exception.h
 *
 * @brief       This file provides functions related to the cortex-v8M architecture.
 *
 * @revision
 * Date         Author          Notes
 * 2020-02-23   OneOS Team      First version.
 ***********************************************************************************************************************
 */
#ifndef __OS_ARCH_EXCEPTION_H__
#define __OS_ARCH_EXCEPTION_H__

#include <os_types.h>
#include <oneos_config.h>
#include <os_task.h>

#ifdef __cplusplus
    extern "C" {
#endif

#ifdef STACK_TRACE_EN

extern os_err_t  task_stack_show(char *name, os_uint32_t context);

#endif  /* STACK_TRACE_EN */

#ifdef OS_USING_OVERFLOW_CHECK

extern os_bool_t task_stack_check(os_task_t *task);

extern void task_switch_stack_check(os_task_t *from_task, os_task_t *to_task);

#endif /* OS_USING_OVERFLOW_CHECK */


#ifdef __cplusplus
    }
#endif

#endif /* __OS_ARCH_EXCEPTION_H__ */

