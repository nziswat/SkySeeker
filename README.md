# SkySeeker

Goes without saying that this is the SkySeeker project.

HOW TO COMPILE:
Keen-eyed browsers of this repo will notice that this doesn't actually contain all the required CEF files one would need, so the first thing to do is to download that.
Specifically, this project is built (as of 2/28/2025) with the compiled windows CEF binaries, the standard distribution, version 134.1.2+gb3fe8ad
You can download that here: https://cef-builds.spotifycdn.com/cef_binary_133.4.2%2Bg0852ba6%2Bchromium-133.0.6943.127_windows64.tar.bz2

Once you've downloaded the files, all you need to do is extract the contents of the archive over wherever you have this repo downloaded. DO NOT REPLACE ANY FILES

If you're an advanced user (NDR) then I'm sure you can figure out the rest, if not;
Install Visual Studio, not visual studio code. Make sure you also install the Cmake addon alongside your VS.
In Visual Studio, simply open SkySeeker as a folder and VS /should/ automatically know how to build the files from there
Select whatever configuration you wish (probably just x64-Release) and build cefsimple.exe
The build target will probably chance closer to release.