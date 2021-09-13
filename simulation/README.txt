Simulation Folder contains the Simulation of Telematics SDK APIs. Currently Location APIs are simulated.

Further information is available in loc folder README.

This has been compiled and tested with GCC and G++ version 9.3.0 and Ubuntu versions 14.04, 20.04

Below contains instructions for Build

Run below instructions in snaptel-sdk/include

1. cmake .
2. cmake --build .
3. sudo make install

Run below instructions in snaptel-sdk/simulation

1. cmake .
2. cmake --build .
3. sudo make install

check where the libraries are installed and ensure that it is included in the the library path of the system environment

Modify CMakeLists.txt in snaptel-sdk/apps folder to include only tests/location_test/app subdirectory

Run below instructions in snaptel-sdk/apps

1. cmake .
2. cmake --build .
3. sudo make install


