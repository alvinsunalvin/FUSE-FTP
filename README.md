FUSE-FTP
=======

清华大学《存储技术基础》2018 课程作业，基于 FUSE 的 FTP 文件系统。

小组成员：

郑远航，陈齐斌，秦一鉴，乔一凡

平台支持
-------------------

* Linux (fully)
* BSD (mostly/best-effort)
* For OS-X, please use [OSXFUSE](https://osxfuse.github.io/)

我们已经在 `Ubuntu 16.04 LTS` 下测试运行成功，推荐在 `Ubuntu 16.04` 下安装运行。

## 库依赖

* [`libfuse`](https://github.com/libfuse/libfuse). 我们的项目本身是基于 `libfuse` 的，工程内已经内置了 `libfuse`。
* `Python 3`
* [_`Meson`_](http://mesonbuild.com/). 工程使用 `Meson` 和 `Ninja` 进行构建。我们已经在工程中内置了 `Meson`。位置在 `meson/`
* [`Ninja`](https://ninja-build.org). 我们需要 `Ninja` 版本 >= 1.5.1

安装
------------

从官方网站下载并安装 `Ninja`：

```bash
$ sudo apt-get install ninja-build
```

构建工程：

```bash
$ mkdir build; cd build
$ ../meson/meson.py ..
```

使用 `Ninja` 编译，测试，安装 `FUSE-FTP`：

```bash
$ ninja
$ sudo ninja install
```

**我们更推荐使用已经提供的脚本进行编译：**

```bash
# in $ROOT of FUSE-FTP project
$ bash build.sh
```

运行
------------

使用工程根目录下的 `run.sh` 脚本运行：

```bash
$ bash run.sh
```

`fuse-ftp` 会自动将远程 `FTP` 服务器根目录映射到本机 `/mnt/fuse` 目录下。

