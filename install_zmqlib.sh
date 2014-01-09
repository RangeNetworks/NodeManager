#!/bin/sh

# install zmq
wget http://download.zeromq.org/zeromq-3.2.2.tar.gz
tar zxf zeromq-3.2.2.tar.gz
cd zeromq-3.2.2
automake -i
./configure
make
sudo make install
sudo ldconfig
cd ..

# install zmqcpp binding
git clone https://github.com/zeromq/cppzmq.git
sudo cp cppzmq/zmq.hpp /usr/local/include/

# install python binding
#sudo apt-get install python-setuptools
#sudo easy_install pyzmq
