RagnarokServer / Hercules
========

About this Fork
-----------------
This fork aims to be used for Large-Scale Ragnarok Servers (Multiple Map-Servers, 400+ Users).


What's different?
-----------------
  - Runs Only on Microsoft Windows NT 6.1+ (Windows Server 2008 / Windows 7)
  - Dropped Hercules Plugin Support 
  - AMD64/64Bit recommended (32bit won't be tested, and maybe you won't be able to compile the source)
  - Microsoft SQL instead of MySQL
  - AEGIS compatible Database Layout
  - Support for AEGIS NPC's (you may need to 'convert' some creepy stuff manually)
  - Support for LUA Based NPC's (Implemented against luajit)
  - Several Threading Optimizations -> Threaded Networking, Threaded SQL, Threaded InstantMap;  so a QuadCore is Recommended  


Current Status
-----------------
Many of the Features mentioned above are still under Development or are missing
it's not recommended to use this Software in Production Environments.

More Details about the specific changes will be made public soon.


Maintainer
-----------------
Florian Wilkemeyer (Sirius_Black) - <fw@f-ws.de>

