#include "ffapp.h"
#include <libavutil/timestamp.h>
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
	#define TIME_LAY "20191205160355"
	char buf[]=TIME_LAY;
	struct timespec ts={0};
	struct tm* timeinfo;
	px_gettime(CLOCK_REALTIME,&ts);
	timeinfo=localtime(&ts.tv_sec);
	strftime(buf,sizeof(TIME_LAY),"%Y%m%d%H%M%S",timeinfo);
	return buf;
}

int RtspClient::pullSteamLoop(){
    //使用TCP连接打开RTSP，设置最大延迟时间
    char errbuf[512]={0};
    AVDictionary *avdic=NULL;  
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
			s_inf("The video frame in pixels: width: %d, height: %d, pixel format: %d",
				in_stream->codecpar->width, in_stream->codecpar->height, in_stream->codecpar->format);
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
				mVbas = new BAStream(10 * mParam->fps);
				if(!mVbas){
					s_err("new BAStream failed!");
					continue;
				}
				mBAStream_status = dumpSteamStatus::CYCW;
			}
			if(dumpSteamStatus::CYCW == mBAStream_status){
				s_dbg("pPkt->pts=%u",pkt.pts);
				mVbas->writeBefor(&pkt);
				if(mParam->bakPath){
					if(fp){
						fwrite(pkt.data,1,pkt.size,fp);
					}				
				}			
			}else if(dumpSteamStatus::GETI == mBAStream_status){
				if(pkt.flags & AV_PKT_FLAG_KEY){
					mBAStream_status = dumpSteamStatus::DUMP;
				}
				mVbas->writeBefor(&pkt);
				if(mParam->bakPath){
					if(fp){
						fwrite(pkt.data,1,pkt.size,fp);
					}				
				}				
			}else if(dumpSteamStatus::DUMP == mBAStream_status){
				s_dbg("pPkt->pts=%u",pkt.pts);
				if(anyStatus::DONE == mVbas->writeAfter(&pkt)){
					mBAStream_status = dumpSteamStatus::START;
					//启动打包和上传的线程					
					std::string oPath = getTimeStemp();
//					std::string oPath = "cut264";
					oPath += mParam->oSufix;					
					auto packPar = new PackPar();		
					packPar->oPath = oPath;
					packPar->ifmt_ctx = ifmt_ctx;
					packPar->vBAs = mVbas;
					std::thread m_trd = std::thread([this,packPar]() {
						mediaPackageUp(packPar);
					});
					m_trd.detach();
					s_war("package %s start...",oPath.data());
					while(1){sleep(1);}
				}
			}			
		}
		if (pkt.stream_index == asIdx) {
			s_inf("audio stream, packet size: %d", pkt.size);
		}		
		av_packet_unref(&pkt);
	}
end:
	avformat_free_context(ifmt_ctx);
	av_dict_free(&avdic);
	return 0;
}
int RtspClient::mediaPackageUp(PackPar* packPar){
	int ret = -1 ;
	bool getOnePkt = false;
	AVStream* in_stream = NULL;
	AVStream* out_stream = NULL;
	AVFormatContext* ofmt_ctx = NULL;	
	AVFormatContext* ifmt_ctx = packPar->ifmt_ctx;	
	AVPacket *pPkt;
	char errbuf[512]={0};
	AVRational encTimebase={1,30};
	AVRational stTimebase={1,30};
	static size_t next_pts = 0;
	#define ff_STREAM_DURATION (10.0)	
	avformat_alloc_output_context2(&ofmt_ctx,NULL,NULL,packPar->oPath.data());
	if(!ofmt_ctx){
		s_err("avformat_alloc_output_context2 failed,oPath:%s",packPar->oPath.data());
		return -1;
	}
	AVOutputFormat *ofmt = ofmt_ctx->oformat;
	for(int i=0;i<ifmt_ctx->nb_streams;i++){//根据输入流创建输出流
		in_stream = ifmt_ctx->streams[i];
		AVCodec *codec = avcodec_find_decoder(in_stream->codecpar->codec_id);
		out_stream = avformat_new_stream(ofmt_ctx,codec);
		if(!out_stream) {
			av_strerror(ret,errbuf,sizeof(errbuf));
			s_err("avformat_new_stream error:%s",errbuf);	
			goto exit;
		}
		AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
		ret = avcodec_parameters_to_context(codec_ctx, in_stream->codecpar);
		if (ret < 0){
			s_err("avcodec_parameters_to_context error:%s",av_err2strc(ret));
			goto exit;
		}
		codec_ctx->codec_tag = 0;
		if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER){
			codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		}
		ret = avcodec_parameters_from_context(out_stream->codecpar, codec_ctx);
		if (ret < 0){
			s_err("avcodec_parameters_from_context error:%s",av_err2strc(ret));
			goto exit;
		}
	}	
	av_dump_format(ofmt_ctx,0,packPar->oPath.data(),1);
    if(!(ofmt->flags & AVFMT_NOFILE)){
        ret = avio_open(&ofmt_ctx->pb,packPar->oPath.data(),AVIO_FLAG_WRITE);
        if(ret<0){
            s_err("Could not open output URL '%s',error:%s",packPar->oPath,av_err2strc(ret));
			goto exit;
        }
    }
	packPar->vBAs->adjustTs();
    ret = avformat_write_header(ofmt_ctx,NULL);
    if(ret < 0){
		av_strerror(ret,errbuf,sizeof(errbuf));
		s_err("avformat_write_header error:%s",errbuf);
        goto exit;
    }
	for(;;){  	
		getOnePkt = packPar->vBAs->readOne(&pPkt);
		if(!getOnePkt){
			s_war("packPar->vBAs->readOne done");
			break;
		}
//		s_inf("pPkt->pts=%u,pPkt->dts=%u",pPkt->pts,pPkt->dts);
//		  av_packet_rescale_ts(pPkt, encTimebase, stTimebase);
//		  log_packet(ofmt_ctx, pPkt);
		ret = av_interleaved_write_frame(ofmt_ctx,pPkt);
		if(ret < 0){
			s_err("av_interleaved_write_frame error[%d]:%s",ret,av_err2strc(ret));
		}
		delete []pPkt->data;
		delete pPkt;
    }
	av_write_trailer(ofmt_ctx);
	s_inf("av_write_trailer dne");
	exit(0);
	//将MP4 上传至FTP服务器
		//...
exit:
	delete packPar;
	avformat_close_input(&ifmt_ctx);
    //Close input
    if(ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE)){
		avio_close(ofmt_ctx->pb);
	}
    avformat_free_context(ofmt_ctx);
    if(ret<0 && ret != AVERROR_EOF)   {
		av_strerror(ret,errbuf,sizeof(errbuf));
		s_war("avformat_free_context error:%s",errbuf);
        return -1;
    }	
	return 0;
}
int help_info(int argc ,char *argv[]){
	s_err("%s help:",get_last_name(argv[0]));
	s_err("\t-s [input fps]");
	s_err("\t-i [input path]");
	s_err("\t-o [output path]");
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
	while ((opt = getopt_long_only(argc, argv,"b:s:i:o:l:p:h",NULL,NULL)) != -1) {
		switch (opt) {
		case 'l':
			lzUtils_logInit(optarg,NULL);
			break;
		case 'p':
			lzUtils_logInit(NULL,optarg);
			break;
		case 's':
			RCPara.fps = atoi(optarg);
			break;
		case 'b':
			RCPara.bakPath = optarg;
			break;	
		case 'i':
			RCPara.iPath = optarg;
			break;
		case 'o':
			RCPara.oSufix = optarg;
			break;			
		default: /* '?' */
			return help_info(argc ,argv);
	   }
	}
	s_inf("RCPara.iPath:%s",RCPara.iPath);
	s_inf("RCPara.fps=%d",RCPara.fps);
	s_inf("ENOMEM:%d",AVERROR(ENOMEM))
	signal(SIGUSR1,&sigHandle);
	auto mCli = new RtspClient(&RCPara);
	if(!mCli){
		return -1;
	}
	g_RtspClientPtr = mCli;
	for(;;){
		sleep(1);
	}
	return 0;
}