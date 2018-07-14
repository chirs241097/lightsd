//Chris Xiong 2018
//3-Clause BSD License
#ifndef SENSOR_ALS_HPP
#define SENSOR_ALS_HPP
#include "sensors.hpp"
class SensorALS:public SensorBase
{
	private:
		float value;
		bool debug=false;
	protected:
		void enable_scan_elements();
	public:
		void set_debug(bool d);
		void update_values();
		float get_value();
};
#endif
