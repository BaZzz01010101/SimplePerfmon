# SimplePerfmon
Simple cross platform library for acquiring main performance stats like CPU load and disk activity

## Overview
This project has been written in C++ and uses no 3rd party libraries.  
It was created to be small and simple as much as possible.  
Only Windows and Linux platforms supported for now.  

Next multidisk volumes are supported:
- spanned and striped volumes in Windows environment
- mdadm software raid volumes in Linux environment

In the case the volume is located on multiple physical disks, bottlenecked disk with  
the maximum activity will be taken into account.  

## License
This software released under [GNU GPL v3](http://www.gnu.org/licenses/gpl.html)
