sudo rm -rf /tmp/fuse-ftp >/dev/null 2>&1
mkdir /tmp/fuse-ftp >/dev/null 2>&1
cd build/example >/dev/null 2>&1
sudo rm -rf /mnt/fuse >/dev/null 2>&1
sudo mkdir /mnt/fuse >/dev/null 2>&1
sudo chmod 777 /mnt/fuse >/dev/null 2>&1
sudo ./fuse-ftp /mnt/fuse -d
sudo umount /mnt/fuse >/dev/null 2>&1
