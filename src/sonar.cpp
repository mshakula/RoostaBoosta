/// \file sonar.cpp
/// \date 2023-04-28
/// \author jcurry39 (jake.curry@ymail.com)
///
/// \referenced Jim Hamblin's https://os.mbed.com/users/4180_1/notebook/using-the-hc-sr04-sonar-sensor/
/// \brief Functions to control the sonar.

#include "sonar.hpp"

#include <mbed.h>
#include <chrono>
#include "pinout.hpp"


// ======================= Local Definitions =========================

namespace {


DigitalOut trigger(rb::pinout::kSonar_trig);
DigitalIn  echo(rb::pinout::kSonar_echo);
//Timer sonar;

} // namespace

// ====================== Global Definitions =========================

//returns true if something is 10cm or closer to sonar
bool
Is_Snoozed()
{
	// sonar.reset();

	// sonar.start();
	// // min software polling delay to read echo pin
	// while (echo==2) {};
	// sonar.stop();
	// int correction = sonar.elapsed_time().count();
	// debug("\r\n\t[Sonar] Approximate software overhead timer delay is %d uS",correction);

	// // trigger sonar to send a ping
	// trigger = 1;
 //    sonar.reset();
 //    ThisThread::sleep_for(5ms);
 //    trigger = 0;
 //    //wait for echo high
 //    while (echo==0) {};
 //    //echo high, so start timer
 //    sonar.start();
 //    //wait for echo low
 //    while (echo==1) {};
 //    //stop timer and read value
 //    sonar.stop();
 //    //subtract software overhead timer delay and scale to cm
 //    int distance = (sonar.elapsed_time().count() - correction) / 58.0;
 //    debug("\r\n\t[Sonar] distance:  %d cm",distance);

 //    //wait a little bit
 //    ThisThread::sleep_for(200ms);

 //    //check distance
 //    if(distance <= 10) {
 //    	return true;
 //    }
    return false;
}