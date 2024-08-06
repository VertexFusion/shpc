# shpc - The shape file compiler

![MIT License](https://img.shields.io/github/license/dotnet/aspnetcore?color=%230b0&style=flat-square)

The SHP compiler converts AutoCAD® shape files into the corresponding SHX fonts.

## Overview

Shapes are objects similar to blocks. They can be inserted into a drawing and manipulated in a similar way to blocks. 
However, their main use is as font definitions. This involves creating SHP files in an ordinary text editor and then 
converting them to SHX files using the SHP Compiler.

Manual procedure:
1. Create a SHP file in a text editor. The SHP file is ASCII coded.
2. Compile the SHP file with the SHP compiler.
3. Integrate the compiled file into the CAD system.

## Operation

The SHP compiler is a command line tool and therefore has no graphical user interface. To run the compiler, you must open the terminal.

In the simplest case you executes the programme with:
~~~
shpc filename.shp
~~~

Further options are given with:
~~~
shpc -h
~~~

## Compiling

### Linux / macOS

Compiling is straight forward:

#### Prerequisites
Install the necessary packages from github:
~~~
git clone --recursive https://github.com/VertexFusion/libcore
git clone https://github.com/VertexFusion/shpc.git
~~~

#### Compile
~~~
ch shpc
make
~~~

#### Install
~~~
sudo make install
~~~

#### Clean Build Folder
~~~
make clean
~~~

#### Uninstall
~~~
sudo make clean
~~~

## Further information
Further information can be found here: https://vertexfusion.org/doc/shpc/
