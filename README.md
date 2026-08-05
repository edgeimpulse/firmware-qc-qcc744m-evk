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
[Arduino Modulino movement](https://docs.zephyrproject.org/latest/boards/shields/arduino_modulino_movement/doc/index.html)
— LSM6DSO 6-axis IMU, I2C address `0x6a`.

How to connect:
```
Modulino <-> QCC EVK
GND      <-> GND
3V3      <-> VDD_3V3
SDA      <-> GPIO23
SCL      <-> GPIO24
```

### Arduino modulino pixels
[Arduino Modulino Pixels](https://docs.zephyrproject.org/latest/boards/shields/arduino_modulino_pixels/doc/index.html)
— 8 addressable RGB LEDs on the same I2C bus, address `0x36`. Wire it to the
same `GND / 3V3 / SDA (GPIO23) / SCL (GPIO24)` pins (daisy-chain the Qwiic
connectors). Enabled with `CONFIG_LED_STRIP=y` and driven through
`src/sensors/ei_pixels.{h,cpp}`:

```c
ei_pixels_init();                              // false if the board isn't found
ei_pixels_on(EI_PIXEL(0) | EI_PIXEL(2));       // LEDs addressed by bitmask
ei_pixels_set_color(EI_PIXELS_ALL, 0, 0x20, 0);// green on all
ei_pixels_toggle(EI_PIXEL(1));
ei_pixels_off(EI_PIXELS_ALL);                  // (or ei_pixels_clear())
```

LEDs are selected by a bitmask — `led0 = 1`, `led1 = 2`, `led2 = 4`, … — build
bits with `EI_PIXEL(i)` and OR them together.

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

A helper script `flash.sh` wraps the vendor `QConn_Flash_Cmd` tool with the
addresses from the [board docs](https://docs.zephyrproject.org/latest/boards/qcom/qcc744m_evk/doc/index.html#programming-and-debugging)
(boot2 @ `0x0`, `zephyr.bin` @ `0x2000`).

Put the board in **download mode** first: hold **BOOT**, tap **RESET**, release **BOOT**.

```bash
./flash.sh              # flash the app (zephyr.bin @ 0x2000) -- default
./flash.sh --boot2      # flash only boot2 (@ 0x0), needed once per fresh board
./flash.sh --all        # flash boot2 + app
./flash.sh --port /dev/ttyACM0 --baud 115200
```

`boot2` only needs flashing once per board (unless its flash region is erased).

### Serial console

The console/AT-command port is one of the two CH347 ACM interfaces (usually
`/dev/ttyACM1`).

## Debug

### VS Code

`.vscode/launch.json` + `.vscode/tasks.json` provide a one-key flow. Open this
firmware folder in VS Code, then press **F5** and pick a configuration:

- **QCC744M: Debug (OpenOCD + GDB)** — uses the Microsoft **C/C++** extension
  (`ms-vscode.cpptools`). Its `preLaunchTask` builds (`west build`) then starts
  the gdb-server (via `debug.sh --server`), attaches, halts, and breaks at
  `main`. OpenOCD is stopped automatically when the session ends.
- **QCC744M: cortex-debug (OpenOCD)** — alternative using the **Cortex-Debug**
  extension (`marus25.cortex-debug`), which manages OpenOCD itself.

Build tasks are also available standalone via **Terminal → Run Build Task**
(`west build --pristine` is the default; an incremental variant is included).

> Tool paths in these files are absolute (matching `flash.sh`/`debug.sh`). If
> your build output lives elsewhere, update `program`/`executable` to point at
> `build/zephyr/zephyr.elf`. Launch VS Code from the activated west venv (with
> `ZEPHYR_BASE` set) so the build tasks find `west`.

## Sensors Supported

The active sensor is chosen by the Edge Impulse model (`EI_CLASSIFIER_SENSOR`),
so which of these feeds inference depends on the model dropped into `model/`.

| Sensor        | Device                         | Bus / pins                     | Source                     |
| ------------- | ------------------------------ | ------------------------------ | -------------------------- |
| IMU (6-axis)  | LSM6DSO (Modulino Movement)    | I2C `0x6a`                     | `src/sensors/ei_inertial.*`|
| ADC           | on-chip ADC channel 0          | GPIO20 (ADC_CH0)               | `src/sensors/ei_adc.*`     |

Output/indicator (not a sensor): **Modulino Pixels** RGB LEDs via
`src/sensors/ei_pixels.*` (see [Supported shield](#arduino-modulino-pixels)).

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
    └── sensors/            # IMU, mic, ADC, and Modulino Pixels interfaces
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
