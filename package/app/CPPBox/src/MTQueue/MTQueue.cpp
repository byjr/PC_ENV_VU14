#include "MTQueue.h"
#include <thread>
#include <unistd.h>
#define XXXX 0
class LineDate{
public:
	char *data;
	size_t size;
	LineDate(char *dat,size_t bytes){		
		data = new char[bytes];
		memcpy(data,dat,bytes);
		size = bytes;
	}
	~LineDate(){
		delete []data;	
	}
};
class MTQueueWriterPar{	
public:
	MTQueue* pMQ;
	const char *path;
};
class MTQueueWriter{
	MTQueueWriterPar* mPar;
	std::thread wThread;
public :
	MTQueueWriter(MTQueueWriterPar* par){
		mPar = par;
		wThread = std::thread([this]() {
			FILE *fp = fopen(mPar->path,"rb");
			if(!fp){
				show_errno(0,fopen);
				return;
			}
			ssize_t res = 0;
			char buf[99999];
			do{
				res = fread(buf,1,sizeof(buf),fp);
				if(res < sizeof(buf)){
					if(res <= 0){
						continue;
					}					
				}
				LineDate* pData = new LineDate(buf,res);
				if(!pData){
					s_err("oom");
					continue;
				}
				do{
					res = mPar->pMQ->write(pData);	
					if(res < 0){
						// s_err("pMQ->write failed");
						usleep(1000*10);
						continue;
					}				
				}while(res < 0);
			}while(!feof(fp));
			fclose(fp);
			mPar->pMQ->setWaitExitState();
			s_inf("read %s done!",mPar->path);
		});
	}
	~MTQueueWriter(){
		if(wThread.joinable()){
			wThread.join();
		}
	}
};
class MTQueueReaderPar{	
public:
	MTQueue *pMQ;
	const char *path;
};
class MTQueueReader{
	MTQueueReaderPar* mPar;
	std::thread rThread;
public :
	MTQueueReader(MTQueueReaderPar* par){
		mPar = par;
		rThread = std::thread([this]() {
			FILE *fp = fopen(mPar->path,"wb");
			if(!fp){
				show_errno(0,fopen);
				return NULL;
			}
			ssize_t res = 0;
			LineDate* pData =NULL;
			do{
				pData = (LineDate*)mPar->pMQ->read(1000);
				if(!pData){
					continue;
				}
				res = fwrite(pData->data,1,pData->size,fp);	
				if(res < pData->size){	
					s_err("fwrite res=%d",res);
					if(res <= 0){
						continue;
					}					
				}
				delete pData;	
			}while(pData);
			fclose(fp);
			s_inf("write %s done!",mPar->path);
			s_inf("pMQ remain size:%lu",mPar->pMQ->getSize());
		});
	}
	~MTQueueReader(){
		if(rThread.joinable()){
			rThread.join();
		}
	}
};
#include <getopt.h>
int help_info(int argc ,char *argv[]){
	s_err("%s help:",get_last_name(argv[0]));
	s_err("\t-s [input fps]");
	s_err("\t-i [input path]");
	s_err("\t-o [output url]");
	s_err("\t-u [user:passwd]");
	s_err("\t-l [logLvCtrl]");
	s_err("\t-p [logPath]");
	s_err("\t-h show help");
	return 0;
}
int MTQueue_main(int argc,char *argv[]){
	int opt = 0;
	MTQueueReaderPar readerPar={0};
	MTQueueWriterPar writerPar={0};
	while ((opt = getopt_long_only(argc, argv,"u:b:i:o:l:p:h",NULL,NULL)) != -1) {
		switch (opt) {
		case 'l':
			lzUtils_logInit(optarg,NULL);
			break;
		case 'p':
			lzUtils_logInit(NULL,optarg);
			break;
		case 'i':
			writerPar.path = optarg;
			break;
		case 'o':
			readerPar.path = optarg;
			break;
		default: /* '?' */
			return help_info(argc ,argv);
	   }
	}	
	MTQueue* pMQ = new MTQueue(10);
	if(!pMQ){
		s_err("newMTQueue failed!");
		return NULL;
	}
	readerPar.pMQ = writerPar.pMQ = pMQ;
	auto mReader = new MTQueueReader(&readerPar);
	if(!mReader){
		s_err("new MTQueueReader failed!");
	}
	auto mWriter = new MTQueueWriter(&writerPar);
	if(!mReader){
		s_err("new MTQueueWriter failed!");
	}
	delete mWriter;
	delete mReader;
	delete pMQ;
}