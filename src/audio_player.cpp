/// \file audio_player.cpp
/// \date 2023-04-22
/// \author jcurry39 (jake.curry@ymail.com)
///
/// \brief fuctions to control the speaker
#include <mbed.h>
#include <stdio.h>
#include <string.h>
#include "MusicPlayer.h"
#include "audio_player.hpp"
#include "weather_data.hpp"

//global to set file prefix
std::string PREFIX = "/sd/sounds/";

//plays audio file from file passed in
//param filename the filename past the root to plat
void
play_file(const char *filename)
{
	std::string fname = filename;
	playMusic((PREFIX + fname).c_str(), 1.0);
}


//reads a numbner 10-19
//separate function for clutter
void
play_teens(int number)
{	
	switch(number)
	{
		case 10:
			play_file("numbers/ten.pcm");
			break;
		case 11:
			play_file("numbers/eleven.pcm");
			break;
		case 12:
			play_file("numbers/twelve.pcm");
			break;
		case 13:
			play_file("numbers/thirteen.pcm");
			break;
		case 14:
			play_file("numbers/fourteen.pcm");
			break;
		case 15:
			play_file("numbers/fifteen.pcm");
			break;
		case 16:
			play_file("numbers/sixteen.pcm");
			break;
		case 17:
			play_file("numbers/seventeen.pcm");
			break;
		case 18:
			play_file("numbers/eighteen.pcm");
			break;
		case 19:
			play_file("numbers/nineteen.pcm");
			break;
		//if not a teen, move on
		default:
			break;
	}
}


//reads any number, used for multiple values
void 
read_number(int number)
{
	//check for negative
	if(number < 0) 
	{
		play_file("numbers/negative.pcm");
		//make positive
		number = number * -1;
	}
	int tens = number / 10;
	int ones =  number % 10;
	//read out the 10s place first
	switch(tens)
	{
		case 1:
			//the teens case has more complicated playing
			play_teens(number);
			//dont read ones place
			return;
		case 2:
			play_file("numbers/twenty.pcm");
			break;
		case 3:
			play_file("numbers/thirty.pcm");
			break;
		case 4:
			play_file("numbers/forty.pcm");
			break;
		case 5:
			play_file("numbers/fifty.pcm");
			break;
		case 6:
			play_file("numbers/sixty.pcm");
			break;
		case 7:
			play_file("numbers/seventy.pcm");
			break;
		case 8:
			play_file("numbers/eighty.pcm");
			break;
		case 9:
			play_file("numbers/ninety.pcm");
			break;
		case 10:
			play_file("numbers/hundred.pcm");
			//one hundred
			break;
		case 11:
			//one hundred and tens
			play_file("numbers/hundred.pcm");
			play_teens(number - 100);
			//dont play ones place
			return;
		default:
			//if less than 10, read nothing
			break;
	}
	//read ones place after
	switch(ones)
	{
		case 1:
			play_file("numbers/one.pcm");
			break;
		case 2:
			play_file("numbers/two.pcm");
			break;
		case 3:
			play_file("numbers/three.pcm");
			break;
		case 4:
			play_file("numbers/four.pcm");
			break;
		case 5:
			play_file("numbers/five.pcm");
			break;
		case 6:
			play_file("numbers/six.pcm");
			break;
		case 7:
			play_file("numbers/seven.pcm");
			break;
		case 8:
			play_file("numbers/eight.pcm");
			break;
		case 9:
			play_file("numbers/nine.pcm");
			break;
		default:
			//read 0
			play_file("numbers/zero.pcm");
			break;
	}
}

//play an alarm sound
//separate function for usability
// void
// play_alarm()
// {
// 	play_file("alarm.pcm");
// }

//reads all weather data 
//param data the current weather conditons
void 
play_audio(weather_data* data)
{
	//read temperature
	play_file("temperature/itis.pcm");
	read_number(data->temperature);
	play_file("temperature/doutside.pcm");

	//read precipitation chance
	play_file("precipitation/there_is.pcm");
	read_number(data->precipitation_chance);
	play_file("precipitation/percent_chance.pcm");

	//read wind speed 
	play_file("wind/wind_speed.pcm");
	read_number(data->wind_speed);
	play_file("wind/mph.pcm");

	//read humidity
	play_file("humidity/humidity.pcm");
	read_number(data->humidity);
	play_file("humidity/percent.pcm");

	//read weather
	//only one allowed, if any condition is satified, return
	//if no sutable phrase, say nothing
	if(strstr(data->weather, "thunder"))
	{
		play_file("weather/weather.pcm");
		play_file("weather/thunderstorms.pcm");
		return;
	}

	if(strstr(data->weather, "rain"))
	{
		play_file("weather/weather.pcm");
		play_file("weather/raining.pcm");
		return;
	}

	if(strstr(data->weather, "snow"))
	{
		play_file("weather/weather.pcm");
		play_file("weather/snowing.pcm");
		return;
	}

	if(strstr(data->weather, "partly"))
	{
		play_file("weather/weather.pcm");
		play_file("weather/partly_cloudy.pcm");
		return;
	}

	if(strstr(data->weather, "cloud"))
	{
		play_file("weather/weather.pcm");
		play_file("weather/cloudy.pcm");
		return;
	}

	if(strstr(data->weather, "sun"))
	{
		play_file("weather/weather.pcm");
		play_file("weather/sunny.pcm");
		return;
	}
}