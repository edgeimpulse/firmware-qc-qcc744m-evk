# Firmware QC QCC744M-EVK
Firmware for the [QC QCC744M-EVK](https://docs.qualcomm.com/bundle/publicresource/topics/80-WL740-250/landingpage.html) board.

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

Your `model/` directory should contain:
- `CMakeLists.txt`
- `edge-impulse-sdk/`
- `model-parameters/`
- `tflite-model/`

## Run inference and ingestion

### Inference
To run ingestion, connect to serial and send
- AT+RUNIMPULSE       # run impulse
- AT+RUNIMPULSECONT   # run impulse in continuos mode
- AT+RUNIMPULSEDEBUG  # run impulse with debug print

You can also use the [edge-impulse-run-impulse](https://docs.edgeimpulse.com/tools/clis/edge-impulse-cli/impulse-runner) cli tool.

### Ingestion
For the IMU, a stream of samples is printed on the serial when the application is in data forwarder mode, with a fixed frequency of CONFIG_SAMPLING_FREQUENCY_HZ.
To enter in data forwarder mode, send AT+STARTDATAFWD.
To exit data forwader mode, send AT+STOPDATAFWD.

When in data formwarder mode, you can use [edge-impulse-data-forwarder](https://docs.edgeimpulse.com/tools/clis/edge-impulse-cli/data-forwarder) to connect to any of your project and directly upload data.

## Supported shield

### Arduino modulino movement
[Arduino Modulino movement](thttps://docs.zephyrproject.org/latest/boards/shields/arduino_modulino_movement/doc/index.html)

How to connect:
Modulino <->QCC EVK
GND <-> GND
3V3 <-> VDD_3V3
SDA <-> GPIO23
SCL <-> GPIO24

## Build

For a clean build:
```bash
west build --pristine -b qcc744m_evk
```

For incremental build, -p or --pristine can be omitted.
```bash
west build -b qcc744m_evk
```

## Flash
Refer to [Programming and Debugging](/https://docs.zephyrproject.org/latest/boards/qcom/qcc744m_evk/doc/index.html#programming-and-debugging) on how to flash and debug the board.


## Sensors Supported
Right now only [arduino modulino movement](https://docs.zephyrproject.org/latest/boards/shields/arduino_modulino_movement/doc/index.html) is supported.

## Project Structure

```
├── CMakeLists.txt          # Build configuration
├── prj.conf                # Zephyr config
├── west.yml                # Manifest (declares Edge Impulse SDK module)
├── model/                  # Your Edge Impulse model (Zephyr module)
└── src/
    ├── main.cpp            #
    ├── ei_loop.cpp         #
    ├── data-fw/            # Ingestion
    ├── inference/          # Inference state machine
    ├── msg_handler/        # Incoming messages
    └── sensors/            # IMU and mic interface
```

## Configuration

Key settings in `prj.conf`:

```properties
CONFIG_EDGE_IMPULSE_SDK=y        # Enable Edge Impulse SDK
CONFIG_EDGE_IMPULSE_HEAP_TYPE    # 0: malloc/free, 1: k_malloc/k_free
CONFIG_HEAP_MEM_POOL_SIZE=2048   # Adjust for your model size
CONFIG_SENSOR=y                  # Enable sensor subsystem
CONFIG_I2C=y                     # Enable I2C for sensors
```

For larger models, increase heap size:

```properties
CONFIG_MCONFIG_HEAP_MEM_POOL_SIZEIN_STACK_SIZE=16384
```

## Resources
- [Edge Impulse SDK for Zephyr](https://github.com/edgeimpulse/edge-impulse-sdk-zephyr)
