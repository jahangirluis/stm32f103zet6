#
# Copyright (c) 2012-2019, CMCC IOT
#
# SPDX-License-Identifier: Apache-2.0
#
#

import os
import sys
import glob

from utils import *
from utils import _make_path_relative
from utils import xml_indent

import xml.etree.ElementTree as etree
from xml.etree.ElementTree import SubElement

source_pattern = ['*.c', '*.cpp', '*.cxx', '*.s', '*.S', '*.asm']

def OSPath(path):
    import platform

    if type(path) == type('str'):
        if platform.system() == 'Windows':
            return path.replace('/', '\\')
        else:
            return path.replace('\\', '/')
    else:
        if platform.system() == 'Windows':
            return [item.replace('/', '\\') for item in path]
        else:
            return [item.replace('\\', '/') for item in path]


# collect the build source code path and parent path
def CollectPaths(paths):
    all_paths = []

    def ParentPaths(path):
        ret = os.path.dirname(path)
        if ret == path or ret == '':
            return []

        return [ret] + ParentPaths(ret)

    for path in paths:
        # path = os.path.abspath(path)
        path = path.replace('\\', '/')
        all_paths = all_paths + [path] + ParentPaths(path)

    all_paths = list(set(all_paths))
    return sorted(all_paths)

'''
Collect all of files under paths
'''
def CollectFiles(paths, pattern):
    files = []
    for path in paths:
        if type(pattern) == type(''):
            files = files + glob.glob(path + '/' + pattern)
        else:
            for item in pattern:
                # print('--> %s' % (path + '/' + item))
                files = files + glob.glob(path + '/' + item)

    return sorted(files)

def CollectAllFilesinPath(path, pattern):
    files = []

    for item in pattern:
        files += glob.glob(path + '/' + item)

    list = os.listdir(path)
    if len(list):
        for item in list:
            if item.startswith('.'):
                continue
            if item == 'bsp':
                continue

            if os.path.isdir(os.path.join(path, item)):
                files = files + CollectAllFilesinPath(os.path.join(path, item), pattern)
    return files

'''
Exclude files from infiles
'''
def ExcludeFiles(infiles, files):
    in_files  = set([OSPath(file) for file in infiles])
    exl_files = set([OSPath(file) for file in files])

    exl_files = in_files - exl_files

    return exl_files


# caluclate the exclude path for project
def ExcludePaths(rootpath, paths):
    ret = []

    files = os.listdir(rootpath)
    for file in files:
        if file.startswith('.'):
            continue

        fullname = os.path.join(rootpath, file)

        if os.path.isdir(fullname):
            # print(fullname)
            if not fullname in paths:
                ret = ret + [fullname]
            else:
                ret = ret + ExcludePaths(fullname, paths)

    return ret


cm_path_prefix = '"${workspace_loc://${ProjName}//'


def ConverToRttEclipsePathFormat(path):
    return cm_path_prefix + path + '}"'


def IsRttEclipsePathFormat(path):
    if path.startswith(cm_path_prefix):
        return True
    else :
        return False


def HandleToolOption(tools, env, project, reset):
    BSP_ROOT = os.path.abspath(env['BSP_ROOT'])

    CPPDEFINES = project['CPPDEFINES']
    paths = [ConverToRttEclipsePathFormat(RelativeProjectPath(env, os.path.normpath(i)).replace('\\', '/')) for i in project['CPPPATH']]

    compile_include_paths_option = None
    compile_include_files_option = None
    compile_defs_option = None
    linker_scriptfile_option = None
    linker_script_option = None
    linker_nostart_option = None
    linker_libs_option = None
    linker_paths_option = None
    linker_newlib_nano_option = None

    for tool in tools:

        if tool.get('id').find('c.compile') != 1:
            options = tool.findall('option')
            # find all compile options
            for option in options:
                if option.get('id').find('c.compiler.include.paths') != -1 or option.get('id').find('c.compiler.option.includepaths') != -1:
                    compile_include_paths_option = option
                elif option.get('id').find('c.compiler.include.files') != -1 or option.get('id').find('c.compiler.option.includefiles') != -1 :
                    compile_include_files_option = option
                elif option.get('id').find('c.compiler.defs') != -1 or option.get('id').find('c.compiler.option.definedsymbols') != -1:
                    compile_defs_option = option

        if tool.get('id').find('c.linker') != -1:
            options = tool.findall('option')
            # find all linker options
            for option in options:
                if option.get('id').find('c.linker.scriptfile') != -1:
                    linker_scriptfile_option = option
                elif option.get('id').find('c.linker.option.script') != -1:
                    linker_script_option = option
                elif option.get('id').find('c.linker.nostart') != -1:
                    linker_nostart_option = option
                elif option.get('id').find('c.linker.libs') != -1 and env.has_key('LIBS'):
                    linker_libs_option = option
                elif option.get('id').find('c.linker.paths') != -1 and env.has_key('LIBPATH'):
                    linker_paths_option = option
                elif option.get('id').find('c.linker.usenewlibnano') != -1:
                    linker_newlib_nano_option = option

    # change the inclue path
    if compile_include_paths_option is not None :
        option = compile_include_paths_option
        # find all of paths in this project
        include_paths = option.findall('listOptionValue')
        for item in include_paths:
            if reset is True or IsRttEclipsePathFormat(item.get('value')) :
                # clean old configuration
                option.remove(item)
        # print('c.compiler.include.paths')
        paths = sorted(paths)
        for item in paths:
            SubElement(option, 'listOptionValue', {'builtIn': 'false', 'value': item})
    # change the inclue files (default) or definitions
    if compile_include_files_option is not None:
        option = compile_include_files_option
        # add '_REENT_SMALL' to CPPDEFINES when --specs=nano.specs has select
        if linker_newlib_nano_option is not None and linker_newlib_nano_option.get('value') == 'true' and '_REENT_SMALL' not in CPPDEFINES:
            CPPDEFINES += ['_REENT_SMALL']

        file_header = '''
#ifndef CMCONFIG_PREINC_H__
#define CMCONFIG_PREINC_H__

/* Automatically generated file; DO NOT EDIT. */
/* OneOS pre-include file */

'''
        file_tail = '\n#endif /*CMCONFIG_PREINC_H__*/\n'
        cm_pre_inc_item = '"${workspace_loc:/${ProjName}/cmconfig_preinc.h}"'
        # save the CPPDEFINES in to cmconfig_preinc.h
        with open('cmconfig_preinc.h', mode = 'w+') as f:
            f.write(file_header)
            for cppdef in CPPDEFINES:
                f.write("#define " + cppdef + '\n')
            f.write(file_tail)
        #  change the c.compiler.include.files
        files = option.findall('listOptionValue')
        find_ok = False
        for item in files:
            if item.get('value') == cm_pre_inc_item:
                find_ok = True
                break
        if find_ok is False:
            SubElement(option, 'listOptionValue', {'builtIn': 'false', 'value': cm_pre_inc_item})
    elif compile_defs_option is not None :
        option = compile_defs_option
        defs = option.findall('listOptionValue')
        project_defs = []
        for item in defs:
            if reset is True:
                # clean all old configuration
                option.remove(item)
            else:
                project_defs += [item.get('value')]
        if len(project_defs) > 0:
            cproject_defs = set(CPPDEFINES) - set(project_defs)
        else:
            cproject_defs = CPPDEFINES

        # print('c.compiler.defs')
        cproject_defs = sorted(cproject_defs)
        for item in cproject_defs:
            SubElement(option, 'listOptionValue', {'builtIn': 'false', 'value': item})

    # update linker script config
    if linker_scriptfile_option is not None :
        option = linker_scriptfile_option
        linker_script = 'link.lds'
        items = env['LINKFLAGS'].split(' ')
        if '-T' in items:
            linker_script = items[items.index('-T') + 1]
            linker_script = ConverToRttEclipsePathFormat(linker_script)

        listOptionValue = option.find('listOptionValue')
        if listOptionValue != None:
            listOptionValue.set('value', linker_script)
        else:
            SubElement(option, 'listOptionValue', {'builtIn': 'false', 'value': linker_script})
    # scriptfile in stm32cubeIDE
    if linker_script_option is not None :
        option = linker_script_option
        items = env['LINKFLAGS'].split(' ')
        if '-T' in items:
            linker_script = ConverToRttEclipsePathFormat(items[items.index('-T') + 1]).strip('"')
            option.set('value', linker_script)
    # update nostartfiles config
    if linker_nostart_option is not None :
        option = linker_nostart_option
        if env['LINKFLAGS'].find('-nostartfiles') != -1:
            option.set('value', 'true')
        else:
            option.set('value', 'false')
    # update libs
    if linker_libs_option is not None :
        option = linker_libs_option
        # remove old libs
        for item in option.findall('listOptionValue'):
            option.remove(item)
        # add new libs
        for lib in env['LIBS']:
            SubElement(option, 'listOptionValue', {'builtIn': 'false', 'value': lib})
    # update lib paths
    if linker_paths_option is not None :
        option = linker_paths_option
        # remove old lib paths
        for item in option.findall('listOptionValue'):
            option.remove(item)
        # add new old lib paths
        paths = [ConverToRttEclipsePathFormat(RelativeProjectPath(env, os.path.normpath(i)).replace('\\', '/')) for i in env['LIBPATH']]
        for path in paths:
            SubElement(option, 'listOptionValue', {'builtIn': 'false', 'value': path})
        #for path in env['LIBPATH']:
        #    SubElement(option, 'listOptionValue', {'builtIn': 'false', 'value': path})

    return


def UpdateProjectStructure(env, prj_name):
    bsp_root = env['BSP_ROOT']
    os_root = env['OS_ROOT']

    project = etree.parse('.project')
    root = project.getroot()

    if os_root.startswith(bsp_root):
        linkedResources = root.find('linkedResources')
        if linkedResources == None:
            linkedResources = SubElement(root, 'linkedResources')

        links = linkedResources.findall('link')
        # delete all OneOS folder links
        for link in links:
            if link.find('name').text.startswith('oneos'):
                linkedResources.remove(link)

    if prj_name:
        name = root.find('name')
        if name == None:
            name = SubElement(root, 'name')
        name.text = prj_name
    print("prjname=", prj_name)
    out = open('.project', 'wb')
    #out.write('<?xml version="1.0" encoding="UTF-8"?>\n'.encode('UTF-8'))
    xml_indent(root)
    out.write(etree.tostring(root, encoding="UTF-8"))
    print("bobtest",etree.tostring(root))
    out.close()

    return


def GenExcluding(env, project):
    os_root = os.path.abspath(env['OS_ROOT'])
    bsp_root = os.path.abspath(env['BSP_ROOT'])
    coll_dirs = CollectPaths(project['DIRS'])
    all_paths = [OSPath(path) for path in coll_dirs]

    # remove unused path
    for path in all_paths:
        if not path.startswith(os_root) and not path.startswith(bsp_root):
            all_paths.remove(path)

    if bsp_root.startswith(os_root):
        # bsp folder is in the OneOS root folder, such as the OneOS source code on GitHub
        exclude_paths = ExcludePaths(os_root, all_paths)
    elif os_root.startswith(bsp_root):
        # OneOS root folder is in the bsp folder, such as project folder which generate by 'scons --dist' cmd
        check_path = []
        exclude_paths = []
        # analyze the primary folder which relative to BSP_ROOT and in all_paths
        for path in all_paths :
            if path.startswith(bsp_root) :
                folders = RelativeProjectPath(env, path).split('\\')
                if folders[0] != '.' and '\\' + folders[0] not in check_path:
                    import platform
                    if platform.system() == "Windows":
                        check_path += ['\\' + folders[0]]
                    else:
                        check_path += ['/' + folders[0]]
        # exclue the folder which has managed by scons
        for path in check_path:
            exclude_paths += ExcludePaths(bsp_root + path, all_paths)
    else:
        exclude_paths = ExcludePaths(os_root, all_paths)
        exclude_paths += ExcludePaths(bsp_root, all_paths)

    paths = exclude_paths
    exclude_paths = []
    # remove the folder which not has source code by source_pattern
    for path in paths:
        # add bsp and libcpu folder and not collect source files (too more files)
        if path.endswith('oneos\\bsp') or path.endswith('oneos\\libcpu'):
            exclude_paths += [path]
            continue

        set = CollectAllFilesinPath(path, source_pattern)
        if len(set):
            exclude_paths += [path]

    exclude_paths = [RelativeProjectPath(env, path).replace('\\', '/') for path in exclude_paths]

    all_files = CollectFiles(all_paths, source_pattern)
    src_files = project['FILES']

    exclude_files = ExcludeFiles(all_files, src_files)
    exclude_files = [RelativeProjectPath(env, file).replace('\\', '/') for file in exclude_files]

    env['ExPaths'] = exclude_paths
    env['ExFiles'] = exclude_files

    return  exclude_paths + exclude_files


def RelativeProjectPath(env, path):
    project_root = os.path.abspath(env['BSP_ROOT'])
    os_root = os.path.abspath(env['OS_ROOT'])
    
    if path.startswith(project_root):
        return _make_path_relative(project_root, path)
    
    if path.startswith(os_root):
        return 'oneos/' + _make_path_relative(os_root, path)

    # TODO add others folder
    print('ERROR: the ' + path + ' not support')

    return path


def HandleExcludingOption(entry, sourceEntries, excluding):
    old_excluding = []
    if entry != None:
        old_excluding = entry.get('excluding').split('|')
        sourceEntries.remove(entry)

    value = ''
    for item in old_excluding:
        if item.startswith('//') :
            old_excluding.remove(item)
        else :
            if value == '':
                value = item
            else:
                value += '|' + item

    for item in excluding:
        # add special excluding path prefix for OneOS
        item = '//' + item
        if value == '':
            value = item
        else:
            value += '|' + item

    SubElement(sourceEntries, 'entry', {'excluding': value, 'flags': 'VALUE_WORKSPACE_PATH|RESOLVED', 'kind':'sourcePath', 'name':""})


def UpdateCproject(env, project, excluding, reset, prj_name):
    excluding = sorted(excluding)

    cproject = etree.parse('.cproject')

    root = cproject.getroot()
    cconfigurations = root.findall('storageModule/cconfiguration')
    for cconfiguration in cconfigurations:
        tools = cconfiguration.findall('storageModule/configuration/folderInfo/toolChain/tool')
        HandleToolOption(tools, env, project, reset)

        sourceEntries = cconfiguration.find('storageModule/configuration/sourceEntries')
        entry = sourceEntries.find('entry')
        HandleExcludingOption(entry, sourceEntries, excluding)
    # update refreshScope
    if prj_name:
        prj_name = '/' + prj_name
        configurations = root.findall('storageModule/configuration')
        for configuration in configurations:
            resource = configuration.find('resource')
            configuration.remove(resource)
            SubElement(configuration, 'resource', {'resourceType': "PROJECT", 'workspacePath': prj_name})

    # write back to .cproject
    out = open('.cproject', 'wb')
    out.write('<?xml version="1.0" encoding="UTF-8" standalone="no"?>\n'.encode('UTF-8'))
    out.write('<?fileVersion 4.0.0?>'.encode('utf-8'))
    xml_indent(root)
    out.write(etree.tostring(root, encoding='utf-8'))
    out.close()


def TargetEclipse(env, reset = False, prj_name = None):
    global source_pattern

    print('Update eclipse setting..., prj_name=',prj_name)

    if not os.path.exists('.cproject'):
        print('no eclipse CDT project found!')
        return

    project = ProjectInfo(env)

    # update the project file structure info on '.project' file
    #UpdateProjectStructure(env, prj_name)

    # generate the exclude paths and files
    excluding = GenExcluding(env, project)

    # update the project configuration on '.cproject' file
    UpdateCproject(env, project, excluding, reset, prj_name)

    print('done!')

    return
