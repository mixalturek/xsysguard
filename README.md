# xsysguard

**xsysguard** is a resource-friendly system monitor based on Imlib2.

![xsysguard screenshot](http://xsysguard.sourceforge.net/images/examples_smallblue.png)

[http://xsysguard.sourceforge.net/](http://xsysguard.sourceforge.net/)


## Features

- written in c, minimal dependencies
- powerful configuration file format
- full alpha channel support, including XShape and ARGB visuals
- configurable widgets: BarChart, LineChart, AreaChart, Text, Image, ...
- data sources implemented as modules
    - daemon module: run a xsysguardd daemon process on your linux based server, router, wlan accesspoint or satellite/cable receiver
    - file module: parse files with scanf or regular expressions (e.g. /proc/* or /sys/*)
    - exec module: execute processes and parse their stdout with scanf or regular expressions
    - ...


## Installation

### Manual Compilation

Install [all dependencies](http://xsysguard.sourceforge.net/). Download a tarbal from Releases/Tags.

```shell
tar -xf xsysguard-0.2.2.tar.gz
cd xsysguard-0.2.2

make config prefix=/usr/local
make

sudo make install
```

### Debian Package

Download a tarbal from Releases/Tags.

```shell
cp xsysguard-0.2.2.tar.gz xsysguard_0.2.2.orig.tar.gz
tar -xf xsysguard_0.2.2.orig.tar.gz
cd xsysguard-0.2.2

debuild -us -uc

sudo dpkg -i ../xsysguard_0.2.2-1_*.deb
```

Build inside Docker container to not mess OS by development packages.

```bash
cd docker
docker build --tag xsysguard-build .
docker run --rm -v `pwd`:/mnt xsysguard-build cp /build/xsysguard_0.2.2-1_amd64.deb /build/xsysguard-dbgsym_0.2.2-1_amd64.deb /mnt
docker rmi xsysguard-build
```


## Usage

See `xsysguard --help` or `man xsysguard` for instructions.

```shell
xsysguard MODULE &
xsysguard_smallblue &
xsysguard_smallgray &
xsysguard_bigblue &
xsysguard_biggray &
```


## ChangeLog

- xsysguard 0.2.2 (2020-02-04)
    - New configuration biggray/acpi.

- xsysguard 0.2.1 (2014-08-05)
    - Predefined bigblue and biggray configurations.
    - Dependency to fonts-dejavu-extra added (DejaVuSansCondensed-Bold font).

- xsysguard 0.2 (2014-06-11)
    - Scripts to run all smallgray or smallblue configurations.
    - Predefined smallblue and smallgray configurations.
    - Build scripts for creation of Debian package.
    - FIXMEs in manual pages solved.
    - Linker errors fixed by explicit linking with -lXext -lX11.
    - Issues found by cppcheck fixed.
    - libstatgrab API changed in version 0.90, statgrab module made compilable again.
    - Compiler warnings -Wall enabled and solved.
    - libsensors API changed, sensors module made compilable again.
    - libiw API changed, iw module made compilable again.
    - Development moved to [https://github.com/mixalturek/xsysguard](https://github.com/mixalturek/xsysguard).
    - Expanded documentation.
    - Improved Text widget performance.
    - Fixed xsysguardd command line option handling.

- xsysguard 0.1 (2008-04-06)
    - First public release.


## This Repository

xsysguard was originally developped by Sascha Wessel (sawe), but nowadays the project is not maintained and the code is no longer compilable. The dependencies and their API evolved a lot during the 6 years from the last release in 2008. This repository is a Git clone of  [the original SVN one](http://sourceforge.net/p/xsysguard/code/) in version r841, which is readonly for everyone except the original author. The attempt to contact him was unsuccessful - no response. The aim is to make xsysguard compilable and runnable again.

[https://github.com/mixalturek/xsysguard](https://github.com/mixalturek/xsysguard)
