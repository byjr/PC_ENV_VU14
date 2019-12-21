#include <lzUtils/base.h>
#include "RtPlayer.h"
class chunkData{
	char *data;
	size_t size;	
public:
	chunkData(char *dat,size_t bytes){
		data = new char[bytes];
		memcpy(data,dat,bytes);
		size = bytes;
	}
	~chunkData(){
		delete []data;
	}
	char* getData(){
		return data;
	}
	size_t getSize(){
		return size;
	}
	static void destroy(void* one);
};
void chunkData::destroy(void* one){
	auto chunk = (chunkData*)one;
	if(!chunk){
		return ;
	}
	delete chunk;
}
RtPlayer::RtPlayer(RtPlayerPar* par){
	mPar = par;
	mMTQ = new MTQueue(mPar->pMTQPar);
	if(!mMTQ){
		s_err("new MTQueue failed");
		return ;			
	}
	s_inf("idev:%s;odev:%s",mPar->pRecPar->device,mPar->pPlyPar->device);
	mPly = alsa_ctrl_create(mPar->pPlyPar);
	if(!mPly){
		s_err("alsa_ctrl_create player failed");
		return ;
	}			
	mRec = alsa_ctrl_create(mPar->pRecPar);
	if(!mRec){
		s_err("alsa_ctrl_create recorder failed");
		return ;
	}
	mPauseFlag = false;
	mFullFlag = false;
	mRecTrd = std::thread([this](){
		char framesBuf[PERIOD_BYTES];
		chunkData *chunk;
		for(;;){
			if(mPauseFlag){
				usleep(1000*100);
				continue;
			}
			ssize_t rret = alsa_ctrl_read_stream(mRec,framesBuf,PERIOD_BYTES);
			if(rret != PERIOD_BYTES){
				s_err("alsa_ctrl_read failed,reset ...");
				continue;
			}
			chunk = new chunkData(framesBuf,rret);
			if(!chunk){
				s_err("new chunkData failed");
				continue;
			}
			mFullFlag = mMTQ->cycWrite(chunk);
		}
	});
	while(!mFullFlag){
		usleep(1000*10);
	}
	mPlyTrd = std::thread([this](){
		chunkData *chunk;
		for(;;){
			if(mPauseFlag){
				usleep(1000*100);
				continue;
			}			
			chunk =(chunkData*)mMTQ->read(5);
			if(!chunk){
				s_err("exit play thread");
				break;
			}
			ssize_t wret = alsa_ctrl_write_stream(mPly,chunk->getData(),chunk->getSize());
			if(wret != chunk->getSize()){
				s_err("alsa_ctrl_read failed,reset ...");
			}
			delete chunk;
		}
	});
}
RtPlayer::~RtPlayer(){
	if(mRecTrd.joinable()){
		mRecTrd.join();
	}
	alsa_ctrl_destroy(mRec);		
	if(mPlyTrd.joinable()){
		mPlyTrd.join();
	}
	alsa_ctrl_destroy(mPly);
	delete mMTQ;
}
void RtPlayer::pause(){
	mPauseFlag = true;
	alsa_ctrl_pause(mRec);
	alsa_ctrl_pause(mPly);
	mMTQ->clear();
}
void RtPlayer::resume(){
	mPauseFlag = false;
	alsa_ctrl_resume(mRec);
	usleep(40*1000);
	alsa_ctrl_resume(mPly);			
}
static int help_info(int argc, char *argv[]){
	printf("%s help:\n",get_last_name(argv[0]));
	printf("\t-t [rFifo:wFifo]:use fifo to test logic.\n");
	printf("\t-m :Use stdin to simulate the Mastr.\n");
	printf("\t-b [baudRate]:Plz use 9600/38400/115200/1500000.\n");
	printf("\t-l [logLvCtrl]\n");
	printf("\t-p [logPath]\n");
	printf("\t-h show help\n");
	return 0;
}
RtPlayer* g_mRtPlayer = NULL;
static void sig_handle(int sig){
	static bool isPaused = false;
	switch(sig){
	case SIGUSR1:
		if(isPaused){
			isPaused = false;
			g_mRtPlayer->resume();
		}else{
			isPaused = true;
			g_mRtPlayer->pause();
		}
		break;
	default:
		break;
	}
}
#include <getopt.h>
int RtPlayer_main(int argc, char *argv[]){
	alsa_args_t recPar={
		.device 	 = "plughw:0,1,0",
		.sample_rate = 48000,
		.channels 	 = 2,
		.action 	 = SND_PCM_STREAM_CAPTURE,
		.flags		 = 0,
		.fmt		 = SND_PCM_FORMAT_S16_LE,
	};
	alsa_args_t plyPar={
		.device 	 = "plughw:1,1",
		.sample_rate = 48000,
		.channels 	 = 2,
		.action 	 = SND_PCM_STREAM_PLAYBACK,
		.flags		 = 0,
		.fmt		 = SND_PCM_FORMAT_S16_LE,
	};
	MTQueuePar MTQPar = {
		.mMax = 4,
		.destroyOne = &chunkData::destroy,
	};
	int opt = 0;
	while ((opt = getopt_long_only(argc, argv, "m:i:o:l:ph",NULL,NULL)) != -1) {
		switch (opt) {
		case 'l':
			lzUtils_logInit(optarg,NULL);
			break;
		case 'p':
			lzUtils_logInit(NULL,optarg);
			break;
		case 'i':
			recPar.device = optarg;
			break;
		case 'o':
			plyPar.device = optarg;
			break;
		case 'm':
			MTQPar.mMax = atoi(optarg);	
			break;			
		default: /* '?' */
			return help_info(argc ,argv);
	   }
	}	
//打印编译时间
	signal(SIGUSR1,sig_handle);
	showCompileTmie(argv[0],s_war);
	RtPlayerPar mPar = {
		.pRecPar = &recPar,
		.pPlyPar = &plyPar,
		.pMTQPar = &MTQPar,
	};
	RtPlayer* mRtPlayer = new RtPlayer(&mPar);
	if(!mRtPlayer){
		s_err("");
		return -1;
	}
	g_mRtPlayer = mRtPlayer;
	delete mRtPlayer;
	return -1;
}