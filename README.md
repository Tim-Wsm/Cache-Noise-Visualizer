# A Cache Noise Analyze and Visualize Program

# Introduction

The following sections explain how to use the profiler and the
visualizer programs and what environment is needed to run them
successfully. The second chapter describes most of the implementation
details and the design decisions.

## Limitations

The profiler program is only available on `x86_64` Intel machines which
run a modern Linux Operating System. The visualization program runs on
any platform which supports Python3. Optimizations like SMT
(Simultaneous Multithreading) or dynamic frequency scaling which are
present in modern CPU’s can falsify the measurement. In order to get
plausible results optimizations should be turned off. More information
will be given in the following sections.  
The profiler profiles level one and level two caches. To do so the
profiler requires root permissions. This will be explained in the
following sections as well. The used performance counter is not
available in virtual machines. Therefore, it is important not to run the
profiler in a virtual machine.

## User Guide

This project provides two separate programs to allow measurement and
visualization of the cache noise. The profiler program which allows
measurement is written in C and the visualization program is written in
Python. The measurement data is saved in the HDF5 file format. To use
these programs it is required to have access to a C compiler which
supports the C11 standard (preferred GCC or Clang) and a Python3
interpreter. The following steps demonstrate how to run these programs.
The following libraries are needed in order to run the programs.  

<table>
<thead>
<tr class="header">
<th style="text-align: left;">Package</th>
<th style="text-align: left;">Type</th>
<th style="text-align: left;">Usage</th>
</tr>
</thead>
<tbody>
<tr class="odd">
<td style="text-align: left;">GCC / Clang</td>
<td style="text-align: left;">System library</td>
<td style="text-align: left;">compiler</td>
</tr>
<tr class="even">
<td style="text-align: left;">GNU make</td>
<td style="text-align: left;">System library</td>
<td style="text-align: left;">build tool</td>
</tr>
<tr class="odd">
<td style="text-align: left;">HDF5</td>
<td style="text-align: left;">System library</td>
<td style="text-align: left;">file format for output files</td>
</tr>
<tr class="even">
<td style="text-align: left;">python3</td>
<td style="text-align: left;">System library</td>
<td style="text-align: left;">python interpreter</td>
</tr>
<tr class="odd">
<td style="text-align: left;">numpy</td>
<td style="text-align: left;">Python library</td>
<td style="text-align: left;">various matrix operations in python</td>
</tr>
<tr class="even">
<td style="text-align: left;">mathplot</td>
<td style="text-align: left;">Python library</td>
<td style="text-align: left;">ploting graphs in python</td>
</tr>
<tr class="odd">
<td style="text-align: left;">h5py</td>
<td style="text-align: left;">Python library</td>
<td style="text-align: left;">reading the result file in python</td>
</tr>
<tr class="even">
<td style="text-align: left;">doxygen*</td>
<td style="text-align: left;">System library</td>
<td style="text-align: left;">generating the profiler documentation</td>
</tr>
</tbody>
</table>

  

*Notice: Packages marked with \* are optional.*

### Prepare

1.  The system libraries need to be installed, one of the following
    commands depending on the environment can be used.
    
    1.  Debian
            9
        
            > sudo apt-get install gcc make pkg-config libhdf5-dev linux-tools python3 python3-pip python3-tk
            > sudo apt-get install doxygen
    
    2.  Ubuntu 16.04/ Ubuntu
            17.10
        
            > sudo apt-get install gcc make pkg-config libhdf5-dev linux-tools-common python3 python3-pip python3-tk
            > sudo apt-get install doxygen
    
    3.  Fedora
            27
        
            > sudo yum install gcc make pkg-config hdf5-devel perf python3 python3-pip python3-tkinter
            > sudo yum install doxygen
    
    4.  Arch Linux/Manjaro
        
            > sudo pacman -S gcc make pkg-config hdf5 perf python
            > sudo pacman -S doxygen

2.  The python libraries are installed with the following command.
    
        > sudo pip3 install -r requirements.txt

### Compile

The profiler is built by GNU make with the following command.

    > make clean && make release

After a successful compilation the `profiler` executable is available in
’bin/release’ with the name ’profiler’.

### Enable Linux Features

The profiler relies on Linux features which need to be enabled before
the program can run. Following scripts can be executed with root
permissions to enable them:

  - **enable/disable performance counter**
    
        > sudo python3 tools/toggleRDPMC.py

  - **enable/disable hugepages**
    
        > sudo python3 tools/toggleHugepages.py

  - **enable/disable SMT (hyper threading)** - *Optional*
    
        > sudo python3 tools/toggleSMT.py

But setting the kernel parameters can also be done manually without
scripts.

### Run the Profiler

The run of profiler is used to profile different scenarios like a single
CPU core with or without workload.

  - **with a workload (using a running process)**
    
        > sudo ./bin/release/profiler profile --pid <PID>

  - **with a workload (spawning a new
        process)**
    
        > sudo ./bin/release/profiler profile --program /home/user/myApp --program-args "hello world"

  - **without a workload on CPU core three into the file data.h5**
    
        > sudo ./bin/release/profiler profile -o data.h5 -c 3

In order to visualize the results they need to be saved into a file.
This can be done by adding the argument `-o <FILE>` to the program. It
is also possible to specify either an amount of iterations or a duration
in seconds. If one of these flags are set, then the profiler will not
run endlessly. After termination of a specified external program the
profiler stops.  
For more details run the following command.

    > ./bin/release/profiler --help

### Visualize the Results

After saving the results from the previous steps in a `<FILE>` the
visualizer is used to generate heat maps or bar charts. The visualizer
can also generate images or videos.

  - **animated heatmap**
    
        > python3 tools/visualize.py <FILE> --graph heatmap --type video

  - **animated bar
        chart**
    
        > python3 tools/visualize.py <FILE> --graph bar_chart --type video

  - **image heatmap**
    
        > python3 tools/visualize.py <FILE> --graph heatmap --type image

  - **image bar chart** – `Can take a long time to
        render.`
    
        > python3 tools/visualize.py <FILE> --graph bar_chart --type image

To save an image/video add the `-o <FILE>` to the given examples. It is
also possible to change the graph size. By adding `—-max <VALUE>` to the
arguments the heatmap and the bar char will cut off values which are
higher than the given max value. In a similar way lower values can be
cut off by using `—-min <VALUE>`. But this only works for heatmaps. To
find these minimum and maximum values use `—-stats -t image` or `—-stats
-t video`. These arguments provide statistical information about the
whole data set. Notice that statistical content depends on the display
type (video or image).  
More details about arguments for visualizer can be found by typing the
following command.

    > python3 tools/visualize.py --help

### Documentation

To generate the documentation for the profiler source code use the
following command.

    > make doc

After running this command the generated documentation can be found in
the `’doc’` directory. There are three different formats of
documentation available.

  - html

  - man

  - latex

### Testsystems

<table>
<thead>
<tr class="header">
<th style="text-align: left;">OS</th>
<th style="text-align: left;">CPU</th>
<th style="text-align: left;">Kernel</th>
<th style="text-align: left;">Compiler</th>
<th style="text-align: left;">Python</th>
<th style="text-align: left;">C-Library (gLib)</th>
</tr>
</thead>
<tbody>
<tr class="odd">
<td style="text-align: left;">Arch Linux</td>
<td style="text-align: left;">Intel Core i3-5005U</td>
<td style="text-align: left;">4.15.7-1-ARCH</td>
<td style="text-align: left;">clang 5.0.1</td>
<td style="text-align: left;">3.6.4</td>
<td style="text-align: left;">2.26</td>
</tr>
<tr class="even">
<td style="text-align: left;">Arch Linux</td>
<td style="text-align: left;">Intel Xeon E3-1231 v3</td>
<td style="text-align: left;">4.15.8-1-ARCH</td>
<td style="text-align: left;">clang 5.0.1</td>
<td style="text-align: left;">3.6.4</td>
<td style="text-align: left;">2.26</td>
</tr>
<tr class="odd">
<td style="text-align: left;">Manjaro</td>
<td style="text-align: left;">Intel Core i5-6200U</td>
<td style="text-align: left;">4.15.5-1-MANJARO</td>
<td style="text-align: left;">gcc 7.3.0</td>
<td style="text-align: left;">3.6.4</td>
<td style="text-align: left;">2.26</td>
</tr>
<tr class="even">
<td style="text-align: left;">Manjaro</td>
<td style="text-align: left;">Intel Core i7-3770k</td>
<td style="text-align: left;">4.15.7-1-MANJARO</td>
<td style="text-align: left;">gcc 7.3.0</td>
<td style="text-align: left;">3.6.4</td>
<td style="text-align: left;">2.26</td>
</tr>
<tr class="odd">
<td style="text-align: left;">Ubuntu 16.04</td>
<td style="text-align: left;">Intel Core i3-4030U</td>
<td style="text-align: left;">4.13.0-36-generic</td>
<td style="text-align: left;">gcc 5.4.0</td>
<td style="text-align: left;">3.5.2</td>
<td style="text-align: left;">2.23-0ubuntu10</td>
</tr>
<tr class="even">
<td style="text-align: left;">Ubuntu 17.10</td>
<td style="text-align: left;">Intel Core i3-4030U</td>
<td style="text-align: left;">4.13.0-37-generic</td>
<td style="text-align: left;">gcc 7.2.0</td>
<td style="text-align: left;">3.6.3</td>
<td style="text-align: left;">2.26-0ubuntu2.1</td>
</tr>
<tr class="odd">
<td style="text-align: left;">Fedora 27</td>
<td style="text-align: left;">Intel Core i3-4030U</td>
<td style="text-align: left;">4.13.9-300-fc27</td>
<td style="text-align: left;">gcc 7.3.1</td>
<td style="text-align: left;">3.6.4</td>
<td style="text-align: left;">2.26</td>
</tr>
<tr class="even">
<td style="text-align: left;">Debian 9</td>
<td style="text-align: left;">Intel Core i3-4030U</td>
<td style="text-align: left;">4.9.82-1+deb9u3</td>
<td style="text-align: left;">gcc 6.3.0</td>
<td style="text-align: left;">3.5.3</td>
<td style="text-align: left;">2.24-11+deb9u3</td>
</tr>
</tbody>
</table>

only works after reboot

