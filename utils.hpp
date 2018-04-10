#ifndef UTILS_HPP
#define UTILS_HPP
#include <string>
#include <vector>
#define LOG(type,format,...) fprintf(stderr,"%c: " format "\n",type,__VA_ARGS__)
int readint(const char* path);
float readfloat(const char* path);
std::string readstr(const char* path);
void writeint(const char* path,int v);
std::string trim(std::string s);
void split(std::string s,char c,std::vector<std::string>& v);
template<typename T>void ignore_result(T){}
#endif
