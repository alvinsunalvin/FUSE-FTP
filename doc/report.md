FUSE-FTP——基于 FUSE 的 FTP 文件系统
=======

计 54 郑远航

计 54 秦一鉴

计 54 乔一凡

计 55 陈齐斌

## 一、实验背景

我们大作业的项目是利用 `FUSE` 实现一个用户态 `FTP` 文件系统。

### FUSE

FUSE (Filesystem in USErspace) 是一套暴露给操作系统内核的用户态文件系统接口。实现一个具体的文件系统需要两部分的支持：

* 一个 `fuse` 内核模块（在 `Linux` 内核中已经实现），会在内核模块被加载后被 `VFS` 调用。`fuse` 被加载后看起来像一个普通的文件系统模块，会获取文件系统的文件操作；当 `VFS` 发来文件操作请求之后，它将该请求转化为特定格式，并通过设备传递给用户空间进程
* 以及一个用户空间的 `fuse` 库（如 `Linux` 的 `libfuse`，`macOS` 的 `FUSE for macOS` 和 `Windows` 的 `Dokan`），用于提供相关函数与 `fuse` 内核模块进行通信，完成文件操作并将结果返回给 `fuse` 内核模块，内核模块再将其还原为 `Linux kernel` 需要的格式，并返回给 `VFS`，完成操作。

`FUSE` 的具体工作流程如下图所示（图片来源：`WikiPedia`）

![fuse struct](https://ws4.sinaimg.cn/large/006tKfTcly1fs7b6dmae6j30d009ggm5.jpg)

### FTP



## 二、实现概述

我们的实现主要分为两部分：

* 一部分是根据 `FTP` 协议实现 `FTP` 基本操作的接口，完成从 `FTP` 服务器读取，上传，访问目录，查看当前目录等基本 `FTP` 操作，并将这些接口提供给 `FUSE` 上层使用；
* 另一部分是利用已经实现好的 `FTP` 操作接口实现 `FUSE` 侧具体的文件系统操作相关函数，并通过 `FUSE` 完成对用户请求的响应。

我们在本地建立了一个缓存目录，将 `FTP` 服务器上的目录结构映射到本地缓存目录下。为了保证性能，我们仅在访问文件时才真正从服务器下载相应文件，避免大量无谓的下载；同样，我们也仅在访问目录时更新目录结构，尽量降低网络通信数据量。

以下我们也将分别就这两方面的实现进行详细说明。

## 三、具体实现

### FTP 端实现

#### 总体思路：

根据 `RFC 959` 协议，通过 `Wireshark` 抓取 `FTP` 操作中客户端和服务端之间数据包，分析客户端和服务端之间数据包的传输过程，并依此实现FTP操作。 

#### 函数 `ftp_login` 的实现：

用 `Wireshark` 抓取登录到 `FTP` 服务器时，客户端和服务端之间传输的数据包，如下图所示（服务端 `IP` 地址为 `162.211.224.25`，客户端 IP 地址为 `10.0.2.15`，下同）：

![](https://ws3.sinaimg.cn/large/006tKfTcly1fs7b8vtq12j31c20denck.jpg)

从上图中可以看出，登录FTP服务器时的过程如下：

1. 从客户端向服务端 21 端口建立 `TCP` 连接（即控制连接）；

2. 服务端返回220；

3. 客户端在控制连接中发送“USER 用户名”发送用户名信息；

4. 服务端返回331；

5. 客户端在控制连接中发送“PASS 密码”发送密码信息；

6. 服务端返回230（如果登录成功）；

7. 客户端在控制连接中发送`“SYST”`请求服务端系统信息；

8. 服务端返回215。

函数`ftp_login`实现了登录到`FTP`服务器的过程。

#### 函数 `ftp_get` 的实现：

用`Wireshark`抓取从`FTP`服务器上下载文件（数据连接使用被动模式，下同）时，客户端和服务端之间传输的数据包，如下图所示：

![](https://ws2.sinaimg.cn/large/006tKfTcly1fs7baa9s8oj31c20etqis.jpg)

从上图中可以看出，从`FTP`服务器下载文件的过程如下：

1. 客户端在控制连接中发送`“TYPE I”`，表明数据连接类型为二进制传输；

2. 服务端返回200；

3. 客户端在控制连接中发送“PASV”，表示建立被动模式的数据连接；

4. 服务端返回227，消息尾部括号中的六个整数中，前4个表示服务端`IP`地址（`IP`地址为162.211.224.25），后两个表示数据连接的服务端端口号的高8位和低8位，这里数据连接的服务端端口号为79×28+34=20258；
5. 从客户端向服务端建立`TCP`连接（即数据连接）；
6. 客户端在控制连接中发送`“RETR 文件名”`，表示要获取服务端的指定文件；
7. 服务端返回150并向数据连接中发送文件内容；
8. 服务端返回226，表示传输完成。

函数`ftp_get`实现了从`FTP`服务器下载文件的过程，并将数据连接中收到的数据写入到本地文件中，以完成文件的下载。

#### 函数 `ftp_put` 的实现：

用`Wireshark`抓取向`FTP`服务器上传文件时，客户端和服务端之间传输的数据包，如下图所示：

![](https://ws3.sinaimg.cn/large/006tKfTcly1fs7bcyxid6j31c20glqng.jpg)

从上图中可以看出，向`FTP`服务器上传文件的过程如下：

1. 建立被动模式的数据连接，同下载文件过程中的第1-5步；

2. 客户端在控制连接中发送`“STOR 文件名”`，表示要将文件传输到服务端，指定上传文件在服务端上的文件名；

3. 服务端返回150，表示客户端可以开始上传文件；

4. 客户端在数据连接中发送文件内容；

5. 服务器返回226，表示传输完成。

函数`ftp_put`从本地文件读取文件内容，并实现了将本地文件上传到FTP服务器的过程。

#### 其他辅助函数的实现：

1. `ftp_response`：

   获取服务端的`response`信息头部的三位数字。实现过程为：

   ① 读取服务端返回的信息；

   ② 获取信息的前3个字符；

   ③ 将信息的前3个字符转换为整数并作为函数返回值。

2. `ftp_data_socket`：

   建立被动模式的数据连接。实现过程同“从`FTP`服务器下载文件”的过程的第1-5步，返回数据连接的`socket`描述符。函数`ftp­_data_socket`的参数`type`表示数据连接的类型，值为”I”表示二进制传输，值为”A”表示ASCII传输。

### FUSE 端实现

#### 总体思路

在 `FUSE` 端，我们需要完成 `FUSE` 相关的文件操作，并在 `main` 函数中 调用 `fuse_main`，使 `libfuse` 在接收到 `fuse` 内核模块发送来的请求后可以调用我们实现的文件操作函数完成 `FTP` 操作并返回。

而我们实现的文件操作函数，基本上是把对给定 path 的 “本地文件” 的访问解析为最接近的 `FTP` 的指令，并封装为对文件的读写。同时让本地来缓存服务器上目录结构（而非文件内容），以正确返回本地文件系统的信息。

实现的函数有 `getattr`, `access`, `mkdir`, `rename`, `unlink`, `truncate`, `create`, `open`, `read`, `write`, `release` 等。

#### 文件操作具体实现

以传入 path、打开一个文件返回文件描述符 fd 的 `xmp_open`, `xmp_release` 函数为例：

```c
static int xmp_open(const char *path, struct fuse_file_info *fi)
{
    int res, fd;
    char cache_path[PATH_MAX], ftp_path[PATH_MAX];
    // 生成该文件在本地对应的临时文件的路径
    map_to_cache_path(path, cache_path);
    createMultiLevelDir(cache_path);
    // 找到文件对应在 FTP 服务器上的路径
    map_to_ftp_path(path, ftp_path);

    // 以 write only 的方式，打开本地缓存文件，其中包含了错误处理
    // 从 FTP 服务器上 get 文件内容，确保打开的是最新的文件
    fd = open(cache_path, O_WRONLY);
    if (fd == -1)
    {
        return -errno;
    }
    res = ftp_get(fd, ftp_path);
    if (res == -1)
    {
        return -errno;
    }
    close(fd);

    // 以用户原本所希望的文件的打开方式打开该文件
    res = open(cache_path, fi->flags);
    if (res == -1) {
        return -errno;
    }

    fi->fh = res;
    return 0;
}

static int xmp_release(const char *path, struct fuse_file_info *fi)
{
    // 找到该文件在本地对应的临时文件的路径
    char ftp_path[PATH_MAX];
    map_to_ftp_path(path, ftp_path);

    // 将用户已经读写完毕后的临时文件内容，发送到 ftp 服务器上进行文件的更新
    int res = ftp_put(fi->fh, ftp_path);
    if (res == -1)
    {
        res = -errno;
    }
    close(fi->fh);
    return res;
}
```

此类函数主要的实现过程抽象包含：

1. 找到该文件在 FTP 服务器上对应的路径，以及本地临时文件的路径；
2. 以特定的模式打开临时文件，从 FTP 服务器上获取关于该文件的信息，经过处理后写入本地临时文件；
3. 以请求的模式访问临时文件，作为该文件的信息返回。

#### 目录操作具体实现

与读写文件不同，用户操作目录时需要知道目录下所有文件的信息（如 ls 命令、GUI 中打开文件夹等），同时 FTP 的 `LIST` 命令返回内容与 fuse 中 `readdir` 要求的返回结果有较大差异，因此本函数的实现也是所有 fuse 函数中最复杂的。

首先需要提供正确的文件权限、修改时间等信息，我们从 FTP LIST 函数返回的内容中选择对应的部分，用 pipe 的方法调用 `chmod`, `touch -d` 等程序，将本地临时文件的这些信息修改为与 ftp 服务器上一样的信息。之后使用 readdir 的默认部分对临时目录进行相同的操作。

其中一个必要的优化是，在获取目录内容的同时无需下载文件内容。对于文件，我们只使用 touch 来创建具有相同 attr 的文件；对于目录，我们只使用 mkdir，而不递归获取子目录。这是由于 readdir 会被频繁调用（如 ls，cd，以及在 bash 中按 tab 自动补全目录下文件名称时）。而真正的下载文件内容则在 `xmp_open` 时进行。

#### 问题以及解决方法

在使用 ftp 接口的过程中，发现由于 fuse 相关的函数并行地被调用，导致 ftp 多次读写缓冲区时出现冲突。找到问题之后使用了 `pthread` 库中的 `pthread_mutex_lock`, `pthread_mutex_unlock`，将上述提到的 ftp 函数变为原子操作，化解了问题。

#### 不足以及优化想法

- `xmp_release` 函数在 close 文件时被调用，由于无法确定用户是否对其进行了更改，目前的方案是总是将该文件调用 ftp put，传到服务器上。但由于多数情况下用户对文件的访问以读为主，并且写具有一定的局部性，可以参考 rsync 的方法：首先服务器上计算校验和，在客户端上进行搜索、比对后，将修改部分使用增量同步的方式上传至 ftp 服务器，可以减少不必要的通信开销。但由于需要在 ftp 服务器端提供额外的函数（计算校验和等函数），并且整个算法实现过于复杂，我们仅找到此可以尝试的方案，没有具体实现。

## 四、效果展示

### 实验环境

我们使用了 `Linux` 的 `libfuse` 开源实现，并基于 `libfuse` 完成了 `FUSE-FTP` 文件系统。运行环境为 `Ubuntu 16.04 LTS`。具体编译运行的方法以及相关库依赖可以参考我们工程下的 `README` 文档。

### 运行效果

我们启动 `fuse-ftp`，并将 `ftp` 服务器目录映射到本地 `/mnt/fuse` 文件夹下。我们将分别使用图形化的文件管理器和终端进行效果展示：

* 图形化界面

![](https://ws1.sinaimg.cn/large/006tKfTcly1fs7cnr245bj30ol0g0tah.jpg)

我们的 `FUSE-FTP` 文件系统可以被文件管理器正常使用，包括查看，修改，创建删除等等。同时，可以看到经过一段时间的加载后文件的缩略图等信息可以正常显示，说明我们的实现可以很好地支持文件操作。

* 终端界面

![](https://ws2.sinaimg.cn/large/006tKfTcly1fs7cu4bqigj30j609l79o.jpg)

注意到服务器时间是 `UTC+0` 的，从图中我们可以看出文件的权限和修改时间均与 `FTP` 服务器同步，保证了文件权限和修改日期不会混淆。

## 五、心得体会



## 六、参考资料

[1] libfuse. [https://github.com/libfuse/libfuse](https://github.com/libfuse/libfuse)

[2] RFC 959 FTP 协议规范 [https://tools.ietf.org/html/rfc959](https://tools.ietf.org/html/rfc959)