rm -rf build
mkdir build
cd build
../meson/meson.py ..
ninja
sudo cp util/fusermount3 /usr/bin/fusermount3
sudo chown root:root /usr/bin/fusermount3
sudo chmod 4755 /usr/bin/fusermount3
