//Structure for the weather data
//in its own file as is used by many files.
#ifndef WEATHER_DATA_HPP
#define WEATHER_DATA_HPP
struct weather_data
{
	int	humidity;
	int precipitation_chance;
	int temperature;
	int wind_speed;
	char *weather;	//static size arr to avoid extra pointers
};
#endif