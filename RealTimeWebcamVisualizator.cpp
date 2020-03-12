#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/v4l2-common.h>
#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <fstream>
#include <string>
#include <chrono>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

using namespace std;

int main() {
 // 1. Open the device
 int fd; // A file descriptor to the video device
 fd = open("/dev/video0",O_RDWR);
 if(fd < 0){
 perror("Failed to open device, OPEN");
 return 1;
 }


 // 2. Ask the device if it can capture frames
 v4l2_capability capability;
 if(ioctl(fd, VIDIOC_QUERYCAP, &capability) < 0){
 // something went wrong... exit
 perror("Failed to get device capabilities, VIDIOC_QUERYCAP");
 return 1;
 }
 
//Ask the device if it can capture single-planar video capture
if(!(capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)){
    fprintf(stderr, "The device does not handle single-planar video capture.\n");
    exit(1);
}

//Ask the device if it can capture single-planar video capture
if(!(capability.capabilities & V4L2_CAP_STREAMING)){
    fprintf(stderr, "The device does not handle frame streaming.\n");
    exit(1);
}


 // 3. Set Image format
 v4l2_format imageFormat;
 imageFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 imageFormat.fmt.pix.width = 1920;
 imageFormat.fmt.pix.height = 1080;
 imageFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
 imageFormat.fmt.pix.field = V4L2_FIELD_NONE;
 // tell the device you are using this format
 if(ioctl(fd, VIDIOC_S_FMT, &imageFormat) < 0){
 perror("Device could not set format, VIDIOC_S_FMT");
 return 1;
 }


 // 4. Request Buffers from the device
 v4l2_requestbuffers requestBuffer = {0};
 requestBuffer.count = 1; // one request buffer
 requestBuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; // request a buffer wich we can use for capturing frames
 requestBuffer.memory = V4L2_MEMORY_MMAP;

 if(ioctl(fd, VIDIOC_REQBUFS, &requestBuffer) < 0){
 perror("Could not request buffer from device, VIDIOC_REQBUFS");
 return 1;
 }


 // 6. We need to ask it about the amount of memory it needs, and allocate it. Basically, 
 //the device is making the calculation I made above, and telling you how many bytes it need 
 //for your format and your frame dimensions. This information is retrieved using the 
 //VIDIOC_QUERYBUF call, and its v4l2_buffer structure.
 v4l2_buffer bufferinfo;
 memset(&bufferinfo, 0, sizeof(bufferinfo));
 bufferinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 bufferinfo.memory = V4L2_MEMORY_MMAP;
 bufferinfo.index = 0;
 
 if(ioctl(fd, VIDIOC_QUERYBUF, &bufferinfo) < 0){
    perror("Device did not return the buffer information, VIDIOC_QUERYBUF");
    exit(1);
}
 

//Mapping our memory
void* buffer_start = mmap(
    NULL,
    bufferinfo.length,
    PROT_READ | PROT_WRITE,
    MAP_SHARED,
    fd,
    bufferinfo.m.offset
);



 // use a pointer to point to the newly created buffer
 // mmap() will map the memory address of the device to
 // an address in memory
if(buffer_start == MAP_FAILED){
    perror("mmap");
    exit(1);
}

memset(buffer_start, 0, bufferinfo.length);


 // Activate streaming
 int type = bufferinfo.type;
 if(ioctl(fd, VIDIOC_STREAMON, &type) < 0){
 perror("Could not start streaming, VIDIOC_STREAMON");
 return 1;
 }


// Initialise everything visualization related
SDL_Init(SDL_INIT_VIDEO);
IMG_Init(IMG_INIT_JPG);

/* Here is where you typically start two loops:
 * - One which runs for as long as you want to capture frames (shoot the video).
 * - One which iterates over your buffers everytime (in case of multiple buffering). */
 
int framesCounterTotal = 0;
int framesCounterPerSecond = 0;

std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

while(framesCounterTotal < 9000){
// Put the buffer in the incoming queue.
if(ioctl(fd, VIDIOC_QBUF, &bufferinfo) < 0){
    perror("Could not queue buffer, VIDIOC_QBUF");
    exit(1);
}
 
// The buffer's waiting in the outgoing queue.
if(ioctl(fd, VIDIOC_DQBUF, &bufferinfo) < 0){
    perror("Could not dequeue the buffer, VIDIOC_DQBUF");
    exit(1);
}

//Visualization in the loop
// Get the screen's surface.
SDL_Surface* screen = SDL_SetVideoMode(
    imageFormat.fmt.pix.width,
    imageFormat.fmt.pix.height,
    32, SDL_HWSURFACE
);
 
SDL_RWops* buffer_stream;
SDL_Surface* frame;
SDL_Rect position = {.x = 0, .y = 0};
 
// Create a stream based on our buffer.
buffer_stream = SDL_RWFromMem(buffer_start, bufferinfo.length);
 
// Create a surface using the data coming out of the above stream.
frame = IMG_Load_RW(buffer_stream, 0);
 
// Blit the surface and flip the screen.
SDL_BlitSurface(frame, NULL, screen, &position);
SDL_Flip(screen);
 
// Free everything, and unload SDL & Co.
SDL_FreeSurface(frame);
SDL_RWclose(buffer_stream);


framesCounterTotal++;

framesCounterPerSecond++;

std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

if(chrono::duration_cast<chrono::microseconds>(end - start).count()>1000000){
	start = std::chrono::steady_clock::now();
	
	cout<< "Frames per second: " << framesCounterPerSecond << endl;
	cout<< "Total frames: " << framesCounterTotal << endl;

	framesCounterPerSecond = 0;
}	

}
 
/* Your loops end here. */
 
// Deactivate streaming
if(ioctl(fd, VIDIOC_STREAMOFF, &type) < 0){
    perror("Could not end streaming, VIDIOC_STREAMOFF");
    exit(1);
}

close(fd);
 

IMG_Quit();
SDL_Quit();

 return 0;
}
