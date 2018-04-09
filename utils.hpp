#ifndef UTILS_HPP
#define UTILS_HPP
#include <string>
#include <vector>
#define LOG(type,format,...) fprintf(stderr,"%c: " format "\n",type,__VA_ARGS__)
extern int readint(const char* path);
extern float readfloat(const char* path);
extern std::string readstr(const char* path);
extern void writeint(const char* path,int v);
extern std::string trim(std::string s);
extern void split(std::string s,char c,std::vector<std::string>& v);
#endif
