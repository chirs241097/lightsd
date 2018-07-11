//Chris Xiong 2018
//3-Clause BSD License
#include <cstdio>
#include <cstring>
#include <variant>
#include <thread>
#include <sys/types.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include "utils.hpp"
#include "sensors.hpp"
void SensorBase::parse_type_string(std::string type,scan_t* ti)
{
	char endian[4],sign;
	if(type.find('X')!=type.npos)
		sscanf(type.c_str(),"%2s:%c%hhu/%hhuX%hhu>>%hhu",
			endian,&sign,&ti->bits,&ti->storagebits,&ti->repeat,&ti->shift
		);
	else
	{
		ti->repeat=0;
		sscanf(type.c_str(),"%2s:%c%hhu/%hhu>>%hhu",
			endian,&sign,&ti->bits,&ti->storagebits,&ti->shift
		);
	}
	ti->is_le=std::string(endian)=="le";
	ti->is_signed=sign=='s';
}
void SensorBase::readbuffer()
{
	pollfd p[2];
	p[0]=pollfd{devfd,POLLIN,0};
	p[1]=pollfd{qpipe[0],POLLIN,0};
	if(poll(p,2,-1)<=0)return;
	char* buf=new char[readsize];
	if(p[1].revents&POLLIN)
	{
		ignore_result(read(qpipe[0],buf,readsize));
		delete[] buf;
		return;
	}
	ssize_t sz=read(devfd,buf,readsize);
	if(sz==readsize&&!paused)
	{
		char *p=buf;
		for(int i=0;p-buf<readsize;++i)
		{
			scan_t ti=std::get<scan_t>(enabled_scan_elem[i]);
			std::string es=std::get<std::string>(enabled_scan_elem[i])+"_value";
			std::vector<char> pp(p,p+ti.storagebits/8);
			if(ti.is_le^PLATFORM_IS_LITTLEENDIAN)
			std::reverse(pp.begin(),pp.end());
			auto readint=[&](auto t){
				memcpy(&t,pp.data(),ti.storagebits/8);
				t>>=ti.shift;dict[es]=t;
			};
			switch(ti.storagebits)
			{
				case 8:
					if(ti.is_signed)readint(int8_t(0));
					else readint(uint8_t(0));
				break;
				case 16:
					if(ti.is_signed)readint(int16_t(0));
					else readint(uint16_t(0));
				break;
				case 32:
					if(ti.is_signed)readint(int32_t(0));
					else readint(uint32_t(0));
				break;
				case 64:
					if(ti.is_signed)readint(int64_t(0));
					else readint(uint64_t(0));
				break;
			}
			p+=ti.storagebits/8;
		}
	}
	delete[] buf;
}
void SensorBase::enable_buffer()
{
	using filesystem::path;
	
	path buffer_enable_path=sysfspath/"buffer"/"enable";
	writeint(buffer_enable_path.c_str(),1);
}
void SensorBase::enable_scan_element(std::string elem)
{
	using filesystem::path;
	std::string elem_base=sensor_basename+(elem.length()?"_"+elem:"");
	
	path elem_type_path=sysfspath/"scan_elements"/(elem_base+"_type");
	std::string ts;dict[elem_base+"_type"]=ts=readstr(elem_type_path.c_str());
	scan_t st;parse_type_string(ts,&st);
	readsize+=st.storagebits/8;//assume this is aligned to byte

	path elem_en_path=sysfspath/"scan_elements"/(elem_base+"_en");
	writeint(elem_en_path.c_str(),1);

	path elem_idx_path=sysfspath/"scan_elements"/(elem_base+"_index");
	int idx;dict[elem_base+"_index"]=idx=readint(elem_idx_path.c_str());
	
	path raw_val_path=sysfspath/(elem_base+"_raw");//initial value
	dict[elem_base+"_value"]=readint(raw_val_path.c_str());

	enabled_scan_elem.insert(
		std::upper_bound(enabled_scan_elem.begin(),enabled_scan_elem.end(),
			std::make_tuple(idx,elem_base,st),
			[](const auto&a,const auto&b){return std::get<0>(a)<std::get<0>(b);}
		),std::make_tuple(idx,elem_base,st)
	);
}
bool SensorBase::init(int id,std::string _sensor_basename)
{
	sysfspath=IIODEV_SYSFS_PATH_BASE+std::to_string(id);
	devbufpath=DEV_PATH+std::to_string(id);
	sensor_basename=_sensor_basename;

	using filesystem::path;
	path name_path=sysfspath/"name";
	type=readstr(name_path.c_str());
	path scale_path=sysfspath/(sensor_basename+"_scale");
	dict[sensor_basename+"_scale"]=readfloat(scale_path.c_str());
	path offset_path=sysfspath/(sensor_basename+"_offset");
	dict[sensor_basename+"_offset"]=readfloat(offset_path.c_str());

	reset();
	ignore_result(pipe(qpipe));
	enabled_scan_elem.clear();
	enable_scan_elements();
	update_values();
	enable_buffer();
	devfd=open(devbufpath.c_str(),O_RDONLY);
	if(!~devfd)
		return LOG('E',"failed to open the iio buffer device: %s",devbufpath.c_str()),1;
	return 0;
}
void SensorBase::deinit()
{
	if(~devfd)close(devfd);
	devfd=-1;
}
void SensorBase::reset()
{
	deinit();
	using filesystem::path;
	
	path buffer_enable_path=sysfspath/"buffer"/"enable";
	writeint(buffer_enable_path.c_str(),0);
	for(auto& ent:filesystem::directory_iterator(sysfspath/"scan_elements"))
		if(ent.path().string().substr(ent.path().string().length()-3)=="_en")
		writeint(ent.path().c_str(),0);
}
void SensorBase::worker()
{
	for(workerquit=0;!workerquit;)
	{
		read_m.lock();
		readbuffer();
		read_m.unlock();
		update_values();
		if(readercb!=nullptr)readercb(this);
	}
	deinit();
}
void SensorBase::quit_worker()
{
	workerquit=1;
	ignore_result(write(qpipe[1],"q",1));
	close(qpipe[1]);
}
void SensorBase::pause_worker()
{
	if(~devfd)
	{
		paused=1;
		while(!read_m.try_lock())//just spin lock, I don't care
		{
			ignore_result(write(qpipe[1],"p",1));
			std::this_thread::yield();
		}
		close(devfd);
		devfd=-1;
	}
}
void SensorBase::resume_worker()
{
	for(auto&i:enabled_scan_elem)//update readings
	{
		std::string& elem_base=std::get<1>(i);
		filesystem::path raw_val_path=sysfspath/(elem_base+"_raw");
		dict[elem_base+"_value"]=readint(raw_val_path.c_str());
	}
	devfd=open(devbufpath.c_str(),O_RDONLY);
	if(!~devfd)
		return (void)LOG('E',"failed to open the iio buffer device: %s",devbufpath.c_str());
	paused=0;
	read_m.unlock();
}
void SensorBase::set_reader_callback(std::function<void(SensorBase*)> cb)
{readercb=cb;}
std::string SensorBase::get_type(int id)
{
	using filesystem::path;
	path sysfspath=path(IIODEV_SYSFS_PATH_BASE+std::to_string(id));

	path name_path=sysfspath/"name";
	return readstr(name_path.c_str());
}
int SensorBase::detect_sensor(std::string type)
{
	using filesystem::path;
	path sysfsbasepath=path(IIODEV_SYSFS_PATH_BASE).remove_filename();
	for(auto& ent:filesystem::directory_iterator(sysfsbasepath))
	{
		path name_path=ent.path()/"name";
		if(trim(readstr(name_path.c_str()))==type)
		{
			std::string es=ent.path().filename().string();
			size_t i;
			for(i=0;i<es.length()&&(es[i]<'0'||es[i]>'9');++i);
			if(i<es.length())return atoi(es.c_str()+i);
			return -1;
		}
	}
	return -1;
}
