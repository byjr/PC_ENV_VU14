#include <iostream>
#include <thread>
extern "C" {
	#include <libavformat/avformat.h>
}
#include <lzUtils/base.h>
#include "steamHandle.h"
#define FRAMES_PER_10SEC 250
class RtspCliParam{
public:
	char *iPath;
	char *oPath;
};
class RtspClient{
	RtspCliParam *mParam;
	std::thread mPullTread;
public:
	RtspClient(RtspCliParam * para);
	int pullSteamLoop();
	int mediaPackageUp(BAStream *vbas);
	~RtspClient();
};
RtspClient::RtspClient(RtspCliParam* para){
	mParam = para;
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
enum class dumpSteamStatus{
	START,
	CYCW,
	DUMP,
	DONE,
};
int RtspClient::mediaPackageUp(BAStream * vbs){
	//从vbs 读出数据打包成MP4文件
	//将MP4 上传至FTP服务器
	return 0;
}
int RtspClient::pullSteamLoop(){
    //使用TCP连接打开RTSP，设置最大延迟时间
    AVDictionary *avdic=NULL;  
    char option_key[]="rtsp_transport";  
    char option_value[]="tcp";  
    av_dict_set(&avdic,option_key,option_value,0);  
    char option_key2[]="max_delay";  
    char option_value2[]="5000000";  
    av_dict_set(&avdic,option_key2,option_value2,0); 

	// Allocate an AVFormatContext
	AVFormatContext* format_ctx = avformat_alloc_context();
 
	// open rtsp: Open an input stream and read the header. The codecs are not opened
	const char* url =mParam->iPath;
	int ret = -1;
	ret = avformat_open_input(&format_ctx, url, NULL, &avdic);
	if (ret != 0) {
		s_err("fail to open url: %s, return value: %d\n", url, ret);
		return -1;
	}
 
	// Read packets of a media file to get stream information
	ret = avformat_find_stream_info(format_ctx, NULL);
	if ( ret < 0) {
		s_err("fail to get stream information: %d\n", ret);
		return -1;
	}
 
	// audio/video stream index
	int video_stream_index = -1;
	int audio_stream_index = -1;
	s_inf("Number of elements in AVFormatContext.streams: %d\n", format_ctx->nb_streams);
	for (int i = 0; i < format_ctx->nb_streams; ++i) {
		const AVStream* stream = format_ctx->streams[i];
		s_inf("type of the encoded data: %d\n", stream->codecpar->codec_id);
		if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			video_stream_index = i;
			s_inf("dimensions of the video frame in pixels: width: %d, height: %d, pixel format: %d\n",
				stream->codecpar->width, stream->codecpar->height, stream->codecpar->format);
		} else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			audio_stream_index = i;
			s_inf("audio sample format: %d\n", stream->codecpar->format);
		}
	}
 
	if (video_stream_index == -1) {
		s_err("no video stream\n");
		return -1;
	}
 
	if (audio_stream_index == -1) {
		s_err("no audio stream\n");
	}
 
	int cnt = 0;
	AVPacket pkt;
	FILE *ofp = fopen(mParam->oPath,"w+");
	if(!ofp){
		s_err("fopen %s failed!",mParam->oPath);
		return -1;
	}
	dumpSteamStatus BAStream_write_status = dumpSteamStatus::START;
	BAStream* mVbas = NULL;
	for(;;) {
		ret = av_read_frame(format_ctx, &pkt);		
		if (ret == AVERROR(EAGAIN)) {
			 show_errno(0,"av_read_frame");		
			continue;
		}else if(ret == AVERROR_EOF ) { 
			s_inf("AVERROR_EOF read done!!!");
			break;
		}
		
		if (pkt.stream_index == video_stream_index) {
			s_inf("video stream, packet  : %d\n", pkt.size);
			if(dumpSteamStatus::START == BAStream_write_status){
				mVbas = new BAStream(FRAMES_PER_10SEC);
				if(!mVbas){
					s_err("new BAStream failed!");
					continue;
				}
				BAStream_write_status = dumpSteamStatus::CYCW;
			}
			if(dumpSteamStatus::CYCW == BAStream_write_status){
				mVbas->writeBefor((char *)pkt.data,pkt.size);
			}else if(dumpSteamStatus::DUMP == BAStream_write_status){
				if(anyStatus::DONE == mVbas->writeAfter((char *)pkt.data,pkt.size)){
					BAStream_write_status = dumpSteamStatus::DONE;
					//启动打包和上传的线程
					std::thread m_trd = std::thread([this,mVbas]() {
						mediaPackageUp(mVbas);
					});
					m_trd.detach();
				}
			}			
		}
 
		if (pkt.stream_index == audio_stream_index) {
			s_inf("audio stream, packet size: %d\n", pkt.size);
		}		
		av_packet_unref(&pkt);
	}
	fclose(ofp);
	avformat_free_context(format_ctx);
	return 0;
}
int help_info(int argc ,char *argv[]){
	printf("%s help:\n",get_last_name(argv[0]));
	printf("\t-i [input path]\n");
	printf("\t-o [output path]\n");
	printf("\t-l [logLvCtrl]\n");
	printf("\t-p [logPath]\n");
	printf("\t-h show help\n");
	return 0;
}
#include <getopt.h>
int main(int argc,char *argv[]){
	RtspCliParam RCPara={0};
	int opt = 0;
	while ((opt = getopt_long_only(argc, argv,"i:o:l:p:h",NULL,NULL)) != -1) {
		switch (opt) {
		case 'l':
			lzUtils_logInit(optarg,NULL);
			break;
		case 'p':
			lzUtils_logInit(NULL,optarg);
			break;
		case 'i':
			RCPara.iPath = optarg;
			break;
		case 'o':
			RCPara.oPath = optarg;
			break;			
		default: /* '?' */
			return help_info(argc ,argv);
	   }
	}
	auto mCli = new RtspClient(&RCPara);
	if(!mCli){
		return -1;
	}
	return 0;
}