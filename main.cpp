#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <condition_variable>
#include <chrono>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "utils.hpp"
#include "sensor_als.hpp"
#include "brightness_ctrl.hpp"
SensorALS als;
int als_id;

filesystem::path fifo_path;
BrightnessControl lcd,kbd;
FILE *fifo_f;

void als_callback(SensorBase* _s)
{
	SensorALS* s=(SensorALS*)_s;
	float val=s->get_value();
	lcd.on_sensor_report(val);
	kbd.on_sensor_report(val);
}
void load_config()
{
	fifo_path="/tmp/lightsd.cmd.fifo";
	FILE* cfgf;
	cfgf=fopen("/etc/lightsd.conf","r");
	if(!cfgf)
	cfgf=fopen("lightsd.conf","r");
	if(!cfgf){LOG('W',"Configuration file not found.",0);return;}
	char* buf=new char[1024];
	while(!feof(cfgf))
	{
		fgets(buf,1024,cfgf);
		if(buf[0]=='#')continue;
		std::string sb=trim(buf);
		std::vector<std::string> sv;
		split(sb,'=',sv);
		if(sv.size()!=2)continue;
		if(sv[0]=="lcd_backlight_control")lcd.set_path(sv[1]);
		if(sv[0]=="kbd_backlight_control")kbd.set_path(sv[1]);
		if(sv[0]=="lcd_backlight_thresholds")
		{
			std::vector<std::string> vals;
			split(sv[1],',',vals);
			std::vector<int> t;
			for(auto&i:vals)t.push_back(atoi(i.c_str()));
			lcd.set_thresh(t);
		}
		if(sv[0]=="lcd_backlight_values")
		{
			std::vector<std::string> vals;
			split(sv[1],',',vals);
			std::vector<int> t;
			for(auto&i:vals)t.push_back(atoi(i.c_str()));
			lcd.set_value(t);
		}
		if(sv[0]=="lcd_backlight_control_delay")lcd.set_delay(atoi(sv[1].c_str()));
		if(sv[0]=="lcd_backlight_trigger_range")lcd.set_trigrange(atoi(sv[1].c_str()));
		if(sv[0]=="kbd_backlight_thresholds")
		{
			std::vector<std::string> vals;
			split(sv[1],',',vals);
			std::vector<int> t;
			for(auto&i:vals)t.push_back(atoi(i.c_str()));
			kbd.set_thresh(t);
		}
		if(sv[0]=="kbd_backlight_values")
		{
			std::vector<std::string> vals;
			split(sv[1],',',vals);
			std::vector<int> t;
			for(auto&i:vals)t.push_back(atoi(i.c_str()));
			kbd.set_value(t);
		}
		if(sv[0]=="kbd_backlight_control_delay")kbd.set_delay(atoi(sv[1].c_str()));
		if(sv[0]=="kbd_backlight_trigger_range")kbd.set_trigrange(atoi(sv[1].c_str()));
		if(sv[0]=="command_fifo_path")fifo_path=sv[1];
	}
	delete[] buf;
}
int get_gid(std::string group)
{
	FILE* grpf=fopen("/etc/group","r");
	char* buf=new char[1024];
	while(!feof(grpf))
	{
		fgets(buf,1024,grpf);
		std::vector<std::string> sv;
		split(buf,':',sv);
		if(sv[0]==group)
		{
			fclose(grpf);
			delete[] buf;
			return atoi(sv[2].c_str());
		}
	}
	fclose(grpf);
	delete[] buf;
	return -1;
}
void setup_fifo()
{
	if(!fifo_path.string().length())return;
	unlink(fifo_path.c_str());
	mkfifo(fifo_path.c_str(),0620);
	chown(fifo_path.c_str(),0,get_gid("video"));
	chmod(fifo_path.c_str(),0620);
}
void command_thread()
{
	fifo_f=fopen(fifo_path.c_str(),"r");
	char cmdbuf[256];
	while(1)
	{
		fgets(cmdbuf,256,fifo_f);
		printf("got command: ");
		puts(trim(cmdbuf).c_str());
		std::vector<std::string> cav;
		split(trim(cmdbuf),' ',cav);
		if(cav.size()>=1)
		{
			if(cav[0]=="u")if(cav.size()>1)lcd.set_offset(1,atoi(cav[1].c_str()));
			if(cav[0]=="d")if(cav.size()>1)lcd.set_offset(-1,atoi(cav[1].c_str()));
			if(cav[0]=="s")if(cav.size()>1)lcd.set_offset(0,atoi(cav[1].c_str()));
			if(cav[0]=="r")lcd.set_offset(0,0);
		}
		fclose(fifo_f);
		fifo_f=fopen(fifo_path.c_str(),"r");
	}
}
void sigterm_handler(int)
{
	exit(0);
}
int main()
{
	signal(SIGTERM,sigterm_handler);
	als_id=SensorBase::detect_sensor("als");
	if(!~als_id)return puts("No ALS found!"),1;
	als.init(als_id,"in_intensity");
	als.set_reader_callback(als_callback);
	float init_val=als.get_value();
	printf("initial value: %f lx\n",init_val);
	load_config();
	setup_fifo();
	lcd.init(init_val,&als);
	kbd.init(init_val,&als);
	std::thread lcd_thread(&BrightnessControl::worker,std::ref(lcd));
	std::thread kbd_thread(&BrightnessControl::worker,std::ref(kbd));
	std::thread cmd_thread(command_thread);
	als.worker();
	return 0;
}
