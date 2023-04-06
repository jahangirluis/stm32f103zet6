#ifndef __ARCH_EXCEPTION_H__
#define __ARCH_EXCEPTION_H__

#ifdef __cplusplus
    extern "C" {
#endif

#include <oneos_config.h>
#include <os_types.h>
#include <os_task.h>

#ifdef OS_USING_OVERFLOW_CHECK
    
extern os_bool_t task_stack_check(os_task_t *task);

extern void task_switch_stack_check(os_task_t *from_task, os_task_t *to_task);
    
#endif /* OS_USING_OVERFLOW_CHECK */


#ifdef __cplusplus
    }
#endif

#endif /* __ARCH_EXCEPTION_H__ */

