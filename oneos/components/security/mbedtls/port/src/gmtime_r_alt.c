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
 * @file        gmtime_rl_alt.c
 *
 * @brief       os related alt time functions
 *
 * @revision
 * Date         Author          Notes
 * 2020-11-25   OneOS Team      First Version
 ***********************************************************************************************************************
 */

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#ifdef MBEDTLS_PLATFORM_GMTIME_R_ALT

#include <sys/time.h>
#include "mbedtls/platform_time.h"

struct tm *mbedtls_platform_gmtime_r( const mbedtls_time_t *tt,
                                      struct tm *tm_buf )
{
    return gmtime_r(tt, tm_buf);
}

#endif
