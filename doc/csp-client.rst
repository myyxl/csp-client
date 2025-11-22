.. CSP Client Documentation

.. |csp-client| replace:: CSP Client

************
Introduction
************

This manual describes the |csp-client| software package.

The |csp-client| includes the client interfaces for most GomSpace standard product.
It can be used for interfacing to e.g. NanoCom AX100 or NanoCam C1U. 
It can also be used as an example of how to develop GomSpace compatible applications for Linux.

The basis of the |csp-client| is the individual modules and their functionalities. The following modules are included:
    
* **libcsp**     Network library
* **libgosh**    Shell interface
* **liblog**     Logging systems
* **libutil**    Various utilities
* **libftp**     File transfer (client interface)
* **libparam**   Parameter system (client interface)
* **clients**    Client interface for individual products

Installation and build instructions are described in the next sections.

|csp-client| Installation and Build
###################################

Source Code Installation
************************

Install Source Code:

#. Create a workspace folder for the source code, e.g. ~/workspace: `mkdir ~/workspace`
#. Change working directory to the workspace folder: `cd ~/workspace`
#. Copy source code tar ball (`csp-client-<version>.tar.bz2`) to ~/workspace
#. Unpack source code: `tar xfv csp-client-<version>.tar.bz2`

You now have the source code installed in `~/workspace/csp-client-<version>/`

Building the Source Code
************************

Prerequisites: Recent Linux with python (example build with ubuntu 14.10)

Install requirements::

    $ sudo apt get install build-essential libzmq3-dev

The executable build script is called `waf` and is placed in the root-folder of the software repository.
Waf uses a recursive scheme to read the local `wscript` file(s), and submerge into the `lib/lib. . . ` folders and
find their respective `wscript` files. This enables an elaborate configuration of all modules included in the |csp-client|.

To configure and build the source code:

#. Change directory to source code folder: `cd ~/workspace/csp-client-<version>`
#. Configure source code: `waf configure`. This will configure the source code and check that the tool chain (compiler, linker, ...) is available
#. Build source code: `waf build` (or just `waf`). 

The `waf build` command will compile the source code, link object files and generate an executable in the build folder with the name `csp-client`.

Client Modules
**************

To include all client modules from the clients sub-folder::

    $ waf configure

To include selected client modules (e.g. nanopower and nanocom-ax) from the clients sub-folder::

    $ waf configure --with-clients=nanopower,nanocom-ax

Remember to re-build source after a `waf configure`::

    $ waf build

Command line arguments
======================

The |csp-client| main application csp-client has the following command line arguments::

    -a Address
    -c CAN device
    -d USART device
    -b USART buad
    -z ZMQHUB server

Example 1: Starting csp-client with address 10 and connecting to a ZMQ proxy on localhost::

    $ ./build/csp-client -a 10 -z localhost
    
Example 2: Starting csp-client with address 10 using usart to /dev/ttyUSB0 at baudrate 500000::

    $ ./build/csp-client -a 10 -d /dev/ttyUSB0 -b 500000
    
Example 3: Starting csp-client with address 10 using CAN interface can0::

    $ ./build/csp-client -a 10 -c can0
