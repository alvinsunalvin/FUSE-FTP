sudo rm -rf /tmp/fuse-ftp
mkdir /tmp/fuse-ftp
cd build/example
sudo rm -rf /mnt/fuse
sudo mkdir /mnt/fuse
sudo ./fuse-ftp /mnt/fuse -d
umount /mnt/fuse