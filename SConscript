# for module compiling
import os
Import('OS_ROOT')
from build_tools import *

sys.path.append(Env['OS_ROOT'] + '/drivers/hal/st/scripts/')
import prebuild
prebuild.prebuild(Env['BSP_ROOT'])

pwd = PresentDir()
objs = []
list = os.listdir(pwd)
if "oneos" in list: list.remove("oneos")

for d in list:
    path = os.path.join(pwd, d)
    if os.path.isfile(os.path.join(path, 'SConscript')):
        objs = objs + SConscript(os.path.join(d, 'SConscript'))

Return('objs')
