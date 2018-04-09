#include "sensor_als.hpp"
void SensorALS::enable_scan_elements()
{
	enable_scan_element("both");
}
void SensorALS::update_values()
{
	value=std::any_cast<int>(dict["in_intensity_both_value"])*
	std::any_cast<float>(dict["in_intensity_scale"]);
}
float SensorALS::get_value(){return value;}
