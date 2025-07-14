# Hailo Detection C++

Este proyecto implementa detección de objetos en tiempo real usando GStreamer, OpenCV y el módulo Hailo en una Raspberry Pi.

## Requisitos

- GStreamer 1.0
- OpenCV 4
- Hailo SDK
- CMake >= 3.14

## Ejecucion en ../build

./hailo_detection --vehicles --usb

## Compilación

```bash
mkdir build && cd build
cmake ..
make
