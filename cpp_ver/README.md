# C++ version

## Descriptions
- This code calculated time difference of arrival (TDOA) (unit: sample) between two receiver channels
- The following processes were tested on the debian 7.9 on the Beaglebone Black (Texas Instruments) also on the Ubuntu 16.04 on Macbook Pro 2016

## Usage
- First, you need to install "cmake" (at least version 2.8.11)
- On the Linux like system, type the followings on the console

```
sudo apt-get install cmake
```

- This code uses DSP library [Aquila](https://github.com/zsiciarz/aquila). Please clone the project on your system. Folder name of the cloned project may be "aquila-master"
- make a new folder, for example "aquila" and move "aquila-master" into "aquila". Make a folder and set name of the folder as "aquila-build"
- Finally, the directory tree would be

```
aquila
   |
   |-- aquila-build
   |-- aquila-master
```

- Go to "aquila/aquila-build" and type

```
sudo cmake ../aquila-master
sudo make
sudo make install
```
- For testing the code, substitute "aquila/aquila-master/example/fft_filter.cpp" with "[cpp_ver/fft_filter.cpp](https://github.com/snuuwal/2016_robotx_acoustic_pinger_based_transit/blob/master/cpp_ver/fft_filter.cpp)" in this repository
- Return to "aquila/aquila-build" and type ```sudo make examples``` which builds our code
- The result is in the "aquila/aquila-build/fft_filter/" in the form of excutable file (file name: fft_filter)
- Copy the excutable file to wherever you want, for example "/home/Desktop/test_folder"
- Copy "[data/Measurement_1.dat](https://github.com/snuuwal/2016_robotx_acoustic_pinger_based_transit/blob/master/data/Measurement_1.dat)" into "/home/Desktop/test_folder"
- You can run the file by typing the following on "/home/Desktop/test_folder"
```
sudo ./fft_filter
```
- It will return -151 which is TDOA (unit: sample) between two receiver channels
- You can test with the other sample "[data/Measurement_1.dat](https://github.com/snuuwal/2016_robotx_acoustic_pinger_based_transit/blob/master/data/Measurement_2.dat)". It will return TDOA of -54 
