#include <lzUtils/base.h>
#include "RePlayer.h"
RePlayer::RePlayer(RePlayerPar* par){
	mPar = par;
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
	isPauseFlag = false;
	mTrd = std::thread([this](){
		char framesBuf[PERIOD_BYTES];
		for(;;){
			if(isPauseFlag){
				usleep(1000*40);
				continue;
			}
			ssize_t rret = alsa_ctrl_read_stream(mRec,framesBuf,PERIOD_BYTES);
			if(rret != PERIOD_BYTES){
				s_err("alsa_ctrl_read failed,reset ...");
				alsa_ctrl_reset(mRec,mPar->pRecPar);
				continue;
			}
			ssize_t wret = alsa_ctrl_write_stream(mPly,framesBuf,rret);
			if(wret != rret){
				s_err("alsa_ctrl_read failed,reset ...");
				alsa_ctrl_reset(mPly,mPar->pPlyPar);
				continue;
			}
		}
	});
}
RePlayer::~RePlayer(){
	if(mTrd.joinable()){
		mTrd.join();
	}
	alsa_ctrl_destroy(mRec);		
	alsa_ctrl_destroy(mPly);
}
void RePlayer::pause(){
	isPauseFlag = true;
	alsa_ctrl_pause(mRec);
	usleep(40*1000);
	alsa_ctrl_pause(mPly);
}
void RePlayer::resume(){
	isPauseFlag = false;
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
static RePlayer* g_mRtPlayer = NULL;
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
		s_war("isPaused:%d",isPaused?1:0);
		break;
	default:
		break;
	}
}
#include <getopt.h>
int RePlayer_main(int argc, char *argv[]){
	alsa_args_t recPar={
		.device 	 = "plughw:0,1,0",
		.sample_rate = 48000,
		.channels 	 = 2,
		.action 	 = SND_PCM_STREAM_CAPTURE,
	};
	alsa_args_t plyPar={
		.device 	 = "plughw:1,1",
		.sample_rate = 48000,
		.channels 	 = 2,
		.action 	 = SND_PCM_STREAM_PLAYBACK,
	};
	int opt = 0;
	while ((opt = getopt_long_only(argc, argv, "i:o:l:ph",NULL,NULL)) != -1) {
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
		default: /* '?' */
			return help_info(argc ,argv);
	   }
	}	
//打印编译时间
	signal(SIGUSR1,sig_handle);
	showCompileTmie(argv[0],s_war);
	RePlayerPar mPar = {
		.pRecPar = &recPar,
		.pPlyPar = &plyPar,
	};
	RePlayer* mRtPlayer = new RePlayer(&mPar);
	if(!mRtPlayer){
		s_err("");
		return -1;
	}
	g_mRtPlayer = mRtPlayer;
	delete mRtPlayer;
	return -1;
}