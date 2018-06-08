/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  Copyright (C) 2011       Sebastian Pipping <sebastian@pipping.org>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
*/

/** @file
 *
 * This file system mirrors the existing file system hierarchy of the
 * system, starting at the root file system. This is implemented by
 * just "passing through" all requests to the corresponding user-space
 * libc functions. Its performance is terrible.
 *
 * Compile with
 *
 *     gcc -Wall passthrough.c `pkg-config fuse3 --cflags --libs` -o passthrough
 *
 * ## Source code ##
 * \include passthrough.c
 */


#define FUSE_USE_VERSION 31

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

#include "ftp.h"
#include "util.h"

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static void *xmp_init(struct fuse_conn_info *conn,
              struct fuse_config *cfg)
{
    (void) conn;
    cfg->use_ino = 1;

    /* Pick up changes from lower filesystem right away. This is
       also necessary for better hardlink support. When the kernel
       calls the unlink() handler, it does not know the inode of
       the to-be-removed entry and can therefore not invalidate
       the cache of the associated inode - resulting in an
       incorrect st_nlink value being reported for any remaining
       hardlinks to this inode. */
    cfg->entry_timeout = 0;
    cfg->attr_timeout = 0;
    cfg->negative_timeout = 0;

    return NULL;
}

static int xmp_getattr(const char *path, struct stat *stbuf,
               struct fuse_file_info *fi)
{
    (void) fi;
    int res;
    char cache_path[PATH_MAX];
    map_to_cache_path(path, cache_path);
    int fd = open(cache_path, O_WRONLY);
    // ftp_get(fd, path+1);
    close(fd);

    res = lstat(cache_path, stbuf);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_access(const char *path, int mask)
{
    int res;
    char cache_path[PATH_MAX], ftp_path[PATH_MAX];
    map_to_cache_path(path, cache_path);
    int fd = open(cache_path, O_WRONLY);

    map_to_ftp_path(path, ftp_path);
    res = ftp_get(fd, ftp_path);
    close(fd);

    res = access(cache_path, mask);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_readlink(const char *path, char *buf, size_t size)
{
    // TODO
    int res;
    char cache_path[PATH_MAX];
    map_to_cache_path(path, cache_path);

    res = readlink(cache_path, buf, size - 1);
    if (res == -1)
        return -errno;

    buf[res] = '\0';
    return 0;
}


static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
               off_t offset, struct fuse_file_info *fi,
               enum fuse_readdir_flags flags)
{
    // TODO: fix file attr
    DIR *dp;
    struct dirent *de;

    (void) offset;
    (void) fi;
    (void) flags;

    char *file_list_buffer = (char**) malloc(1<<14 * sizeof(char));

    char pwd[PATH_MAX] = {0};
    char ftp_path[PATH_MAX];
    map_to_ftp_path(path, ftp_path);
    if(ftp_dir(ftp_path, file_list_buffer) == -1) {
        return -errno;
    }

    map_to_ftp_path(path, pwd);

    int pwd_len = strlen(pwd);
    if (pwd[pwd_len - 1] != '/') {
        pwd[pwd_len] = '/';
    }

    int n_files = 0;
    char *delims = "\r\n";
    char *item = NULL;
    char *file_list[128] = {NULL};
    int isDir[128] = {0};
    fprintf(stderr, "buf: %s\n", file_list_buffer);
    item = strtok(file_list_buffer, delims);
    while (item)
    {
        isDir[n_files] = (int)(item[0] == 'd');
        int len = strlen(item), j;
        for (j = len - 1; j >= 0; j--)
        {
            if (item[j] == ' ') // do not support space in file name
            {
                item = item + j + 1;
                break;
            }
        }
        file_list[n_files] = (char *) malloc(PATH_MAX * sizeof(char));
        memset(file_list[n_files], 0, PATH_MAX);
        strcpy(file_list[n_files], pwd);
        strcat(file_list[n_files], item);
        if (isDir[n_files]) {
            len = strlen(file_list[n_files]);
            file_list[n_files][len] = '/';
        }

        n_files++;
        item = strtok(NULL, delims);
    }
    int i;
    fprintf(stderr, "BEGIN readdir n = %d:\n", n_files);
    for (i = 0; i < n_files; i++) {
        fprintf(stderr, "%s\n", file_list[i]);
    }
    fprintf(stderr, "END readdir:\n");


    char cache_path[PATH_MAX], base_path[PATH_MAX];

    for (i = 0; i < n_files; i++)
    {
        cut_ftp_path(file_list[i], base_path);
        map_to_cache_path(base_path, cache_path);

        int len = strlen(cache_path);
        if (isDir[i])
        {
            int res = mkdir(cache_path, 0755);
        }
        else
        {
            int fd = open(cache_path, O_CREAT, 0644);
            close(fd);
        }
        free(file_list[i]);
    }
    free(file_list_buffer);

    map_to_cache_path(path, cache_path);
    dp = opendir(cache_path);
    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0, 0))
            break;
    }

    closedir(dp);
    return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
    // TODO
    int res;
    char cache_path[PATH_MAX];
    map_to_cache_path(path, cache_path);

    /* On Linux this could just be 'mknod(path, mode, rdev)' but this
       is more portable */
    if (S_ISREG(mode)) {
        res = open(cache_path, O_CREAT | O_EXCL | O_WRONLY, mode);
        if (res >= 0)
            res = close(res);
    } else if (S_ISFIFO(mode))
        res = mkfifo(path, mode);
    else
        res = mknod(path, mode, rdev);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
    int res, res_cache;
    char cache_path[PATH_MAX], ftp_path[PATH_MAX];
    map_to_cache_path(path, cache_path);
    map_to_ftp_path(path, ftp_path);
    res = ftp_mkdir(ftp_path);
    fprintf(stderr, "res: %d\n", res);
    res_cache = mkdir(cache_path, mode);
    fprintf(stderr, "res_cache: %d\n", res_cache);

    if (res == -1 || res_cache == -1)
        return -errno;

    return 0;
}

static int xmp_unlink(const char *path)
{
    int res, res_cache;
    char cache_path[PATH_MAX];
    map_to_cache_path(path, cache_path);

    res_cache = unlink(cache_path);

    char ftp_path[PATH_MAX];
    map_to_ftp_path(path, ftp_path);
    res = ftp_rm(ftp_path);
    if (res == -1 || res_cache == -1)
        return -errno;

    return 0;
}

static int xmp_rmdir(const char *path)
{
    int res, res_cache;
    char cache_path[PATH_MAX], ftp_path[PATH_MAX];
    map_to_cache_path(path, cache_path);
    map_to_ftp_path(path, ftp_path);

    res_cache = rmdir(cache_path);

    res = ftp_rmdir(ftp_path);
    if (res == -1 || res_cache == -1)
        return -errno;

    return 0;
}

static int xmp_symlink(const char *from, const char *to)
{
    // TODO
    int res;
    res = symlink(from, to);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_rename(const char *from, const char *to, unsigned int flags)
{
    int res;

    if (flags)
        return -EINVAL;
    
    char ftp_from[PATH_MAX], ftp_to[PATH_MAX];
    map_to_ftp_path(from, ftp_from);
    map_to_ftp_path(to, ftp_to);
    res = ftp_mv(ftp_from, ftp_to);
    if (res == -1)
        return -errno;
    res = rename(from, to);
    if (res == -1) {
        return -errno;
    }
    return 0;
}

static int xmp_link(const char *from, const char *to)
{
    // TODO
    int res;
    res = link(from, to);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_chmod(const char *path, mode_t mode,
             struct fuse_file_info *fi)
{
    (void) path;
    (void) mode;
    (void) fi;
    return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid,
             struct fuse_file_info *fi)
{
    (void) fi;
    (void) uid;
    (void) gid;
    (void) fi;
    return 0;
}

static int xmp_truncate(const char *path, off_t size,
            struct fuse_file_info *fi)
{
    int res;
    char cache_path[PATH_MAX], ftp_path[PATH_MAX];
    map_to_cache_path(path, cache_path);
    map_to_ftp_path(path, ftp_path);

    if (fi != NULL)
        res = ftruncate(fi->fh, size);
    else
    {
        int fd = open(cache_path, O_RDWR);
        ftp_get(fd, ftp_path);
        res = ftruncate(fd, size);
        if (res != -1)
        {
            close(fd);
            return -errno;
        }
        else
        {
            ftp_put(fd, ftp_path);
            close(fd);
        }
    }

    if (res == -1)
        return -errno;

    return 0;
}

#ifdef HAVE_UTIMENSAT
static int xmp_utimens(const char *path, const struct timespec ts[2],
               struct fuse_file_info *fi)
{
    (void) fi;
    int res;
    char cache_path[PATH_MAX];
    map_to_cache_path(path, cache_path);

    /* don't use utime/utimes since they follow symlinks */
    res = utimensat(0, cache_path, ts, AT_SYMLINK_NOFOLLOW);
    if (res == -1)
        return -errno;

    return 0;
}
#endif

static int xmp_create(const char *path, mode_t mode,
              struct fuse_file_info *fi)
{
    int res;
    char cache_path[PATH_MAX], ftp_path[PATH_MAX];
    map_to_cache_path(path, cache_path);
    map_to_ftp_path(path, ftp_path);

    createMultiLevelDir(cache_path);
    res = open(cache_path, fi->flags, mode);
    if (res == -1)
        return -errno;

    int ftp_res = ftp_put(res, ftp_path);
    fi->fh = res;
    return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
    int res;
    char cache_path[PATH_MAX], ftp_path[PATH_MAX];
    map_to_cache_path(path, cache_path);
    map_to_ftp_path(path, ftp_path);

    createMultiLevelDir(cache_path);
    res = open(cache_path, fi->flags);
    if (res == -1)
        return -errno;

    res = ftp_get(res, ftp_path);
    // fprintf(stderr, "open res:%d, ftp path: %s, cache path: %s", res, ftp_path, cache_path);
    if (res == -1) {
        return -errno;
    }

    fi->fh = res;
    return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
            struct fuse_file_info *fi)
{
    int fd;
    int res;
    char cache_path[PATH_MAX], ftp_path[PATH_MAX];
    map_to_cache_path(path, cache_path);
    map_to_ftp_path(path, ftp_path);

    if (fi == NULL)
    {
        fd = open(cache_path, O_RDWR);
        res = ftp_get(fd, ftp_path);
        // fprintf(stderr, "read res:%d, ftp path: %s, cache path: %s", res, ftp_path, cache_path);
        if (res == -1) {
            return -errno;
        }
    }
    else
        fd = fi->fh;
    
    if (fd == -1)
        return -errno;

    res = pread(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    if(fi == NULL)
        close(fd);
    return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
             off_t offset, struct fuse_file_info *fi)
{
    int fd;
    int res;
    char cache_path[PATH_MAX], ftp_path[PATH_MAX];
    map_to_cache_path(path, cache_path);
    map_to_ftp_path(path, ftp_path);

    (void) fi;
    if(fi == NULL)
    {
        fd = open(cache_path, O_RDWR);
        res = ftp_get(fd, ftp_path);
    }
    else
        fd = fi->fh;
    
    if (fd == -1)
        return -errno;

    res = pwrite(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    if(fi == NULL)
    {
        res = ftp_put(fd, ftp_path);
        // fprintf(stderr, "ftp write fd: %d, path: %s, res: %d\n", fd, ftp_path, res);
        close(fd);
    }
    return res;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
    // TODO
    int res;
    char cache_path[PATH_MAX], ftp_path[PATH_MAX];
    map_to_cache_path(path, cache_path);
    map_to_ftp_path(path, ftp_path);

    res = statvfs(cache_path, stbuf);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_release(const char *path, struct fuse_file_info *fi)
{
    (void) path;
    char ftp_path[PATH_MAX];
    map_to_ftp_path(path, ftp_path);

    int res = ftp_put(fi->fh, ftp_path);
    if (res == -1) res = -errno;
    close(fi->fh);
    // fprintf(stderr, "ftp release fd: %d, path: %s, res: %d\n", fi->fh, path+1, res);
    return res;
}

static int xmp_fsync(const char *path, int isdatasync,
             struct fuse_file_info *fi)
{
    /* Just a stub.	 This method is optional and can safely be left
       unimplemented */

    (void) path;
    (void) isdatasync;
    (void) fi;
    return 0;
}

#ifdef HAVE_POSIX_FALLOCATE
static int xmp_fallocate(const char *path, int mode,
            off_t offset, off_t length, struct fuse_file_info *fi)
{
    int fd;
    int res;

    (void) fi;

    if (mode)
        return -EOPNOTSUPP;

    if(fi == NULL)
        fd = open(path, O_WRONLY);
    else
        fd = fi->fh;
    
    if (fd == -1)
        return -errno;

    res = -posix_fallocate(fd, offset, length);

    if(fi == NULL)
        close(fd);
    return res;
}
#endif

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int xmp_setxattr(const char *path, const char *name, const char *value,
            size_t size, int flags)
{
    int res = lsetxattr(path, name, value, size, flags);
    if (res == -1)
        return -errno;
    return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value,
            size_t size)
{
    int res = lgetxattr(path, name, value, size);
    if (res == -1)
        return -errno;
    return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
    int res = llistxattr(path, list, size);
    if (res == -1)
        return -errno;
    return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
    int res = lremovexattr(path, name);
    if (res == -1)
        return -errno;
    return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations xmp_oper = {
    .init       = xmp_init,
    .getattr	= xmp_getattr,
    .access		= xmp_access,
    .readlink	= xmp_readlink,
    .readdir	= xmp_readdir,
    .mknod		= xmp_mknod,
    .mkdir		= xmp_mkdir,
    .symlink	= xmp_symlink,
    .unlink		= xmp_unlink,
    .rmdir		= xmp_rmdir,
    .rename		= xmp_rename,
    .link		= xmp_link,
    .chmod		= xmp_chmod,
    .chown		= xmp_chown,
    .truncate	= xmp_truncate,
#ifdef HAVE_UTIMENSAT
    .utimens	= xmp_utimens,
#endif
    .open		= xmp_open,
    .create 	= xmp_create,
    .read		= xmp_read,
    .write		= xmp_write,
    .statfs		= xmp_statfs,
    .release	= xmp_release,
    .fsync		= xmp_fsync,
#ifdef HAVE_POSIX_FALLOCATE
    .fallocate	= xmp_fallocate,
#endif
#ifdef HAVE_SETXATTR
    .setxattr	= xmp_setxattr,
    .getxattr	= xmp_getxattr,
    .listxattr	= xmp_listxattr,
    .removexattr	= xmp_removexattr,
#endif
};

int main(int argc, char *argv[])
{
    umask(0);
    ftp_login();
    return fuse_main(argc, argv, &xmp_oper, NULL);
}
