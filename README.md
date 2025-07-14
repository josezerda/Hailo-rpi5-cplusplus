# Hailo Detection C++

Este proyecto implementa detección de objetos en tiempo real usando GStreamer, OpenCV y el módulo Hailo en una Raspberry Pi.

## Requisitos

- GStreamer 1.0
- OpenCV 4
- Hailo SDK
- CMake >= 3.14
## Prueba desde consola para asegurar que el Pipeline este correcto (Donde $ es el prompt)

$ gst-launch-1.0 v4l2src device=/dev/video0 !     video/x-raw,format=YUY2,width=640,height=480,framerate=30/1 !     videoconvert ! videoscale !     video/x-raw,format=RGB,width=640,height=640 !     hailonet hef-path=/usr/share/hailo-models/yolov8s_h8l.hef !     hailofilter so-path=/usr/lib/aarch64-linux-gnu/hailo/tappas/post_processes/libyolo_hailortpp_post.so !     hailooverlay !     videoconvert !     waylandsink

## Ejecucion en ../build

./hailo_detection --vehicles --usb

## Compilación

```bash
mkdir build && cd build
cmake ..
make
