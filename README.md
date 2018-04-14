FUSE-FTP
=======

A FTP file system based on FUSE

Supported Platforms
-------------------

* Linux (fully)
* BSD (mostly/best-effort)
* For OS-X, please use [OSXFUSE](https://osxfuse.github.io/)

Dependency:
* libfuse. This project is based on libfuse and there is already one.
* Python 2
* [_Meson_](http://mesonbuild.com/). We have already integrated one in meson/
* [Ninja](https://ninja-build.org)

Installation
------------

    $ mkdir build; cd build
    $ ../meson/meson.py ..

To build, test and install libfuse and FUSE-FTP, you then use Ninja:

    $ ninja
    $ sudo ninja install

Run FUSE-FTP
------------

    $ cd example/
    $ sudo ftp $YOUR_MOUNT_DIR$ [-d]