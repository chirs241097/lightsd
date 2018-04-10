#include "brightness_ctrl.hpp"
#include "utils.hpp"
#include <cmath>
#include <algorithm>
#define log10_n(x) ((x)<1?0:log10(x))
void BrightnessControl::_brightness_slide(int p)
{
	p+=offset;
	if(p>100)p=100;
	if(p<0)p=0;
	int pbr=maxbr*p/100;
	printf("brightness adjust: %d->%d/%d\n",br,pbr,maxbr);
	int d=1;if(pbr<br)d=-1;double dd=1;
	while(d>0&&br+round(d*dd)<=pbr||d<0&&br+round(d*dd)>=pbr)
	{
		br+=(int)round(d*dd);writeint(brpath.c_str(),br);
		dd=dd*1.2;std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	br=pbr;writeint(brpath.c_str(),br);
}

void BrightnessControl::init(float initv,SensorALS *s)
{
	cur=std::upper_bound(thresh.begin(),thresh.end(),(int)roundf(initv))
		-thresh.begin();
	if(thresh.size()+1!=value.size())LOG('W',
		"Size of threshold array should be one more than size of value array",0);
	als=s;set_offset(0,0);
}
void BrightnessControl::set_path(filesystem::path p)
{
	cpath=p;
	brpath=cpath/"brightness";
	maxbrpath=cpath/"max_brightness";
	maxbr=readint(maxbrpath.c_str());
	br=readint(brpath.c_str());
}
void BrightnessControl::set_thresh(std::vector<int> _th){thresh=_th;}
void BrightnessControl::set_value(std::vector<int> _v){value=_v;}
void BrightnessControl::set_delay(int _d){delay=_d;}
void BrightnessControl::set_trigrange(int _tr){tr=_tr;}

void BrightnessControl::set_offset(int rel,int off)
{
	if(rel)offset+=rel*off;else offset=off;
	if(offset>100)offset=100;
	if(offset<-100)offset=-100;
	brightness_slide(value[cur]);
}

void BrightnessControl::on_sensor_report(float v)
{
	int lb=cur>0?thresh[cur-1]:0;
	int ub=cur<thresh.size()?thresh[cur]:~0U>>1;
	if(v<lb-log10_n(lb)*tr||v>ub+log10_n(ub)*tr)
	{
		if(direction!=(v>ub))
		{
			{
				std::lock_guard<std::mutex> lck(interrupt_m);
				direction=(v>ub);
			}
			interrupt.notify_one();
			std::this_thread::yield();
		}
		{
			std::lock_guard<std::mutex> lcd(threshnotify_m);
			//nothing to do within this lock
		}
		threshnotify.notify_one();
		std::this_thread::yield();
	}
}
void BrightnessControl::brightness_slide(int p)
{
	std::thread brth(&BrightnessControl::_brightness_slide,std::ref(*this),p);
	brth.detach();
}
void BrightnessControl::worker()
{
	if(cpath.empty())return;
	while(1)
	{
		std::unique_lock<std::mutex>lock_thresh(threshnotify_m);
		threshnotify.wait(lock_thresh);
		lock_thresh.unlock();
		std::cv_status intr;
		do{
			std::unique_lock<std::mutex>lock_interrupt(interrupt_m);
			intr=interrupt.wait_for(lock_interrupt,std::chrono::seconds(delay));
			lock_interrupt.unlock();
		}while(intr==std::cv_status::no_timeout);
		//carry out brightness adjustment here
		float val=als->get_value();
		int lb=cur>0?thresh[cur-1]:0;
		int ub=cur<thresh.size()?thresh[cur]:~0U>>1;
		while(val>ub)
		{
			++cur;lb=thresh[cur-1];
			ub=cur<thresh.size()?thresh[cur]:~0U>>1;
		}
		while(val<lb)
		{
			--cur;lb=cur>0?thresh[cur-1]:0;
			ub=thresh[cur];
		}
		printf("%f lx\n",val);
		brightness_slide(value[cur]);
	}
}
