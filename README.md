CurlDump Ayria Extension
---

This plugin allows for CURL dumping to an .acp file which can be opened by the software Wireshark. Upon compiling, place in your ./Plugins/ directory and create a curldump.ini file in the program's root directory which contains the offsets for Curl_setopt and Curl_close for your program. Upon starting your program, the plugin will start writing to the ACP file.

Example curldump.ini:
```
[CURL]
SetOpt=55 8B EC 83 EC 0C 8B 45 0C 53 33 D2 56 57 89 55 FC 3D 11 27 00 00
Close=55 8B EC 56 8B 75 08 57 33 FF 3B F7 0F 84 ?? ?? ?? ?? 57 56 E8
HookDelay=10000
```