sudo apt-get update -y
sudo apt-get install gcc-12 g++-12 -y
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 100 --slave /usr/bin/g++ g++ /usr/bin/g++-12
echo "gcc-12 is installed"
gcc --version
