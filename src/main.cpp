/// \file main.cpp
/// \date 2023-04-11
/// \author mshakula (matvey@gatech.edu)
///
/// \brief The main entrypoint for the program.

#include "pinout.hpp"

#include <mbed.h>

// ======================= Local Definitions =========================

namespace {

} // namespace

// ====================== Global Definitions =========================

int
main()
{
  DigitalOut led(LED1);

  while (true) {
    led = !led;
    ThisThread::sleep_for(500ms);
  }
}
