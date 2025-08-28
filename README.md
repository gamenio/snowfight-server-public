SnowFight Go is a multiplayer online survival snowball fight game. Written in C++, it supports multiple platforms, such as Windows and Linux. The service framework can support other types of games.
## Running on Windows
### Requirements
> Windows 10 1903 (19H1) or their servers versions (fully updated 2019) and up  
> Boost ≥ 1.58  
> CMake ≥ 3.2.0  
> SQLite ≥ 3.12.2  
> MS Visual Studio ≥ 14.0 (2015)
### Compiling the Source
Enter the following commands in CMD  

    > cd <source_root>  
    > mkdir build  
    > cd build  
    > cmake -G "Visual Studio 14 2015 Win64" ../ -DCMAKE_INSTALL_PREFIX="./bin" -DCMAKE_BUILD_TYPE=DEBUG -DWITH_COREDEBUG=TRUE 

Open the `snowfight_server.sln` in the `build` directory with MS Visual Studio and compile the project.  
### Running the Server
After the build is complete, you can find the files required for server runtime in the `<source_root>\build\bin\<config>` directory.  
- Rename the following server configuration files:  
`worldserver.conf.dist -> worldserver.conf`  
`ntsserver.conf.dist -> ntsserver.conf`  
`authserver.conf.dist -> authserver.conf`   
- Run `import_auth_sql.bat` and `import_world_sql.bat` in the `scripts` directory to generate DB files for the server.  
- Finally, run `worldserver.exe`, `ntsserver.exe`, and `authserver.exe` separately.  

## Running on Linux
### Requirements
> Boost ≥ 1.58  
> CMake ≥ 3.2.0  
> Clang ≥ 3.5 or GCC ≥ 6.3.0  
> zlib ≥ 1.2.7  
### Compiling the Source
Enter the following commands in Shell  

    $ cd <source_root>  
    $ mkdir build
    $ cd build
    $ cmake -G "Unix Makefiles" ../ -DCMAKE_INSTALL_PREFIX="./bin" -DCMAKE_BUILD_TYPE=RELWITHDEBINFO
    $ make install

### Running the Server
After the build is complete, you can find the files required for server runtime in the `<source_root>\build\bin` directory.  
Rename the following server configuration files  

    $ mv ./authserver.conf.dist ./authserver.conf
    $ mv ./worldserver.conf.dist ./worldserver.conf
    $ mv ./ntsserver.conf.dist ./ntsserver.conf

Generate DB files for the server  

    $ sh ./scripts/import_world_sql.sh
    $ sh ./scripts/import_auth_sql.sh

Start the server in the background  

    $ setsid ./worldserver &
    $ setsid ./ntsserver &
    $ setsid ./authserver &

