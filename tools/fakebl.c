#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
char *path;
char *brpath;
char *mbrpath;
char *cmd;
char *rcmd;
FILE *brf;
int setup_dir()
{
	int r=mkdir(path,0755);
	brpath=malloc(strlen(path)+12);
	mbrpath=malloc(strlen(path)+16);
	strcpy(brpath,path);
	strcpy(mbrpath,path);
	strcat(brpath,"/brightness");
	strcat(mbrpath,"/max_brightness");
	r|=mkfifo(brpath,0666);
	FILE* mbrf=fopen(mbrpath,"w");
	r|=(mbrf==NULL);
	if(mbrf)fputs("100\n",mbrf);
	fclose(mbrf);
	return r;
}
void remove_dir()
{
	unlink(brpath);
	unlink(mbrpath);
	rmdir(path);
	free(brpath);
	free(mbrpath);
}
void usage(char* s)
{
	printf("usage: %s -c <cmd> -p <path>\n",s);
	_exit(1);
}
int main(int argc,char **argv)
{
	if(argc<5)usage(argv[0]);
	for(int i=1;i<argc-1;++i)
	{
		if(!strcmp(argv[i],"-c"))cmd=strdup(argv[i+1]);
		if(!strcmp(argv[i],"-p"))path=strdup(argv[i+1]);
	}
	if(!cmd||!path)
	{
		if(cmd)free(cmd);
		if(path)free(path);
		usage(argv[0]);
	}
	if(setup_dir())return puts("error setting up fake brightness interface"),1;
	rcmd=malloc(strlen(cmd)+16);
	while((brf=fopen(brpath,"r")))
	{
		char dat[64],*eptr;
		eptr=fgets(dat,64,brf);//unused return value
		int v=strtol(dat,&eptr,10),r;
		if(eptr==dat)break;
		snprintf(rcmd,strlen(cmd)+16,cmd,v);
		if((r=system(rcmd)))
		printf("%s returned %d\n",rcmd,r);
		fclose(brf);
	}
	remove_dir();
	free(path);
	free(cmd);
	free(rcmd);
	return 0;
}
