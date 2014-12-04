#!/bin/sh

echo "# checking for a compatible host"
if hash lsb_release 2>/dev/null; then
	ubuntu=`lsb_release -r -s`
	if [ $ubuntu != "12.04" ]; then
		echo "# - WARNING : This script has only tested on Ubuntu 12.04, YMMV. Please open an issue if you've used it successfully on another version of Ubuntu."
	else
		echo "# - fully supported host detected: Ubuntu 12.04"
	fi
else
	echo "# - ERROR : Sorry, this script currently only supports Ubuntu as the host OS. Please open an issue for your desired host."
	echo "# - exiting"
	exit 1
fi
echo

echo "# adding additional repo tools"
sudo apt-get install software-properties-common python-software-properties
echo

echo "# adding modern zeromq repository"
sudo add-apt-repository ppa:chris-lea/zeromq
echo

echo "# updating repositories"
sudo apt-get update
echo

echo "# installing modern zeromq packages"
sudo apt-get install libzmq3-dev libzmq3 python-zmq
echo

echo "# done"
