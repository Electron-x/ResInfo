# ResInfo #

## Windows CE Resource Monitor ##

ResInfo displays the most important system resources of a Windows CE device in a multi-page dialog box and in the system tray of the taskbar. The program was developed platform independent (Handheld PC, Palm-size PC, Pocket PC) and can be compiled for all common processor architectures (ARM, MIPS, SuperH, x86). Prerequisite is at least Windows CE 2.0 and a generic platform with a shell.

**Main features:**
- Displays the state of the main and backup battery and the battery lifetime
- Displays the allocation of program memory and storage memory
- Displays the allocation of non-volatile storage media
- Displays information of currently executed processes and their visible main windows
- Displays the load of the central processing unit
- Displays the most important system information of the device

Further details can be found in the [ResInfo](ResInfo.htm) help file.

![Screenshot of ResInfo on a Pocket PC](http://www.wolfgang-rolke.de/wince/resinfo_ppc.gif) ![Screenshot of ResInfo on a Handheld PC](http://www.wolfgang-rolke.de/wince/resinfo_hpc.gif)

## Status of the project ##

The development of the application has been finished. It is not intended to add new features or adaptations to new platforms or even fix potential bugs. The project was published for reference purposes only.

## Building the project ##

You need Microsoft eMbedded Visual C++ 3.0 (as part of the eMbedded Visual Tools) and a compatible target platform SDK to build the project.

The project contains configurations for each Microsoft reference platform:
* Palm-size PC 2.01
* Palm-size PC 2.11
* H/PC Ver. 2.00
* H/PC Pro 2.11
* HPC 2000
* Pocket PC
* Pocket PC 2002
* Smartphone 2002

Note: Configurations with "200" in the label are for platforms based on Windows CE 2.0 (e.g. "ResInfo - Win32 (WCE MIPS) 200 Debug"). Configuration names with a "300" are applicable for platforms based on Windows CE 3.0. Labels without a number are for platforms based on Windows CE 2.11.

## License ##

Copyright (c) 2003 - 2011 by Wolfgang Rolke

Licensed under the [EUPL](https://joinup.ec.europa.eu/software/page/eupl), Version 1.2 - see the [Licence.txt](Licence.txt) file for details.

## Links ##
- [ResInfo web page](http://www.wolfgang-rolke.de/wince/)
- [HPC:Factor developer downloads](https://www.hpcfactor.com/developer/)
- [European Union Public Licence 1.1 & 1.2](https://joinup.ec.europa.eu/software/page/eupl)
- [EUPL Guidelines for users and developers](https://joinup.ec.europa.eu/collection/eupl/guidelines-users-and-developers)
- [Managing copyright information within a free software project](https://softwarefreedom.org/resources/2012/ManagingCopyrightInformation.html)