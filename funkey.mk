CXX = /opt/funkey-sdk/usr/bin/arm-linux-g++
PREFIX = /opt/funkey-sdk/arm-funkey-linux-musleabihf/sysroot/usr
SDLCONFIG = $(PREFIX)/bin/sdl-config
CXXFLAGS += -march=armv7-a+neon-vfpv4 -mtune=cortex-a7 -mfpu=neon-vfpv4 -DFUNKEY=1