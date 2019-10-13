#include <getopt.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>

#define s_raw(x...) 	printf(x)
#define s_err(x...) 	printf("ERR [%s %d]:",__func__,__LINE__);printf(x);printf("\n")
#define s_war(x...) 	printf("WAR [%s %d]:",__func__,__LINE__);printf(x);printf("\n")
#if 0
#define s_inf(x...) 	printf("INF [%s %d]:",__func__,__LINE__);printf(x);printf("\n")
#define s_dbg(x...) 	printf("DBG [%s %d]:",__func__,__LINE__);printf(x);printf("\n")
#define s_trc(x...) 	printf("TRC [%s %d]:",__func__,__LINE__);printf(x);printf("\n")
#else
#define s_inf(x...)
#define s_dbg(x...)
#define s_trc(x...)
#endif

#define show_member(msg,c) do{\
	s_inf(msg);\
	for(auto i : c){\
		s_raw("%s",i.c_str());\
		s_raw("\n");\
	}\
}while(0)
	
using namespace std;

int userage(int argc,char *argv[]){
	printf("%s help:\n",argv[0]);
	printf("\t-s [pkg]        :pkg:short pacakge path.\n");
	printf("\t-f [path]       :path:The package list you want to build.\n");
	// printf("\t-l [log_ctrl]   :log_ctrl:log level contronl code.\n");
	// printf("\t-p [log_tty]    :log_tty:the tty path of you want write log to.\n");
	return 0;
}

string getCmdRes(const char *fmt, ...){
	va_list args;
	va_start(args, (char *)fmt);
	char *cmd = NULL;
	vasprintf(&cmd, fmt, args);
	va_end(args);
	FILE *fp = popen(cmd,"r");
	if(!fp){
		s_err("popen");
		return "";
	}
	fcntl(fileno(fp), F_SETFD, FD_CLOEXEC);
	string cmdRes;
	do{
		char buf[256]="";
		char *res = fgets(buf,sizeof(buf),fp);
		if(!res){
			break;
		}
		cmdRes += buf;
	}while(!feof(fp));
	fclose(fp);
	if(cmd)free(cmd);
	return cmdRes;
}
list<string> getPkgDeps(string pkg){
	s_dbg("make -C %s depend",pkg.c_str());
	string cmdRes = getCmdRes("make -C %s depend",pkg.c_str());
	stringstream sCmdRes(cmdRes);
	list<string> deps;
	string temp;
	while(getline(sCmdRes,temp)){
		if(temp.find("make")){
			break;
		}
	}
	stringstream sDeps(temp);
	while(getline(sDeps,temp,' ')){
		deps.push_back(temp);
	}
	deps.unique();
	return deps;
}


	
list<string> up_active_list(string pkg,list<string> dep_list,list<string> active_list,list<string> un_list){
	// show_member("dep_list:",dep_list);
	// show_member("un_list:",un_list);
	s_dbg("---------------------------------------------");
	if(!dep_list.empty()){
		for(auto i : dep_list){
			bool in_un_list = false;
			for(auto k : un_list){
				s_dbg("-----i:%s--k:%s--",i.c_str(),k.c_str());
				if(!k.compare(i)){
					in_un_list = true;
					break;
				}
			}
			if(in_un_list){
				continue;
			}
			bool had_appeared = false;
			for(auto j : active_list){
				if(!j.compare(i)){
					had_appeared = true;
					break;
				}
			}
			s_dbg("i:%s------",i.c_str());
			if(!had_appeared){
				s_dbg("i:%s------",i.c_str());
				list<string> subd_list = getPkgDeps(i);
				un_list.push_back(i);
				active_list=up_active_list(i,subd_list,active_list,un_list);			
			}
		}	
	}
	for(auto i : active_list){
		if(!pkg.compare(i)){
			return active_list;
		}
	}
	active_list.push_back(pkg);
	return active_list;
}
list<string>  get_pkg_active_list(string pkg,list<string> active_list){
	list<string> dep_list = getPkgDeps(pkg);
	list<string> un_list(1,pkg);
	return up_active_list(pkg,dep_list,active_list,un_list);	
}
int main(int argc,char *argv[]){
	s_inf("%s build in:[%s %s].",argv[0],__DATE__,__TIME__);
	int opt = 0;
	const char *path = NULL;
	const char *pkg = NULL;
	while ((opt = getopt(argc, argv, "l:p:f:s:h")) != -1) {
		switch (opt) {
		case 'f':path = optarg;
			break;
		case 's':pkg = optarg;
			break;			
		// case 'l':lzUtils_logInit(optarg,NULL);
			// break;
		// case 'p':lzUtils_logInit(NULL,optarg);
			// break;				
		case 'h':userage(argc,argv);
			return 0;
		default:printf("invaild option!\n");
			userage(argc,argv);
			return -__LINE__;
	   }
	}
	list<string> active_list;
	if(pkg){
		s_inf("get_pkg_active_list...");
		active_list = get_pkg_active_list(pkg,active_list);
	}else if(path){
		s_inf("get pkgList_ifs pkg list...");
		list<string> pkg_list;
		ifstream pkgList_ifs(path);
		string line;
		while(getline(pkgList_ifs,line)){
			pkg_list.push_back(line);
		}
		pkg_list.unique();
		for(auto i:pkg_list){
			active_list = get_pkg_active_list(i,active_list);		
		}		
	}else{
		printf("invaild option!\n");
		userage(argc,argv);
		return -__LINE__;		
	}
	show_member("final active_list:",active_list);
	return 0;
}
