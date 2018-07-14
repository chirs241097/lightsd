//Chris Xiong 2018
//3-Clause BSD License
#include <cstdio>
#include "sensor_als.hpp"
void SensorALS::set_debug(bool d)
{
	debug=d;
}
void SensorALS::enable_scan_elements()
{
	enable_scan_element("both");
}
void SensorALS::update_values()
{
	value=std::any_cast<int>(dict["in_intensity_both_value"])*
	std::any_cast<float>(dict["in_intensity_scale"]);
	if(debug)printf("ALS reading: %.2f\n",value);
}
float SensorALS::get_value(){return value;}
