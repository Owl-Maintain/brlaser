brlaser: Brother laser printer driver
=====================================

brlaser is an open-source CUPS driver designed specifically for Brother monochrome laser printers and multi-function devices.

While most Brother printers can use standard printer languages like PCL or PostScript, some models do not. If you have a monochrome Brother laser printer (or multi-function device) and the other open-source drivers are not working, brlaser might be able to help. Additionally, there have been reports of some non-Brother printers working with this driver.

The software is released under the GNU General Public License, which grants the right to freely use, distribute, and modify the program without requiring any permission from the software's author or any fees.


Supported Printers
------------------
This driver only supports Monochrome Laser Printers, any other printer types (e.g. Color Laser or InkJet) will not work with this driver.

The following printers have been reported to work with this driver:

* Brother DCP-1510 series
* Brother DCP-1600 series
* Brother DCP-1610W series
* Brother DCP-7010
* Brother DCP-7020
* Brother DCP-7030
* Brother DCP-7040
* Brother DCP-7055
* Brother DCP-7055W
* Brother DCP-7060D
* Brother DCP-7065DN
* Brother DCP-7070DW
* Brother DCP-7080
* Brother DCP-7080D
* Brother DCP-8065DN
* Brother DCP-B7500D series
* Brother DCP-L2500D series
* Brother DCP-L2510D series
* Brother DCP-L2520D series
* Brother DCP-L2520DW series
* Brother DCP-L2537DW
* Brother DCP-L2540DW series
* Brother DCP-L2550DW series
* Brother DCP-L2560DW series
* Brother FAX-2820
* Brother FAX-2840
* Brother HL-1110 series
* Brother HL-1200 series
* Brother HL-2030 series
* Brother HL-2130 series
* Brother HL-2140 series
* Brother HL-2150N
* Brother HL-2220 series
* Brother HL-2230 series
* Brother HL-2240 series
* Brother HL-2240D series
* Brother HL-2250DN series
* Brother HL-2260
* Brother HL-2260D
* Brother HL-2270DW series
* Brother HL-2280DW
* Brother HL-5030 series
* Brother HL-5040 series
* Brother HL-5140 series
* Brother HL-5370DW series
* Brother HL-5450DN series
* Brother HL-L2300D series
* Brother HL-L2305 series
* Brother HL-L2310D series
* Brother HL-L2320D series
* Brother HL-L2335D series
* Brother HL-L2340D series
* Brother HL-L2350DW series
* Brother HL-L2360D series
* Brother HL-L2370DN series
* Brother HL-L2375DW series
* Brother HL-L2380DW series
* Brother HL-L2390DW
* Brother HL-L2400DW
* Brother HL-L2402D
* Brother HL-L2405W
* Brother HL-L5000D series
* Brother MFC-1810 series
* Brother MFC-1910W series
* Brother MFC-7240
* Brother MFC-7320
* Brother MFC-7340
* Brother MFC-7360N
* Brother MFC-7365DN
* Brother MFC-7420
* Brother MFC-7440N
* Brother MFC-7460DN
* Brother MFC-7860DW
* Brother MFC-8440
* Brother MFC-8710DW
* Brother MFC-8860DN
* Brother MFC-9160
* Brother MFC-L2690DW
* Brother MFC-L2700DN series
* Brother MFC-L2700DW series
* Brother MFC-L2710DN series
* Brother MFC-L2710DW series
* Brother MFC-L2750DW series
* Fuji Xerox DocuPrint P265 dw
* Lenovo LJ2650DN

**Note**:
Some operating systems may not immediately update the driver they provide. This can cause supported printers to be missing from your system, or fixed bugs/issues to still be present on your installation.


Installation
------------

Most Linux and BSD operating systems already ship this driver. This is the case for at least Debian, Ubuntu, Raspbian, Fedora, openSUSE, Arch Linux, Gentoo, NixOS and Guix.

Look for a package named ``printer-driver-brlaser`` or ``brother-brlaser``.

You'll also need ``Ghostscript``, in case that's not installed automatically.

Once brlaser is installed, you can add your printer using the usual CUPS interface.


Testing Other Printers
----------------------

If your printer is not officially supported, please select either the ``Owl-Maintain/brlaser Test Driver`` or the ``Owl-Maintain/brlaser Test Driver Duplex`` to test if the driver works with your printer.

If you are able to successfully print, please submit a compatibility report by opening a new issue on GitHub and selecting "**Report Compatible Printer**".

Next, with your printer powered on, connect it to your computer via _USB_ or obtain your printer's _IP address_.

**For USB-connected printers:**
- Run the following command:

   ``sudo lpinfo --include-schemes usb -l -v``

- Example of output:
   ```
   Device: uri = usb://Brother/HL-2270DW%20series?serial=000000000000
           class = direct
           info = Brother HL-2270DW series
           make-and-model = Brother HL-2270DW series
           device-id = MFG:Brother;CMD:PJL,PCL,PCLXL;MDL:HL-2270DW series;CLS:PRINTER;CID:Brother Laser Type1;
           location =
   ```

**For network printers:**
- Run the following command:

   ``/usr/lib/cups/backend/snmp IP_ADDRESS_HERE``

- Example of output:
   ```
   INFO: Using default SNMP Community public
   network lpd://HL-2270DW-LAN/BINARY_P1 "Brother HL-2270DW series" "Brother HL-2270DW series" "MFG:Brother;CMD:PJL,PCL,PCLXL;MDL:HL-2270DW series;CLS:PRINTER;CID:Brother Laser Type1;" ""
   ```

Please provide the output of the command so that we can add the proper entry for your specific printer model in the driver.


Building from source
--------------------

To compile brlaser from source, you will need to have CMake and the CUPS development packages (such as cups-devel, libcups2-dev, or libcupsimage2-dev) installed on your system.

You can get the source code by cloning the Git repository or downloading the [latest release](https://github.com/Owl-Maintain/brlaser/releases/latest).

To compile and install the driver, use the following commands:

````
cmake .
make
sudo make install
````

Note that you may need to restart CUPS before the driver is loaded and ready to use.


Copyright
---------

Copyright © 2013 Peter De Wachter

brlaser is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

brlaser is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with brlaser.  If not, see <http://www.gnu.org/licenses/>.
