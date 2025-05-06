# SkySeeker

## Introduction
SkySeeker is a University of Tampa CS capstone project from Spring 2025. The main focus of the project was to create a program that would take signal from an ADS-B reciever and decode the data and display live
information on any transmitting aircraft in the local area. You may also choose to collect any of these aircraft, which will be uniquely displayed if you see them again in the future.

## Requirements
If you wish to try SkySeeker for yourself, you must first have an ADS-B antenna. You also must first download [Zadig](https://zadig.akeo.ie/) and install the libusb generic driver.
<br>You can download the current release to skip compiling from source code, at which point you can extract the program into a folder of your choice and enjoy catching some flights in SkySeeker!

## Compilation
Keen-eyed browsers of this repo will notice that this doesn't actually contain all the required CEF files one would need to compile, so the first thing to do is to download that.
<br>Specifically, this project is built (as of 2/28/2025) with the compiled windows CEF binaries, the standard distribution.
You can download that [here](https://cef-builds.spotifycdn.com/cef_binary_133.4.2%2Bg0852ba6%2Bchromium-133.0.6943.127_windows64.tar.bz2)
<br>Once you've downloaded the files, you only need to extract include, libcef_dll, Debug and resources. DO NOT OVERWRITE ANY FILES (though you should not be prompted to anyway)
At this point you may compile as you would normally using CMake. If you're not sure how to do this:
Install Visual Studio, not visual studio code. Make sure you also install the CMake addon alongside your VS.
In Visual Studio, simply open SkySeeker as a folder and VS /should/ automatically know how to build the files from there
Select whatever configuration you wish (probably just x64-Release) and build SkySeeker.exe.
The output can be found in 'out\build\x64-Release\src\Release\'

## Credits
This project was developed with the help of existing open source code. I would like to thank in particular:
<br>The [Chromium Embedded Framework](https://github.com/chromiumembedded/cef) Team
<br>Salvatore Sanfilippo for [dump1090](https://github.com/antirez/dump1090)
<br>Niels Lohmann for his [C++ JSON library](https://github.com/nlohmann/json)
<br>D. Richard Hipp for [SQLite3](https://www.sqlite.org/)

## Licensing
This Project is licensed under the 3-clause BSD license and all that it entails and implies.