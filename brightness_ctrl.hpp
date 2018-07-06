#ifndef BRIGHTNESS_CTRL_HPP
#define BRIGHTNESS_CTRL_HPP
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <thread>
#include <mutex>
#include <vector>
#include "sensor_als.hpp"
class BrightnessControl
{
private:
	filesystem::path cpath,brpath,maxbrpath;
	std::vector<int> thresh,value;
	int delay,direction,br,maxbr,minabr,tr,offset;
	size_t cur;
	SensorALS *als;
	std::mutex interrupt_m,threshnotify_m,adjust_m;
	std::condition_variable interrupt,threshnotify;
	void _brightness_slide(int p);
public:
	void init(float initv,SensorALS *s);
	void set_path(filesystem::path p);
	void set_thresh(std::vector<int> _th);
	void set_value(std::vector<int> _v);
	void set_delay(int _d);
	void set_trigrange(int _tr);
	void set_minabr(int _mbr);
	
	void set_offset(int rel,int off);

	void force_adjust();
	void on_sensor_report(float v);
	void brightness_slide(int p);
	void worker();
};
#endif
