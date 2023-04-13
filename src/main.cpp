/// \file main.cpp
/// \date 2023-04-11
/// \author mshakula (matvey@gatech.edu)
///
/// \brief The main entrypoint for the program.

#include <mbed.h>
#include <4DGL-uLCD-144-MBedOS6/uLCD_4DGL.hpp>

// ======================= Local Definitions =========================

namespace {

} // namespace

// ====================== Global Definitions =========================

uLCD_4DGL uLCD(p28,p27,p30); // serial tx, serial rx, reset pin;

int
main()
{
  DigitalOut led(LED1);

  while (true) {
    led = !led;
    uLCD.text_string("working", 2, 4, FONT_7X8, GREEN);
    ThisThread::sleep_for(500ms);
    led = !led;
    uLCD.cls();
    ThisThread::sleep_for(500ms);
  }
}
