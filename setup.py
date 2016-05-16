#!/usr/bin/env python
# -*- coding: utf-8 -*-

from setuptools import extension, setup, find_packages
import os, platform, sys

if sys.version_info[0] < 3:
    from codecs import open

path = os.path.dirname(os.path.realpath(__file__))

def read_readme():
    f = open(os.path.join(os.path.dirname(__file__), 'README'))
    try:
        return f.read()
    finally:
        f.close()
    
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
    
cflags = ["-fno-strict-aliasing"]
define_macros = [('HAVE_EXPAT_CONFIG_H', '1')]
libraries = []
os_name = platform.system().lower()
if os_name.startswith('win'):
    define_macros.append(('XML_STATIC', 1))
    define_macros.append(('_CRT_SECURE_NO_WARNINGS', 1))
    libraries.append('Advapi32')
    cflags = ["/EHsc", "/MD"]
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
    sources=[
        './src/module.cpp',
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
        "./deps/nkit/src/encoding/transcode.cpp",
        "./deps/nkit/src/xml/xml2var.cpp",
        "./deps/nkit/3rd/netbsd/strptime.cpp"
    ]
)

setup(
    name='nkit4py',
    version='2.4.1',
    description='Fast XML to Python (and vise versa) converter and filter. Written in C++ using Expat SAX parser',
    long_description=read_readme(),
    url='https://github.com/eye3/nkit4py',
    author='Boris T. Darchiev',
    author_email='boris.darchiev@gmail.com',
    license='Apache-2.0 <http://www.apache.org/licenses/LICENSE-2.0>',

    keywords='xml python object list json convertor filter fast expat sax xpath nkit nkit4py xml2py xml2json',

    py_modules=[],
    ext_modules=[cpp_module],
    classifiers=[
          'Development Status :: 5 - Production/Stable',
          'Environment :: Console',
          'Environment :: MacOS X',
          'Environment :: No Input/Output (Daemon)',
          'Environment :: Win32 (MS Windows)',
          'Intended Audience :: Developers',
          'Intended Audience :: System Administrators',
          'Intended Audience :: Other Audience',
          'Intended Audience :: Education',
          'License :: OSI Approved :: Apache Software License',
          'Operating System :: MacOS',
          'Operating System :: MacOS :: MacOS X',
          'Operating System :: Microsoft',
          'Operating System :: Microsoft :: Windows',
          'Operating System :: POSIX',
          'Operating System :: POSIX :: Linux',
          'Programming Language :: C++',
          'Programming Language :: Python :: 2.6',
          'Programming Language :: Python :: 2.7',
          'Programming Language :: Python :: 3.3',
          'Programming Language :: Python :: 3.4',
          'Programming Language :: Python :: 3.5',
          'Topic :: Text Processing',
          'Topic :: Text Processing :: Filters',
          'Topic :: Text Processing :: Markup :: XML',
          'Topic :: Software Development :: Libraries :: Python Modules'
          ]
)
