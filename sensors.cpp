#include <cstdio>
#include <cstring>
#include <variant>
#include <sys/types.h>
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
	char* buf=new char[readsize];
	ssize_t sz=read(devfd,buf,readsize);
	if(sz==readsize)
	{
		char *p=buf;
		for(int i=0;p-buf<readsize;++i)
		{
			scan_t ti=std::get<scan_t>(enabled_scan_elem[i]);
			std::string es=std::get<std::string>(enabled_scan_elem[i])+"_value";
			if(ti.is_le)
			switch(ti.storagebits)
			{
				case 8:
					if(ti.is_signed)
					{int8_t t;memcpy(&t,p,1);t>>=ti.shift;dict[es]=t;}
					else
					{uint8_t t;memcpy(&t,p,1);t>>=ti.shift;dict[es]=t;}
					++p;
				break;
				case 16:
					if(ti.is_signed)
					{int16_t t;memcpy(&t,p,2);t>>=ti.shift;dict[es]=t;}
					else
					{uint16_t t;memcpy(&t,p,2);t>>=ti.shift;dict[es]=t;}
					p+=2;
				break;
				case 32:
					if(ti.is_signed)
					{int32_t t;memcpy(&t,p,4);t>>=ti.shift;dict[es]=t;}
					else
					{uint32_t t;memcpy(&t,p,4);t>>=ti.shift;dict[es]=t;}
					p+=4;
				break;
				case 64:
					if(ti.is_signed)
					{int64_t t;memcpy(&t,p,8);t>>=ti.shift;dict[es]=t;}
					else
					{uint64_t t;memcpy(&t,p,8);t>>=ti.shift;dict[es]=t;}
					p+=8;
				break;
			}
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
	readsize+=st.storagebits/8;//assume this shit is aligned to byte

	path elem_en_path=sysfspath/"scan_elements"/(elem_base+"_en");
	writeint(elem_en_path.c_str(),1);

	path elem_idx_path=sysfspath/"scan_elements"/(elem_base+"_index");
	int idx;dict[elem_base+"_index"]=idx=readint(elem_idx_path.c_str());
	
	path raw_val_path=sysfspath/(elem_base+"_raw");//initial value
	dict[elem_base+"_value"]=readint(raw_val_path.c_str());
	
	enabled_scan_elem.insert(
		std::upper_bound(enabled_scan_elem.begin(),enabled_scan_elem.end(),
			std::make_tuple(idx,elem_base,st),
			[](const auto&a,const auto&b)->bool{return std::get<0>(a)<std::get<0>(b);}
		),std::make_tuple(idx,elem_base,st)
	);
}
void SensorBase::init(int id,std::string _sensor_basename)
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

	readsize=0;
	enabled_scan_elem.clear();
	enable_scan_elements();
	update_values();
	enable_buffer();
	devfd=open(devbufpath.c_str(),O_RDONLY);
	if(!~devfd)LOG('E',"failed to open the iio buffer device: %s",devbufpath.c_str());
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
		readbuffer();update_values();
		if(readercb!=nullptr)readercb(this);
	}
}
void SensorBase::quit_worker()
{workerquit=1;}
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
