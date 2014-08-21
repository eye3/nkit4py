#!/usr/bin/env python
# -*- coding: utf-8 -*-

from setuptools import extension, setup, find_packages
import os, platform, sys

if sys.version_info[0] < 3:
    from codecs import open

path = os.path.dirname(os.path.realpath(__file__))

with open(path + "/README", "U", encoding="utf-8") as r:
    readme_text = r.read()
    
## OS X non-PPC workaround
# Apple OS X 10.6 with Xcode 4 have Python compiled with PPC but they removed
# support for compiling with that arch, so we have to override ARCHFLAGS.
if sys.platform == "darwin" and not os.environ.get("ARCHFLAGS"):
    compiler_dirn = "/usr/libexec/gcc/darwin"
    if os.path.exists(compiler_dirn):
        dir_items = os.listdir(compiler_dirn)
        if "ppc" not in dir_items:
            os.environ["ARCHFLAGS"] = "-arch i386 -arch x86_64"
            
from distutils.sysconfig import get_config_vars

(opt,) = get_config_vars('OPT')
if opt:
    os.environ['OPT'] = " ".join(
        flag for flag in opt.split() if flag != '-Wstrict-prototypes'
    )
    
cflags = ["-fno-strict-aliasing", ]

define_macros = [('HAVE_EXPAT_CONFIG_H', '1')]
libraries = []
os_name = platform.system().lower()
if os_name.find('win') >= 0:
    define_macros.append(('XML_STATIC', 1))
elif os_name.find('linux') >= 0:
    libraries.append('rt')
    
cpp_module = extension.Extension(
    'nkit4py',
    define_macros = define_macros,
    include_dirs=[
        './deps/include',
        './deps/expat-2.1.0/lib',
        './deps/nkit/src',
        './src'
    ],
    library_dirs=[],
    libraries=libraries,
    extra_compile_args=cflags,
    sources=['./src/wrap.cpp',
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
    version='0.1.0.dev11',
    description='Simple and fast XML to Python object or JSON converter and filter. Written in C++ using Expat SAX parser.',
    long_description=readme_text,
    url='https://github.com/eye3/nkit4py',
    author='Boris T. Darchiev',
    author_email='boris.darchiev@gmail.com',
    license='Apache-2.0 <http://www.apache.org/licenses/LICENSE-2.0>',

    keywords='xml2py xml2json xml python object list json fast expat sax nkit nkit4py nkit4nodejs',

    py_modules=[],
    ext_modules=[cpp_module]
)
