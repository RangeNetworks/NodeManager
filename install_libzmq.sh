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

echo "# checking for root user"
if [ "$EUID" -ne 0 ];
	then echo "Please run the command as 'sudo $0 $@'"
	exit
fi

echo "# adding additional repo tools"
apt-get install -y software-properties-common python-software-properties || echo "# Installation of software-properties-common and pyton-software-properties installation failed!"
echo

echo "# adding modern zeromq repository"
add-apt-repository -y ppa:chris-lea/zeromq || echo "# Adding modern zeromq repository failed. First fix adding the command 'sudo add-apt-repository -y ppa:chris-lea/zeromq '"
echo

echo "# updating repositories"
apt-get update || echo "# apt-get update failed!"
echo

echo "# installing modern zeromq packages"
apt-get install -y libzmq3-dev libzmq3 python-zmq || echo "# Installation of libzmq3-dev libzmq3 python-zmq failed!"
echo

echo "# done"
