import os
import json


# toolchains options
ARCH       = 'arm'
CPU        = 'cortex-m3'
CROSS_TOOL = 'gcc'

# bsp lib config
BSP_LIBRARY_TYPE = None

if os.getenv('OS_CC'):
    CROSS_TOOL = os.getenv('OS_CC')
if os.getenv('OS_ROOT'):
    OS_ROOT = os.getenv('OS_ROOT')

# cross_tool provides the cross compiler
# COMPILER_PATH is the compiler execute path, for example, CodeSourcery, Keil MDK, IAR
if  CROSS_TOOL == 'gcc':
    COMPILER    = 'gcc'
    COMPILER_PATH   = r'C:\Users\XXYYZZ'
elif CROSS_TOOL == 'keil':
    COMPILER    = 'armcc'
    # Notice: The installation path of armcc cannot have Chinese
    COMPILER_PATH   = r'C:/Keil_v5'
elif CROSS_TOOL == 'iar':
    COMPILER    = 'iar'
    COMPILER_PATH   = r'C:/Program Files (x86)/IAR Systems/Embedded Workbench 8.0'

if CROSS_TOOL == 'gcc' and os.getenv('OS_EXEC_PATH'):
    COMPILER_PATH = os.getenv('OS_EXEC_PATH')

BUILD = 'debug'
#BUILD = 'release'

if COMPILER == 'gcc':
    # toolchains
    PREFIX = 'arm-none-eabi-'
    CC = PREFIX + 'gcc'
    AS = PREFIX + 'gcc'
    AR = PREFIX + 'ar'
    CXX = PREFIX + 'g++'
    LINK = PREFIX + 'gcc'
    RESULT_SUFFIX = 'elf'
    SIZE = PREFIX + 'size'
    OBJDUMP = PREFIX + 'objdump'
    OBJCPY = PREFIX + 'objcopy'

    DEVICE = ' -mcpu=cortex-m3 -mthumb -ffunction-sections -fdata-sections'
    CFLAGS = DEVICE + ' -Dgcc'
    AFLAGS = ' -c' + DEVICE + ' -x assembler-with-cpp -Wa,-mimplicit-it=thumb '
    LFLAGS = DEVICE + ' -Wl,--gc-sections,-Map=oneos.map,-cref,-u,Reset_Handler -T board/linker_scripts/link.lds'

    CPATH = ''
    LPATH = ''

    if BUILD == 'debug':
        CFLAGS += ' -O0 -gdwarf-2 -g'
        AFLAGS += ' -gdwarf-2'
    else:
        CFLAGS += ' -O2'

    CXXFLAGS = CFLAGS 

    POST_ACTION = OBJCPY + ' -O binary $TARGET oneos.bin\n' + SIZE + ' $TARGET \n'

elif COMPILER == 'armcc':
    # toolchains
    CC = 'armcc'
    CXX = 'armcc'
    AS = 'armasm'
    AR = 'armar'
    LINK = 'armlink'
    RESULT_SUFFIX = 'axf'

    DEVICE = ' --cpu Cortex-M3 '
    CFLAGS = '-c ' + DEVICE + ' --apcs=interwork --split_sections --c99'
    AFLAGS = DEVICE + ' --apcs=interwork '
    LFLAGS = DEVICE + ' --scatter "board\linker_scripts\link.sct" --info sizes --info totals --info unused --info veneers --list oneos.map --strict'
    CFLAGS += ' -I "' + COMPILER_PATH + '/ARM/ARMCC/include"'
    LFLAGS += ' --libpath="' + COMPILER_PATH + '/ARM/ARMCC/lib"'

    #CFLAGS += ' -D__MICROLIB '
    #AFLAGS += ' --pd "__MICROLIB SETA 1" '
    #LFLAGS += ' --library_type=microlib '
    COMPILER_PATH += '/ARM/ARMCC/bin/'

    if BUILD == 'debug':
        CFLAGS += ' -g -O0'
        AFLAGS += ' -g'
    else:
        CFLAGS += ' -O2'

    CXXFLAGS = CFLAGS

    POST_ACTION = 'fromelf --bin $TARGET --output oneos.bin \nfromelf -z $TARGET'

elif COMPILER == 'iar':
    # toolchains
    CC = 'iccarm'
    CXX = 'iccarm'
    AS = 'iasmarm'
    AR = 'iarchive'
    LINK = 'ilinkarm'
    RESULT_SUFFIX = 'out'

    DEVICE = '-Dewarm'

    CFLAGS = DEVICE
    CFLAGS += ' --diag_suppress Pa050'
    CFLAGS += ' --no_cse'
    CFLAGS += ' --no_unroll'
    CFLAGS += ' --no_inline'
    CFLAGS += ' --no_code_motion'
    CFLAGS += ' --no_tbaa'
    CFLAGS += ' --no_clustering'
    CFLAGS += ' --no_scheduling'
    CFLAGS += ' --endian=little'
    CFLAGS += ' --cpu=Cortex-M4'
    CFLAGS += ' -e'
    CFLAGS += ' --fpu=VFPv4_sp'
    CFLAGS += ' --dlib_config "' + COMPILER_PATH + '/arm/INC/c/DLib_Config_Normal.h"'
    CFLAGS += ' --silent'

    AFLAGS = DEVICE
    AFLAGS += ' -s+'
    AFLAGS += ' -w+'
    AFLAGS += ' -r'
    AFLAGS += ' --cpu Cortex-M4'
    AFLAGS += ' --fpu VFPv4_sp'
    AFLAGS += ' -S'

    if BUILD == 'debug':
        CFLAGS += ' --debug'
        CFLAGS += ' -On'
    else:
        CFLAGS += ' -Oh'

    LFLAGS = ' --config "board/linker_scripts/link.icf"'
    LFLAGS += ' --entry __iar_program_start'

    CXXFLAGS = CFLAGS
    
    COMPILER_PATH = COMPILER_PATH + '/arm/bin/'
    POST_ACTION = 'ielftool --bin $TARGET oneos.bin'


def get_params_from_json():
    """
    read json and update local variables
    Returns: None

    """
    file_name = 'osconfig.json'
    if not os.path.exists(file_name):
        return
    with open(file_name, 'r') as fp:
        json_values = json.load(fp)

    assert len(json_values) == 1, 'osconfig.json must have only one key-value pair, please check it'
    cross_tool = list(json_values.keys())[0]
    globals().update(json_values[cross_tool][cross_tool])
    globals().update({'CROSS_TOOL': cross_tool})

    if cross_tool == 'gcc':
        globals().update({'COMPILER': 'gcc', 'PREFIX': PREFIX})
    elif cross_tool == 'keil':
        # Notice: The installation path of armcc cannot have Chinese
        globals().update({'COMPILER': 'armcc'})
    elif cross_tool == 'iar':
        globals().update({'COMPILER': 'iar'})


get_params_from_json()
