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
* Python 3
* [_Meson_](http://mesonbuild.com/). We have already integrated one in meson/
* [Ninja](https://ninja-build.org). We need version >= 1.5.1

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

`ftp.c`说明
------------

`int ftp_get_response(void)`：从ftp服务器读取一条response，返回response编号，response的内容存放在字符数组recv_buf中。

`void ftp_login(void)`：登录ftp服务器。如果登录失败直接报错退出。

`int ftp_data_socket(void)`：与ftp服务器建立数据连接。返回数据连接的socket描述符，若建立数据连接错误则返回-1。

`int ftp_get(int fd, char* filename)`：从ftp服务器上下载文件到本地文件。fd是本地文件描述符（必须以写方式打开），filename是远程文件名。若下载成功则返回0，下载失败则返回-1。

`int ftp_put(int fd, char* filename)`：将本地文件上传到ftp服务器。fd是本地文件描述符（必须以读方式打开），filename是远程文件名。若上传成功则返回0，上传失败则返回-1。

提示
------------

1. 在文件`ftp.c`和`ftp.h`中编写FTP命令相关函数，在`fuse-ftp.c`中编写FUSE文件系统相关函数。
2. 在文件`ftp.c`中，除函数`ftp_login`外，不要使用assert。
3. 可以使用Wireshark抓取网络数据包来分析FTP命令实现细节。