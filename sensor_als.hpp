#ifndef SENSOR_ALS_HPP
#define SENSOR_ALS_HPP
#include "sensors.hpp"
class SensorALS:public SensorBase
{
	private:
		float value;
	protected:
		void enable_scan_elements();
	public:
		void update_values();
		float get_value();
};
#endif
