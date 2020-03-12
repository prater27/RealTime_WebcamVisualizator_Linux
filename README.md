# RealTime_WebcamVisualizator_Linux
A simple code for buffering and streaming a webcam using v4l2(Linux). It uses SDL 1.2 for depicting the processed frames. One single buffer.

Requires sdl 1.2 and sdl 1.2 image. In order to install this library completely  (using apt) run:
apt-cache search ^libsdl | grep 1.2
and then sudo apt install with each of the elements listed. So something like:
sudo apt install libsdl-gfx1.2-5 libsdl-gfx1.2-dev libsdl-image1.2 libsdl-image1.2-dev libsdl-mixer1.2 libsdl-mixer1.2-dev libsdl-net1.2 libsdl-net1.2-dev libsdl-ttf2.0-0 libsdl-ttf2.0-dev libsdl1.2-dev libsdl1.2debian libsdl1.2debian-dbgsym

For compiling do:
g++ -c RealTimeWebcamVisualizator.cpp `sdl-config --cflags --libs`
and then
g++ RealTimeWebcamVisualizator.o -o webcam-app -lSDL -lSDL_image
