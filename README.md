hpcvmmon
========

## REQUIREMENTS
  * Qt (4.6 or higher) -- http://qt-project.org
  * libssh2 (1.4.2 or higer) -- http://libssh2.org
  * libvncclient (0.9.9) -- http://libvncserver.sourceforge.net

## BUILDING
### Linux
You can install the dependencies from your package manager, or build and install from
source; just make sure your `PKG_CONFIG_PATH` environment variable can locate the `.pc` 
files for `QtGui`, `QtNetwork`, `libssh2`, and `libvncclient`.  Then you can build
`hpcvmmon` with the following command:

    ./waf configure build

### Mac OSX
I suggest using [Homebrew](http://mxcl.github.com/homebrew) to install the dependencies.

    brew install qt libssh2 libvncserver
    ./waf configure build
    
### Windows
`hpcvmmon` can be built using [MinGW & MSYS](http://www.mingw.org).  All dependencies have 
to be built and installed manually.

#### libz
    tar zxvf zlib-1.2.7.tar.gz
    cd zlib-1.2.7
    make -f win32/Makefile.gcc
    make -f win32/Makefile.gcc install \
       SHARED_MODE=1 \
       INCLUDE_PATH=/C/Library/libz/include \
       LIBRARY_PATH=/C/Library/libz/lib \
       BINARY_PATH=/C/Library/libz/bin

#### OpenSSL
    tar zxvf openssl-1.0.1c.tar.gz
    cd openssl-1.0.1c
    perl Configure mingw64 shared no-asm --prefix=/C/Library/OpenSSL
    make
    make install

#### libssh2
    tar zxvf libssh2-1.4.2.tar.gz
    cd libssh2-1.4.2
    ./configure --prefix=/C/Library/LibSSH2 \
       --with-libssl-prefix=/C/Library/OpenSSL \
       --with-libz-prefix=/C/Library/libz
    make
    make install

#### libvncserver
**TODO**

#### hpvvmmon
    ./waf configure build
    

## TODO
* Save qemu state / resume from saved state
* Integrate with OrangeFS
* 