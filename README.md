# Hailo Detection C++

Este proyecto implementa detección de objetos en tiempo real usando GStreamer, OpenCV y el módulo Hailo en una Raspberry Pi.

## Requisitos

- GStreamer 1.0
- OpenCV 4
- Hailo SDK
- CMake >= 3.14

## Compilación

```bash
mkdir build && cd build
cmake ..
make


## Ejecucion
```bash
./hailo_detection --vehicles --usb
