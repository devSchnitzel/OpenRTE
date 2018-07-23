# OpenRTE - Playstation 4 Memory/Module Manager (For 1.76 ONLY)
DEPRECATED - You are on 5.05 ? see the [PS4Debug](https://github.com/xemio/ps4debug/) by xemio

[![N|Solid](http://i.imgur.com/22NR4YM.png)](#)

OpenRTE is a little software for explore/edit memory of the Playstation 4.

**Download** pre-compiled version [here](https://github.com/theorywrong/OpenRTE/releases/).

# Executing
You need [NodeJS](https://nodejs.org/en/)  and [Elf-loader](https://github.com/ps4dev/elf-loader)

Tips: Add random page to signet and replace this url by http://ipofserver:5350/ (the backslash at the end is very important)

# Compiling
You need [PS4SDK](https://github.com/ps4dev/ps4sdk) and a magic command
```
make
```

# Issue

***sched.h related error :*** Simply download sched.h from [here](https://ghostbin.com/paste/d78ms
) and add this to you're include path.

# Debugging
OpenRTE use libdebugnet from [ps4link](https://github.com/psxdev/ps4link) by psxdev, the debug port is 15000 (change the ip of your computer on the binary, or in the source)
```
socat udp-recv:15000 stdout
```

# Credit
Thanks to:
[Dev_ShoOTz](https://realitygaming.fr/members/dev_shootz.412/) for all night of debugging :D

MarentDev for some help and the library for iOS (not here for the moment)

Marbella, MsKx, JimmyModding, AZN for testing

BadChoiceZ for the notify function

psxdev for libdebugnet

Quentin from [PLS Squad](https://discord.gg/5zPDW5) for some help in C/C++ with my crappy code xP

CTurt, Zecoxao, wskeu, wildcard, Z80 and all people contribuing to PS4 Scene


***Note: A release version is also available***

I have a bad english, sorry for that ^^'
Have fun :-)
