export CROSS_COMPILE=/home/bart/abc/arm-2009q3/bin/arm-none-eabi-
make clean
make
sudo python ./parallel_bringup_test_thinclient.py thinclient.cfg; sudo minicom
