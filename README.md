# SkySeeker

Goes without saying that this is the SkySeeker project.

HOW TO COMPILE:
Keen-eyed browsers of this repo will notice that this doesn't actually contain all the required CEF files one would need, so the first thing to do is to download that.
Specifically, this project is built (as of 2/28/2025) with the compiled windows CEF binaries, the standard distribution, version 134.1.2+gb3fe8ad
You can download that here: https://cef-builds.spotifycdn.com/cef_binary_133.4.2%2Bg0852ba6%2Bchromium-133.0.6943.127_windows64.tar.bz2

Once you've downloaded the files, you only need to extract include, libcef_dll, Debug and resources. DO NOT OVERWRITE ANY FILES (though you should not be prompted to anyway)

If you're an advanced user (NDR) then I'm sure you can figure out the rest, if not;
Install Visual Studio, not visual studio code. Make sure you also install the Cmake addon alongside your VS.
In Visual Studio, simply open SkySeeker as a folder and VS /should/ automatically know how to build the files from there
Select whatever configuration you wish (probably just x64-Release) and build SkySeeker.exe
the output can be found in 'out\build\x64-Release\src\Release\'

Frontend coders:
You should place your files in 'src/html'. Cmake will copy the HTML files and RTLSDR.dll on build, and running it in debug mode will copy the HTML files everytime on start up
If for some reason this doesn't work, you can always just manually copy the files.
