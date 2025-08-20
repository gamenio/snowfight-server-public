SnowFight Go is a multiplayer online survival snowball fight game. Written in C++, it supports multiple platforms, such as Windows and Linux. The service framework can support other types of games.
## Compile on Windows
### Requirements
> Windows 10 1903 (19H1) or their servers versions (fully updated 2019) and up  
> Boost ≥ 1.58   
> CMake ≥ 3.2.0  
> MS Visual Studio (Community) ≥ 14.0 (2015) (Desktop) (Not previews)  
### Installation
Enter the following commands in CMD  
`cd <source_root>`  
`mkdir build`  
`cd build`  
`cmake -G "Visual Studio 14 2015 Win64" ../ -DCMAKE_INSTALL_PREFIX="./bin" -DCMAKE_BUILD_TYPE=DEBUG -DWITH_COREDEBUG=TRUE`  
