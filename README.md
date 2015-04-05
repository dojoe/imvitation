# Imvitation

a Demo by Wursthupe, Digital Demolition Krew and Brain Control.

# Source material

This is all the source material we used to build the demo. All our own sources are in the root directory, as are the VS2013 project and Linux/OSX Makefiles. The data subdirectory has the original demo data folder. The dsrc directory has some source data files we derived the release data from.

# License

The demo source code and assets are released under the terms of the WTFPL (http://www.wtfpl.net/), just do whatever the fuck you want to. If you happen to use any of these or found the source useful, a shoutout would make us happy though =)

# Building on Windows

You need some external libs that have to go into specific folders:
  GLEW and BASS go into the glew and bass subdirs. Just unpack.
  SOIL and GLFW3 go into the soil and glfw subdirs. You'll have to unpack the source and rebuild the .lib to use the static VC++ runtime, or change the demo to use the DLL runtime.
  AntTweakBar must be rebuilt as DLL from source, and the patch from https://sourceforge.net/p/anttweakbar/tickets/11/#5a6e must be included so it has GLFW3 bindings.
  
# Building on Linux

Linux is not our dev platform, so we never tried using AntTweakBar there. YMMV. Apart from that, intall libglfw3-dev, libsoil-dev, libglew-dev, and unpack the BASS Linux release, then a simple make should get you a demo.
