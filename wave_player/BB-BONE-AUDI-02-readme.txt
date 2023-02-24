BB-BONE-AUDI-02-00A0.dtbo
--------------------------

NOTE Feb 2023:
- This file is no longer needed for newer releases of the BBG.
- If you are running Debian 11.x or newer on your BBG, then you don't need this file.
- When in doubt, don't use this file.


NOTE: Feb 2021
This is a copy of one of the files received when doing:
(bbg)$ sudo apt update
(bbg)$ sudo apt install bb-cape-overlays

Copied from upgrade which stated:
"Unpacking bb-cape-overlays (4.14.20201221.0-0~stretch+20201221) over (4.4.20180126.0-0rcnee0~stretch+20180126)"

Note that as of Feb 2021, doing the above command on the "BeagleBoard.org Debian Image 2018-01-28" image (as per /ID.txt)
will make the board unbootable; however, the audio cape works and is therefore copied here.

When using newer images (such as Debian 11.x 2022_11 or newer) the .dtbo file that it comes with works well 
and does not need the file in this folder.