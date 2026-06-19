# Firmware QC QCC744M-EVK
Firmware for the [QC QCC744M-EVK](https://docs.qualcomm.com/bundle/publicresource/topics/80-WL740-250/landingpage.html) board

## Initialize This Repo

```bash
west init https://github.com/edgeimpulse/firmware-qc-qcc744m-evk.git
cd firmware-qc-qcc744m-evk
west update
```

This fetches:
- Zephyr RTOS
- Edge Impulse Zephyr SDK module
- All module dependencies

## Update the Model

In Edge Impulse Studio go to:  
**Deployment** > **Zephyr library** > **Build**

Download the generated `.zip`

Extract into the `model/` folder:

```bash
unzip -o ~/Downloads/your-model.zip -d model/
```

Your `model/` directory should contain:
- `CMakeLists.txt`
- `edge-impulse-sdk/`
- `model-parameters/`
- `tflite-model/`

## Supported shield

--shield arduino_modulino_movement

### Arduino modulino movement
[Arduino Modulino movement](thttps://docs.zephyrproject.org/latest/boards/shields/arduino_modulino_movement/doc/index.html)

How to connect:
Modulino <->QCC EVK
GND <-> GND
3V3 <-> VDD_3V3
SDA <-> GPIO23
SCL <-> GPIO24

## Build

```bash
west build --pristine -b qcc744m_evk
```

```

## Flash

```bash
west flash
```

## Sensors Supported
Right now only [arduino modulino movement](https://docs.zephyrproject.org/latest/boards/shields/arduino_modulino_movement/doc/index.html) is supported.

## Project Structure

```
├── CMakeLists.txt          # Build configuration
├── prj.conf                # Zephyr config
├── west.yml                # Manifest (declares Edge Impulse SDK module)
├── model/                  # Your Edge Impulse model (Zephyr library)
└── src/
    ├── main.cpp
    ├── inference/          # Inference state machine
    └── sensors/            # IMU interface
```

## How It Works

1. **Initialize** - Sensor setup via Zephyr sensor API
2. **Sample** - Continuous data collection at model frequency
3. **Buffer** - Circular buffer stores samples
4. **Infer** - Run classifier when buffer full
5. **Output** - Print classification results
6. **Loop** - Repeat

## Configuration

Key settings in `prj.conf`:

```properties
CONFIG_EDGE_IMPULSE_SDK=y        # Enable Edge Impulse SDK
CONFIG_MAIN_STACK_SIZE=2048      # Adjust for your model size
CONFIG_SENSOR=y                  # Enable sensor subsystem
CONFIG_I2C=y                     # Enable I2C for sensors
```

For larger models, increase stack size:

```properties
CONFIG_MAIN_STACK_SIZE=16384
```

## Using in Your Own Project

Add to your `west.yml`:

```yaml
projects:
  - name: edge-impulse-sdk-zephyr
    path: modules/edge-impulse-sdk-zephyr
    revision: v1.93.7  # See https://github.com/edgeimpulse/edge-impulse-sdk-zephyr/tags
    url: https://github.com/edgeimpulse/edge-impulse-sdk-zephyr
```

Then:

```bash
west update
```

Add to your `CMakeLists.txt`:

```cmake
list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/model)
```

## Troubleshooting

### **Module not found**
```bash
west update
```

### **Insufficient memory**
```properties
# In prj.conf
CONFIG_MAIN_STACK_SIZE=16384
```

## Resources
- [Edge Impulse SDK for Zephyr](https://github.com/edgeimpulse/edge-impulse-sdk-zephyr)

