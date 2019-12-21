#include "ffapp.h"
#include <libavutil/timestamp.h>
#include "curlPush.h"
RtspClient::RtspClient(RtspCliPar* para){
	mParam = para;
	vsIdx = -1;
	asIdx = -1;	
	mPullTread = std::thread(&RtspClient::pullSteamLoop,this);
    if(!mPullTread.joinable()) {
		s_err("thread");
		return;
    }
}
RtspClient::~RtspClient(){
	if(mPullTread.joinable()) {
		mPullTread.join();
	}	
}
#define av_ts2strc(ts) ({\
	const char buf[AV_TS_MAX_STRING_SIZE]={0};\
	av_ts_make_string((char *)buf, ts);\
})
#define av_ts2timestrc(ts, tb) ({\
	const char buf[AV_TS_MAX_STRING_SIZE]={0};\
	av_ts_make_time_string((char *)buf, ts, tb);\
}) 
#define av_err2strc(ret) ({\
	const char buf[AV_ERROR_MAX_STRING_SIZE]={0};\	
    av_make_error_string((char *)buf, AV_ERROR_MAX_STRING_SIZE, ret);\
})
static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)
{
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    s_war("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d",
           av_ts2strc(pkt->pts), av_ts2timestrc(pkt->pts, time_base),
           av_ts2strc(pkt->dts), av_ts2timestrc(pkt->dts, time_base),
           av_ts2strc(pkt->duration), av_ts2timestrc(pkt->duration, time_base),
           pkt->stream_index);
}
std::string RtspClient::getTimeStemp(){
	#define FF_TIME_LAY "2019-12-05_16:03:55"
	char buf[]=FF_TIME_LAY;
	struct timespec ts={0};
	struct tm* timeinfo;
	clock_gettime(CLOCK_REALTIME,&ts);
	timeinfo=localtime(&ts.tv_sec);
	strftime(buf,sizeof(FF_TIME_LAY),"%Y-%m-%d_%H:%M:%S",timeinfo);
	return buf;
}

int RtspClient::pullSteamLoop(){
    //使用TCP连接打开RTSP，设置最大延迟时间
    char errbuf[512]={0};
    AVDictionary *avdic = NULL;  
	char option_key[]="rtsp_transport";  
	char option_value[]="tcp";  
	av_dict_set(&avdic,option_key,option_value,0);  
    char option_key2[]="max_delay";  
    char option_value2[]="5000000";
    av_dict_set(&avdic,option_key2,option_value2,0); 
	// Allocate an AVFormatContext
	AVFormatContext* ifmt_ctx = avformat_alloc_context();
	
	// open rtsp: Open an input stream and read the header. The codecs are not opened
	int ret = avformat_open_input(&ifmt_ctx, mParam->iPath, NULL, &avdic);
	if (ret != 0) {
		s_err("fail to open url: %s, error:%s", mParam->iPath, av_err2strc(ret));			
		return -1;
	}
	
	// Read packets of a media file to get stream information
	ret = avformat_find_stream_info(ifmt_ctx, NULL);
	if ( ret < 0) {
		s_err("fail to get stream information  error:%s",av_err2strc(ret));
		return -1;
	}
	
	s_inf("Number of elements in AVFormatContext.streams: %d", ifmt_ctx->nb_streams);
	AVStream* in_stream = NULL;
	for (int i = 0; i < ifmt_ctx->nb_streams; ++i) {
		in_stream = ifmt_ctx->streams[i];
		s_inf("type of the encoded data: %d", in_stream->codecpar->codec_id);
		if (in_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			vsIdx = i;
			mFps = in_stream->avg_frame_rate.num/in_stream->avg_frame_rate.den;
			s_inf("The video frame in pixels: width: %d, height: %d, pixel format: %d,fps:%d",
				in_stream->codecpar->width, in_stream->codecpar->height, in_stream->codecpar->format,mFps);
		} else if (in_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			asIdx = i;
			s_inf("audio sample format: %d", in_stream->codecpar->format);
		}
	}	
	AVPacket pkt;
	mBAStream_status = dumpSteamStatus::START;
	BAStream* mVbas = NULL;
	static FILE *fp = NULL;
	if(!fp){
		fp = fopen(mParam->bakPath,"w+");
	}	
	for(;;) {
		ret = av_read_frame(ifmt_ctx, &pkt);
		if(ret){
			if(ret == AVERROR_EOF ) {
				av_strerror(ret,errbuf,sizeof(errbuf));
				s_war("av_read_frame :%s", errbuf);
				break;
			}else{
				av_strerror(ret,errbuf,sizeof(errbuf));
				s_err("av_read_frame :%s", errbuf);
				break;
			}
		}
		s_dbg("mBAStream_status=%d",mBAStream_status);
		if (pkt.stream_index == vsIdx) {
			s_dbg("video stream, packet : %d", pkt.size);   
			if(dumpSteamStatus::START == mBAStream_status ){
				MTQueuePar mPar={0};
				mPar.mMax = 10 * mFps;
				mPar.destroyOne = &frameDataDestroy;
				mVbas = new BAStream(&mPar);
				if(!mVbas){
					s_err("new BAStream failed!");
					continue;
				}
				mBAStream_status = dumpSteamStatus::CYCW;
			}
			if(dumpSteamStatus::CYCW == mBAStream_status){
				mVbas->writeBefor(pkt);
				if(mParam->bakPath){
					if(fp){
						fwrite(pkt.data,1,pkt.size,fp);
					}				
				}			
			}else if(dumpSteamStatus::GETI == mBAStream_status){
				if(mVbas->getRiFlags() & AV_PKT_FLAG_KEY){
					mBAStream_status = dumpSteamStatus::DUMP;
					mVbas->writeAfter(pkt);
					std::string oPath = mParam->oPath;
					oPath += getTimeStemp();
					oPath += ".264";					
					curlPushCliPar* curlPar = new curlPushCliPar();
					curlPar->mDstPath = oPath;
					curlPar->userpwd = mParam->userpwd;s_inf("userpwd:%s", mParam->userpwd);
					curlPar->vBAs = mVbas;
					std::thread m_trd = std::thread([this,curlPar]() {
						curlPushUp(curlPar);
					});
					m_trd.detach();
					s_war("package %s start...",oPath.data());						
				}else{
					mVbas->writeBefor(pkt);
				}				
				if(mParam->bakPath){
					if(fp){
						fwrite(pkt.data,1,pkt.size,fp);
					}				
				}				
			}else if(dumpSteamStatus::DUMP == mBAStream_status){
				if(mVbas->writeAfter(pkt) < 0){
					mBAStream_status = dumpSteamStatus::START;
					mVbas->setWaitExitCond();
				}				
			}
		}
		if (pkt.stream_index == asIdx) {
			s_inf("audio stream, packet size: %d", pkt.size);
		}		
		av_packet_unref(&pkt);
	}
end:
	avformat_close_input(&ifmt_ctx);
	avformat_free_context(ifmt_ctx);
	av_dict_free(&avdic);
	s_war("%s thread exit done!!!",__func__);
	return 0;
}
int RtspClient::curlPushUp(curlPushCliPar* curlPar){
	auto pushCli = new curlPushCli(curlPar);
	if(!pushCli){
		s_err("new curlPushCli failed!");
		return -1;
	}
	int res = pushCli->pushPerform();
	if(res){
		s_err("pushCli->pushPerform failed!");
		delete pushCli;
		return -2;
	}
	delete pushCli;
	return 0;
}

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
static RtspClient* g_RtspClientPtr = NULL;
static void sigHandle(int signo){
	switch(signo){
	case SIGUSR1:
		s_war("setBAStreamStatus DUMP.");
		g_RtspClientPtr->setBAStreamStatus();
		break;
	default:
		break;
	}
}
#include <getopt.h>
int main(int argc,char *argv[]){
	RtspCliPar RCPara={0};
	int opt = 0;
	while ((opt = getopt_long_only(argc, argv,"u:b:i:o:l:p:h",NULL,NULL)) != -1) {
		switch (opt) {
		case 'l':
			lzUtils_logInit(optarg,NULL);
			break;
		case 'p':
			lzUtils_logInit(NULL,optarg);
			break;
		case 'b':
			RCPara.bakPath = optarg;
			break;	
		case 'i':
			RCPara.iPath = optarg;
			break;
		case 'o':
			RCPara.oPath = optarg;
			break;
		case 'u':
			RCPara.userpwd = optarg;
			break;
		default: /* '?' */
			return help_info(argc ,argv);
	   }
	}
	s_inf("RCPara.iPath:%s",RCPara.iPath);
	s_inf("RCPara.oPath:%s",RCPara.oPath);
	s_inf("RCPara.bakPath=%s",RCPara.bakPath);	
	signal(SIGUSR1,&sigHandle);
	auto mCli = new RtspClient(&RCPara);
	if(!mCli){
		return -1;
	}
	g_RtspClientPtr = mCli;
	for(;;){
		pause();
	}
	return 0;
}