# AudioSystems

## setup

For this you will need [vcpkg](https://vcpkg.io/en/getting-started.html). It should be in
the following path **C:/dev/vcpkg**, you can also check the cmake for it. It's a cmake
project used with visual studio, using clang and a presets file for debug and release
configuration.

### packages
vcpkg install
- ffmpeg:x64-windows
- ffmpeg[avcodec]:x64-windows --recurse 
- ffmpeg[avformat]:x64-windows --recurse 
- ffmpeg[avutil]:x64-windows --recurse 
- sdl2:x64-windows

