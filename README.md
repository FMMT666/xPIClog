
xPIClog
=======

A simple, but quite versatile and configurable, long-time 8 channel SD-card data logger.
Powered by a PIC18 and [SourceBoost][1] Compiler.

For hardware and more info, visit [www.askrprojects.net/hardware/xpiclog/index.html][2]

Unfortunately, all the source code files can not be put in a subdirectory.
This is a limitation of MPLab in conjunction with SourceBoost.
Doesn't feel right, but doesn't hurt either...

---

## Applications

  - [9V battery test][3]
  - [1.5V battery test][4]
  - ...

---

## TODO

  - fix date limits (>31 days)
  - add command to browse SD-card contents
  - outsmart Sourceboost to use directories
  - provided compiled lib (depends on license)
  - add Scilab scripts for 1V5 and 9V0 measurements
  - enable the external trigger mechanism
  - update (or rewrite) SD-card library
  - rewrite for XC8 compiler (maybe)
  - ...


---

## CHANGES

          
### V0.8a
    - LED now on in MENU MODE (half brightness)
    - continue to write to card after overflow (previous write error) 


Have fun  
FMMT666(ASkr)  


[1]: http://www.sourceboost.com
[2]: http://www.askrprojects.net/hardware/xpiclog/index.html
[3]: http://www.askrprojects.net/other/battest/index.html
[4]: http://www.askrprojects.net/other/battest2/index.html
