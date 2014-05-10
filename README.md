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


## History

xsysguard was originally developped by Sascha Wessel (sawe), but nowadays the project is not maintained and the code is no longer compilable. The dependencies and their API evolved a lot during the 6 years from the last release in 2008. This repository is a Git clone of  [http://sourceforge.net/p/xsysguard/code/](the original SVN one) in version r841, which is readonly for everyone except the original author. The attempt to contact him was unsuccessful - no response. The aim is to make xsysguard compilable and runnable again, no new features are planned at the moment.
