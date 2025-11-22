#!/usr/bin/env python
# encoding: utf-8

import os
import time

APPNAME = 'csp-client'
VERSION = 'v1.3'

top    = '.'
out    = 'build'

# Scan modules in local lib dir
modules = ['lib/libcsp', 'lib/libutil', 'lib/libgosh', 'lib/libftp', 'lib/liblog', 'lib/libparam', 'lib/libhk', 'lib/libfp', 'clients']

def options(ctx):
    ctx.load('gcc')
    if os.path.exists('eclipse.py'):
        ctx.load('eclipse')
    ctx.recurse(modules)
    ctx.add_option('--full-source', action='store_true', help='Include full source in dist')

def configure(ctx):
    ctx.load('gcc')
    
    try:
        ctx.find_program("sphinx-build", var="SPHINX_BUILD")
    except ctx.errors.ConfigurationError:
        pass
    
    ctx.env.append_unique('FILES_CSPCLIENT', 'src/*.c')
    ctx.env.append_unique('LIBS_CSPCLIENT', ['rt', 'pthread'])

    # Options for CSP
    ctx.options.with_os = 'posix'
    ctx.options.enable_rdp = True
    ctx.options.enable_qos = True
    ctx.options.enable_crc32 = True
    ctx.options.enable_hmac = True
    ctx.options.enable_xtea = True
    ctx.options.enable_promisc = True
    ctx.options.enable_if_kiss = True
    ctx.options.enable_if_can = True
    ctx.options.enable_if_zmqhub = True
    ctx.options.disable_stlib = True
    ctx.options.with_rtable = 'cidr'
    ctx.options.enable_can_socketcan = True
    ctx.options.with_driver_usart = 'linux'
    ctx.options.with_router_queue_length = 100
    ctx.options.with_conn_queue_length = 100
    
    # Options for clients
    ctx.options.enable_nanopower2_client = True
    ctx.options.enable_bpx_client = True
    if ctx.options.with_clients is None:
        ctx.options.with_clients = 'libgssb, nanocam2, p60-dock, libp60, gatoss-uc, a712, nanopower, nanopower-bpx, p60-pdu, nanocom, nanocom-ax, p60-acu, liba3200, nanohub, z7000-monitor'

    # Options for libftp
    ctx.options.enable_ftp_client = True
    
    # Options for liblog
    ctx.options.enable_log_node = True

    # Options for libutil
    ctx.options.clock = 'linux'
    ctx.options.enable_driver_debug = True
    ctx.options.with_log = 'cdh'
    ctx.options.enable_vmem = True
    
    # Options for libparam
    ctx.options.enable_param_client = True
    ctx.options.param_lock = 'none'

    # Options for libhk
    #ctx.options.enable_hk_collector = True

    # Options for libfp
    ctx.options.enable_fp_client = True

    # Options for clients
    if ctx.options.with_clients == 'all':
        ctx.options.with_clients = None
    elif ctx.options.with_clients == None:
        ctx.options.with_clients = ' '

    ctx.define('CSPCLIENT_VERSION', VERSION)
    param_path = "build/lib/libparam/include/param"
    if not os.path.exists(param_path):
        os.makedirs(param_path)
    with open(os.path.join(param_path, "param.h"), "w") as param_hdr_file:
        param_hdr_file.write("#include <param/rparam_client.h>\n")
        param_hdr_file.write("#include <param/param_string.h>\n")
        param_hdr_file.write("#include <param/param_serializer.h>\n")
        param_hdr_file.write("#include <param/param_fletcher.h>\n")

    # Recurse and write config
    ctx.write_config_header('include/conf_cspclient.h', top=True, remove=True)
    ctx.recurse(modules, mandatory=False)    

def build(ctx):
    ctx(export_includes=['include', 'client', 'lib/libparam/include', 'lib/libparam/src/include'], name='include')
    ctx.recurse(modules, mandatory=False)
    use = ['csp', 'util', 'gosh', 'ftp', 'log', 'libparam-client', 'hk', 'fp'] + ctx.env.LIBCLIENTS_USE
    ctx.program(
        source=ctx.path.ant_glob(ctx.env.FILES_CSPCLIENT), 
        target='csp-client', 
        linkflags=ctx.env.LINKFLAGS_CSPCLIENT,
        use=use,
        lib=ctx.env.LIBS_CSPCLIENT + ctx.env.LIBS + ['m']
        )

def doc(ctx):
    os.system('rm -rf doc/html')
    os.system('rm -rf doc/*.pdf')
    VERSION = os.popen('git describe --always --dirty=-dirty 2> /dev/null || echo unknown').read().strip()
    ctx(
        rule   = "${SPHINX_BUILD} -q -c ./doc -b html -D release="+VERSION+" -d build/doc/doctrees . build/doc/html -t cspclient",
        cwd    = ctx.path.abspath(),
        source = ctx.path.ant_glob('**/*.rst'),
        target = './doc/html/doc/index.html',
        )
    ctx(
        rule   = "${SPHINX_BUILD} -q -c ./doc -b latex -D release="+VERSION+" -d build/doc/doctrees . build/doc/latex -t cspclient",
        cwd    = ctx.path.abspath(),
        source = ctx.path.ant_glob('**/*.rst'),
        target = './doc/latex/csp-client.tex',
        )
    ctx(
        cwd    = ctx.path.abspath(),
        rule   = 'make -C build/doc/latex all-pdf 2>&1 > /dev/null || echo make error',
        source = './doc/latex/csp-client.tex',
        target = './doc/latex/csp-client.pdf',
        )

from waflib.Build import BuildContext
class Doc(BuildContext):
   cmd = 'doc'
   fun = 'doc'
   
def dist(ctx):
    if not ctx.path.find_node('build/doc/latex/csp-client.pdf'):
        ctx.fatal('You forgot to run \'waf doc\' first, we need to include the documentation in the output')

    # Remove/clean all libs
    os.system('rm -f lib/*.gz')

    # Get git version
    VERSION = os.popen('git describe --always --dirty=-dirty 2> /dev/null || echo unknown').read().strip()

    os.system('cp build/doc/latex/csp-client.pdf doc/gs-man-csp-client-'+VERSION+'.pdf')
    os.system('cp -r build/doc/html doc/html')

    ctx.base_name = 'csp-client-' + VERSION
    if ctx.options.full_source:
        ctx.base_name += '-source'
        bin_modules.clear()
    ctx.excl = ['**/.waf-1*', '**/*~', '**/*.pyc', '**/*.swp', '**/.lock-w*', '**/build', '**/.settings', '**/.git*']
    ctx.files = ctx.path.ant_glob(['waf', 'wscript', 'eclipse.py', 'clients/**', 'src/**', 'doc/**', 'CHANGELOG'])
    ctx.files.extend(ctx.path.ant_glob('lib/libcsp/**'))
    ctx.files.extend(ctx.path.ant_glob('lib/libutil/**'))
    ctx.files.extend(ctx.path.ant_glob('lib/libparam/**'))
    ctx.files.extend(ctx.path.ant_glob('lib/liblog/wscript'))
    ctx.files.extend(ctx.path.ant_glob('lib/liblog/**/*.h'))
    ctx.files.extend(ctx.path.ant_glob('lib/liblog/src/client/**'))
    ctx.files.extend(ctx.path.ant_glob('lib/liblog/src/host/**'))
    ctx.files.extend(ctx.path.ant_glob('lib/liblog/src/pack/**'))

    ctx.files.extend(ctx.path.ant_glob('lib/libgosh/wscript'))
    ctx.files.extend(ctx.path.ant_glob('lib/libgosh/**/*.h'))
    ctx.files.extend(ctx.path.ant_glob('lib/libgosh/src/extras/**'))
    ctx.files.extend(ctx.path.ant_glob('lib/libgosh/src/gosh/*.c'))

    ctx.files.extend(ctx.path.ant_glob('lib/libftp/wscript'))
    ctx.files.extend(ctx.path.ant_glob('lib/libftp/**/*.h'))
    ctx.files.extend(ctx.path.ant_glob('lib/libftp/src/ftp/ftp_client.c'))
    ctx.files.extend(ctx.path.ant_glob('lib/libftp/src/ftp/cmd_ftp.c'))

    ctx.files.extend(ctx.path.ant_glob('lib/libhk/wscript'))
    ctx.files.extend(ctx.path.ant_glob('lib/libhk/**/*.h'))
    ctx.files.extend(ctx.path.ant_glob('lib/libhk/client/libhk_cmd.c'))

    ctx.files.extend(ctx.path.ant_glob('lib/libfp/wscript'))
    ctx.files.extend(ctx.path.ant_glob('lib/libfp/**/*.h'))
    ctx.files.extend(ctx.path.ant_glob('lib/libfp/src/client/fp_client.c'))
    ctx.files.extend(ctx.path.ant_glob('lib/libfp/src/client/fp_cmd.c'))
