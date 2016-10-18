CurlDump Ayria Extension
---

This plugin allows for CURL dumping to an .acp file which can be opened by the software Wireshark. Upon compiling, place in your ./Plugins/ directory and create a curldump.ini file in the program's root directory which contains the offsets for Curl_setopt and Curl_close for your program. Upon starting your program, the plugin will start writing to the ACP file.

Example curldump.ini:
```
[CURL]
SetOpt=0x0138F740
Close=0x00000000
HookDelay=10000
```