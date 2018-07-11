//Chris Xiong 2018
//3-Clause BSD License
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cctype>
#include "utils.hpp"
int readint(const char* path)
{
	FILE* f=fopen(path,"r");
	if(!f)return LOG('W',"failed to open %s for reading: %d",path,errno),0;
	char buf[16];
	ignore_result(fgets(buf,16,f));
	buf[15]=0;
	fclose(f);
	return atoi(buf);
}
float readfloat(const char* path)
{
	FILE* f=fopen(path,"r");
	if(!f)return LOG('W',"failed to open %s for reading: %d",path,errno),0;
	char buf[16];
	ignore_result(fgets(buf,16,f));
	buf[15]=0;
	fclose(f);
	return atof(buf);
}
std::string readstr(const char* path,int max_length)
{
	FILE* f=fopen(path,"r");
	if(!f)return LOG('W',"failed to open %s for reading: %d",path,errno),"";
	char* buf=new char[max_length+1];
	ignore_result(fgets(buf,max_length+1,f));
	buf[max_length]=0;
	fclose(f);
	std::string ret(buf);
	delete buf;
	return ret;
}
void writeint(const char* path,int v)
{
	FILE* f=fopen(path,"w");
	if(!f){LOG('W',"failed to open %s for writing",path);return;}
	ignore_result(fprintf(f,"%d",v));
	fclose(f);
}
std::string trim(std::string s)
{
	size_t st=0;
	for(;st<s.length()&&isspace(s[st]);++st);
	if(st==s.length())return "";
	s=s.substr(st);
	while(s.length()&&isspace(s.back()))s.pop_back();
	return s;
}
void split(std::string s,char c,std::vector<std::string>& v)
{
	v.clear();
	for(size_t anch=0;;)
	{
		std::string sec;
		if(s.find(c,anch)==std::string::npos)
		sec=s.substr(anch);
		else sec=s.substr(anch,s.find(c,anch)-anch);
		v.push_back(sec);
		if(s.find(c,anch)==std::string::npos)break;
		anch=s.find(c,anch)+1;
	}
}
