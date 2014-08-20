#!/usr/bin/env python
# -*- coding: utf-8 -*-

from setuptools import extension, setup, find_packages
import os, platform

from distutils.sysconfig import get_config_vars

(opt,) = get_config_vars('OPT')
os.environ['OPT'] = " ".join(
    flag for flag in opt.split() if flag != '-Wstrict-prototypes'
)

define_macros = [('HAVE_EXPAT_CONFIG_H', '1')]

os_name = platform.system().lower()
if os_name.find('win') >= 0:
    define_macros.append(('XML_STATIC', 1))

cpp_module = extension.Extension(
    'nkit4py',
    define_macros = define_macros,
    include_dirs=[
        './deps/include',
        './deps/expat-2.1.0/lib',
        './deps/nkit/src',
        '/usr/include'
    ],
    library_dirs=[
        '/usr/local/lib',
        '/usr/lib'
    ],
    libraries=[
        'rt'
    ],
    sources=['wrap.cpp',
            './deps/expat-2.1.0/lib/xmlparse.c',
            './deps/expat-2.1.0/lib/xmltok.c',
            './deps/expat-2.1.0/lib/xmlrole.c',
            "./deps/yajl-2.0.5/src/yajl_lex.c",
            "./deps/yajl-2.0.5/src/yajl_encode.c",
            "./deps/yajl-2.0.5/src/yajl_parser.c",
            "./deps/yajl-2.0.5/src/yajl_version.c",
            "./deps/yajl-2.0.5/src/yajl_alloc.c",
            "./deps/yajl-2.0.5/src/yajl_buf.c",
            "./deps/yajl-2.0.5/src/yajl_tree.c",
            "./deps/yajl-2.0.5/src/yajl_gen.c",
            "./deps/yajl-2.0.5/src/yajl.c",
            "./deps/nkit/src/constants.cpp",
            "./deps/nkit/src/tools.cpp",
            "./deps/nkit/src/dynamic/dynamic.cpp",
            "./deps/nkit/src/dynamic/dynamic_json.cpp",
            "./deps/nkit/src/dynamic/dynamic_path.cpp",
            "./deps/nkit/src/dynamic/dynamic_table.cpp",
            "./deps/nkit/src/dynamic/dynamic_table_index_comparators.cpp",
            "./deps/nkit/src/dynamic/dynamic_xml.cpp",
            "./deps/nkit/src/logger/rotate_logger.cpp",
            "./deps/nkit/src/vx/encodings.cpp",
            "./deps/nkit/src/vx/vx.cpp",
            "./deps/nkit/3rd/netbsd/strptime.cpp"
             ]
)

setup(
    name='nkit4py',
    version='0.1.0.dev1',
    description='A sample Python project',
    long_description="long_description",

    # The project's main homepage.
    url='https://github.com/eye3/nkit4py',

    # Author details
    author='Boris T. Darchiev',
    author_email='boris.darchiev@gmail.com',

    # Choose your license
    license='Apache-2.0',

    keywords='xml2py xml2json xml python object list json fast expat sax nkit nkit4py nkit4nodejs',

    py_modules=['datetime_json_encoder'],
    ext_modules=[cpp_module]
)
