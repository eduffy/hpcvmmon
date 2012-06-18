#vim: syn=python

APPNAME = 'vmmon'
VERSION = '0.1'

def options(ctx):
  ctx.load('compiler_c')
  ctx.load('compiler_cxx')
  ctx.load('qt4')

def configure(conf):
  conf.env.INCLUDES = [ '.' ]
  conf.env.DEFINES  = [ 'QTONLY' ]
  conf.env.CFLAGS   = [ '-Wall', '-O0', '-ggbd' ]
  conf.env.CXXFLAGS = [ '-Wall', '-O0', '-ggbd' ]
  conf.load('compiler_c')
  conf.load('compiler_cxx')
  conf.load('qt4')

  conf.check_cfg(package      = 'libssh2',
                 args         = '--cflags --libs',              
                 uselib_store = 'LIBSSH2')

  conf.check_cfg(package      = 'libvncclient',
                 args         = '--cflags --libs',              
                 uselib_store = 'LIBVNCCLIENT')

def build(bld):
   bld(target   = 'vmmon',
       features = 'qt4 cxx cxxprogram',
       uselib   = [ 'QTGUI',
                    'QTNETWORK',
                    'LIBVNCCLIENT',
                    'LIBSSH2',
                  ],
       source   = [ 'vmman.cpp',
                    'SessionsWindow.cpp',
                    'LoginDialog.cpp',
                    'AddSessionDialog.cpp',
                    'VNCWindow.cpp',
                    'SSHCredentials.cpp',
                    'SSHCommand.cpp',
                    'SSHPortConnection.cpp',
                    'SSHUploadFile.cpp',
                    'SubmitJobThread.cpp',
                    'BusyButton.cpp',
                    'strbuf.c',
                    # libssh2 C-only wrapers
                    'libssh2_auth.c',
                    'libssh2_exec.c',
                    'libssh2_forward_port.c',
                    'libssh2_scp_write.c',
                    # from krdc
                    'remoteview.cpp',
                    'vncview.cpp',
                    'vncclientthread.cpp',
                  ])
