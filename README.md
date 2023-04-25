![MbedOS Header Image](https://raw.githubusercontent.com/ARMmbed/mbed-os/master/logo.png)

# RoostaBoosta

Who doesn't need a good morning boost?
Return to your rich idyllic pastoral roots with RoostaBoosta™, the innovative, awesome, groundbreaking, fascinating, mind-boggling, stupendous IoT weather-predicting alarm clock!

Final project for Georgia Tech ECE4180.
Based on the newer [MbedOS 6](https://os.mbed.com/docs/mbed-os/v6.16/introduction/index.html) platform, this project incorporates networking, API calls, and audio-visual effects for an web-enabled alarm clock / weather prediction station.

## Building

Since the project is based on MbedOS 6, it strives to support the newer Mbed CLI 2, which among other things has [CMake support](https://os.mbed.com/docs/mbed-os/v6.16/build-tools/use.html#build-the-project-with-cmake-advanced).

In addition, this project also relies on [git submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules) for dependency management, since Mbed CLI tools for dependency management have been unreliable at best in the past.

### Requirements

- [Mbed CLI 2](https://os.mbed.com/docs/mbed-os/v6.16/build-tools/mbed-cli-2.html)
- [CMake](https://cmake.org/) >= v3.19
- [Supported Compiler](https://os.mbed.com/docs/mbed-os/v6.16/build-tools/index.html#:~:text=CLI%201%20page.-,compiler%20versions,-You%20can%20build)

### Process

Download the project files and all included dependencies:
```sh
git clone --recurse-submodules https://github.com/mshakula/RoostaBoosta.git
```

Alternatively, if the project is already checked out, do the following to update your dependencies:
```sh
git submodule update --init --recursive
```

To build, use standard CMake
```sh
cmake -S <project_dir> -B <output_directory>
cmake --build <output_directory> -t RoostaBoosta
```

### Online Compiler / Mbed Studio / Other compilers

We decided to use the Mbed CLI 2 / CMake as our main build tool since that seems to be the most actively supported solution for Mbed. Regardless, if another compilation solution is necessary, it may be possible to add add support with the following:

Create a symlink to `third_party/mbed-os` from the project root.
```sh
ln -s third_party/mbed-os mbed-os
```

Add a `mbed-os.lib` file that points to the checked out revision of mbed-os.
```sh
echo "https://github.com/ARMmbed/mbed-os.git#"$(cd third_party/mbed-os && git rev-parse HEAD) > mbed-os.lib
```

Note that this is not guaranteed to work if there are library conflicts and is not officially maintained.

## Branching 

All branches should be headed at `master`, which should be the stable build branch.

New features should be developed in parallel on `feature/[name]` branches.
Once a feature is complete, it can be merged into `master`.
Any features that depend on another should branch from the other one.
Features should only merge into their parent when they are certain that they work.

When merging back into `master` merge conflicts arise, they should be resolved in a local editor and tested before committing.
If any bug is retroactively found in `master`, a `bugfix/[name]` branch should be created and then merged back into it asap.

Master branch should have updated version numbers done in the top-level `CMakeLists.txt` file.
Feature branches can have the parent version number suffixed with the feature name.

## Style

Try to keep code legible and nice.
[Google C++ style guide](https://google.github.io/styleguide/cppguide.html) is good, with following exceptions:

- Use `.h` headers for C header files exclusively. Use `.hpp` for C++ headers.
- Avoid `/* */` style comments, prefer `//`.
- Use `/// \tag` style as opposed to `/** @tag` for doxygen parameter and function annotation.

A `.clang-format` is provided for all other formatting needs.
Please keep all checked-in code formatted by it.

## Hardware

The hardware used in this project is based on the hardware provided by the Georgia Tech ECE department.

Primary components used are:

- [mBed LPC1768](https://os.mbed.com/platforms/mbed-LPC1768/)
- [4D Systems uLCD-144-G2](https://www.sparkfun.com/products/11377)
- [HC-SRO4 Ultrasonic Sensor](https://www.sparkfun.com/products/15569)
- [Adafruit ESP8266 Breakout](https://www.adafruit.com/product/2471)
- [SparkFun microSD Breakout](https://www.sparkfun.com/products/544)
- [SparkFun RJ45 Ethernet Breakout](https://www.sparkfun.com/products/13021)
- [SparkFun TPA2005D1 Amp](https://www.sparkfun.com/products/11044)

Schematics and a breakout PCB are designed in [KiCAD](https://www.kicad.org/), which is free and open-source.

The hardware setup is currently as bare-bones as possible to simplify manual wiring.
