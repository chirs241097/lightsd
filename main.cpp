//Chris Xiong 2018
//3-Clause BSD License
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
bool manualmode;

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
std::vector<int> numlist2vec(std::string l)
{
	std::vector<std::string> v;
	split(l,',',v);
	std::vector<int> r;
	for(auto&i:v)r.push_back(atoi(i.c_str()));
	return r;
}
void load_config()
{
	fifo_path="/tmp/lightsd.cmd.fifo";
	FILE* cfgf;
	cfgf=fopen("/etc/lightsd.conf","r");
	if(!cfgf)
	cfgf=fopen("lightsd.conf","r");
	if(!cfgf)return (void)LOG('W',"Configuration file not found.",0);
	char* buf=new char[1024];
	while(!feof(cfgf))
	{
		ignore_result(fgets(buf,1024,cfgf));
		if(buf[0]=='#')continue;
		std::string sb=trim(buf);
		std::vector<std::string> sv;
		split(sb,'=',sv);
		if(sv.size()!=2)continue;
		if(sv[0]=="lcd_backlight_control")lcd.set_path(sv[1]);
		if(sv[0]=="kbd_backlight_control")kbd.set_path(sv[1]);
		if(sv[0]=="lcd_backlight_thresholds")lcd.set_thresh(numlist2vec(sv[1]));
		if(sv[0]=="lcd_backlight_values")lcd.set_value(numlist2vec(sv[1]));
		if(sv[0]=="lcd_backlight_control_delay")lcd.set_delay(atoi(sv[1].c_str()));
		if(sv[0]=="lcd_backlight_trigger_range")lcd.set_trigrange(atoi(sv[1].c_str()));
		if(sv[0]=="lcd_backlight_min_value")lcd.set_minabr(atoi(sv[1].c_str()));
		if(sv[0]=="kbd_backlight_thresholds")kbd.set_thresh(numlist2vec(sv[1]));
		if(sv[0]=="kbd_backlight_values")kbd.set_value(numlist2vec(sv[1]));
		if(sv[0]=="kbd_backlight_control_delay")kbd.set_delay(atoi(sv[1].c_str()));
		if(sv[0]=="kbd_backlight_trigger_range")kbd.set_trigrange(atoi(sv[1].c_str()));
		if(sv[0]=="kbd_backlight_min_value")kbd.set_minabr(atoi(sv[1].c_str()));
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
		ignore_result(fgets(buf,1024,grpf));
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
void cleanup()
{
	als.quit_worker();
	if(!fifo_path.empty())unlink(fifo_path.c_str());
}
void sighandler(int)
{
	if(!fifo_path.empty())//command thread *exists*
	{
		FILE* tf=fopen(fifo_path.c_str(),"w");
		fputs("q\n",tf);
		//let the command thread cleanup
		fclose(tf);
	}
	else
		cleanup();
}
void setup_fifo()
{
	if(fifo_path.empty())return;
	int ret=0;
	unlink(fifo_path.c_str());
	ret|=mkfifo(fifo_path.c_str(),0220);
	ret|=chown(fifo_path.c_str(),0,get_gid("video"));
	ret|=chmod(fifo_path.c_str(),0220);
	if(ret)
	{
		LOG('W',"Failed to create fifo.",0);
		unlink(fifo_path.c_str());
		fifo_path="";
	}
}
void command_thread()
{
	if(fifo_path.empty())return;
	fifo_f=fopen(fifo_path.c_str(),"r");
	char cmdbuf[256];
	while(1)
	{
		ignore_result(fgets(cmdbuf,256,fifo_f));
		LOG('I',"got command: %s",trim(cmdbuf).c_str());
		std::vector<std::string> cav;
		split(trim(cmdbuf),' ',cav);
		if(cav.size()>=1)
		{
			if(cav[0]=="u")
			{
				if(cav.size()>1)lcd.set_offset(1,atoi(cav[1].c_str()));
				else lcd.set_offset(1,5);
			}
			if(cav[0]=="d")
			{
				if(cav.size()>1)lcd.set_offset(-1,atoi(cav[1].c_str()));
				else lcd.set_offset(-1,5);
			}
			if(cav[0]=="s")if(cav.size()>1)lcd.set_offset(0,atoi(cav[1].c_str()));
			if(cav[0]=="r"&&!manualmode)lcd.set_offset(0,0);
			if(cav[0]=="f"&&!manualmode)
			{
				lcd.force_adjust();
				kbd.force_adjust();
			}
			if(cav[0]=="m")
				if(!manualmode)
				{
					als.pause_worker();
					lcd.set_frozen(true);
					kbd.set_frozen(true);
					manualmode=true;
				}
			if(cav[0]=="a")
				if(manualmode)
				{
					manualmode=false;
					als.resume_worker();
					lcd.set_frozen(false);
					kbd.set_frozen(false);
				}
			if(cav[0]=="i")
			{
				printf("Mode: %s\n",manualmode?"Manual":"Automatic");
				printf(manualmode?"ALS value: --\n":"ALS value: %.2f\n",als.get_value());
				printf("Display brightness: %d%%",lcd.get_brightness());
				if(!manualmode&&lcd.get_offset())printf(" (+%d%%)\n",lcd.get_offset());else putchar('\n');
				//TODO: check for existance
				printf("Keyboard backlight: %d%%\n",kbd.get_brightness());
			}
			if(cav[0]=="q")
			{
				fclose(fifo_f);
				cleanup();
				return;
			}
		}
		fclose(fifo_f);
		fifo_f=fopen(fifo_path.c_str(),"r");
	}
}
int main()
{
	signal(SIGTERM,sighandler);
	signal(SIGINT,sighandler);
	als_id=SensorBase::detect_sensor("als");
	if(!~als_id)return puts("No ALS found!"),1;
	if(als.init(als_id,"in_intensity"))return puts("Failed to initialize sensor."),1;
	als.set_reader_callback(als_callback);
	float init_val=als.get_value();
	manualmode=false;
	load_config();
	setup_fifo();
	lcd.init(init_val,&als);
	kbd.init(init_val,&als);
	std::thread (&BrightnessControl::worker,std::ref(lcd)).detach();
	std::thread (&BrightnessControl::worker,std::ref(kbd)).detach();
	std::thread (command_thread).detach();
	als.worker();
	_exit(0);
}
