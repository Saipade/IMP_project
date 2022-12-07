# RGB LED Controller

## Installation

Since Arduino framework is used as a component, we need to download it to `./components` folder

```
mkdir -p components && \
cd components && \
git clone https://github.com/espressif/arduino-esp32.git arduino && \
cd arduino && \
git submodule update --init --recursive && \
cd ../.. && \
idf.py menuconfig
```
