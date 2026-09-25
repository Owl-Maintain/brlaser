brlaser on macOS
================

macOS ships CUPS, so brlaser works on Macs — including Apple Silicon Macs, where Brother's
own driver (Intel-only) no longer loads without Rosetta. Three things differ from Linux:

* `sudo make install` fails: CMake targets `/usr/libexec/cups`, which macOS protects (SIP).
* macOS's cupsd has no filter search path for third-party locations, so the PPDs must
  reference the filter by absolute path.
* A binary downloaded from the internet carries a quarantine flag that keeps cupsd from
  executing it.

`install.sh` handles all three and puts the driver under `/Library/Printers/brlaser`.

Install from a release
----------------------

Every release carries a `brlaser-<version>-macos-arm64.zip` built by CI on an Apple Silicon
runner. Unpack it and run:

    sudo ./install.sh

Then add the printer in **System Settings → Printers & Scanners → Add Printer, Scanner, or
Fax…**, open the **Use** menu, choose **Select Software…** and search for `brlaser`.
Add it through System Settings, not `lpadmin`: that is what links the queue to the scanner
of a DCP/MFC (the "Open Scanner…" button), and the PPDs here carry `*APICADriver: True`
for those models.

Build from source
-----------------

Xcode Command Line Tools and CMake (`brew install cmake`) are enough; `ppdc` and
`cupstestppd` ship with macOS.

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build
    (cd build && ctest)
    ppdc build/brlaser.drv -d build/ppd
    sudo macos/install.sh build/rastertobrlaser build/ppd

Uninstall
---------

    sudo macos/uninstall.sh

removes the queues that use brlaser, the PPDs and the filter.

Notes
-----

* Apple has deprecated PPD-based drivers; they still work on current macOS.
* Scanning is not part of brlaser. For network DCP/MFC models Brother's ICA scanner
  package (a universal binary) or Image Capture handles it.
