#ifndef SENSORS_HPP
#define SENSORS_HPP
#include <cstdint>
#include <string>
#include <any>
#include <experimental/filesystem>
#include <functional>
#include <unordered_map>
#include <tuple>
#include <vector>

#define IIODEV_SYSFS_PATH_BASE "/sys/bus/iio/devices/iio:device"
#define DEV_PATH "/dev/iio:device"

namespace filesystem=std::experimental::filesystem::v1;

struct scan_t
{
	bool is_le;
	bool is_signed;
	uint8_t bits,storagebits;
	uint8_t shift,repeat;
};

class SensorBase
{
	private:
		int devfd;
		int workerquit;
		int readsize;
		std::function<void(SensorBase*)> readercb;
		std::vector<std::tuple<int,std::string,scan_t>> enabled_scan_elem;
		
		void enable_buffer();
		void parse_type_string(std::string type,scan_t* ti);
		void readbuffer();
	protected:
		std::string type,sensor_basename;
		filesystem::path devbufpath;
		filesystem::path sysfspath;
		std::unordered_map<std::string,std::any> dict;

		void enable_scan_element(std::string elem);
		
		virtual void enable_scan_elements()=0;
		virtual void update_values()=0;
	public:
		virtual ~SensorBase(){}
		void init(int id,std::string _sensor_basename);
		void deinit();
		void reset();
		void worker();
		void quit_worker();
		void set_reader_callback(std::function<void(SensorBase*)> cb);
		static std::string get_type(int id);
		static int detect_sensor(std::string type);
};
#endif
