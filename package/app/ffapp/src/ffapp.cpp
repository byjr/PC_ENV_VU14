#include <iostream>
extern "C" {
	#include <libavformat/avformat.h>
}
#define s_inf(x...) printf("INF [%s %d]:",__func__,__LINE__);printf(x);printf("\n");
#define s_err(x...) printf("ERR [%s %d]:",__func__,__LINE__);printf(x);printf("\n");
class clientExParam{
public:
	char *iPath;
	char *oPath;
};
int rtsp_client(clientExParam * para)
{
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
	const char* url = para->iPath;
	int ret = -1;
	ret = avformat_open_input(&format_ctx, url, NULL, &avdic);
	if (ret != 0) {
		fprintf(stderr, "fail to open url: %s, return value: %d\n", url, ret);
		return -1;
	}
 
	// Read packets of a media file to get stream information
	ret = avformat_find_stream_info(format_ctx, NULL);
	if ( ret < 0) {
		fprintf(stderr, "fail to get stream information: %d\n", ret);
		return -1;
	}
 
	// audio/video stream index
	int video_stream_index = -1;
	int audio_stream_index = -1;
	fprintf(stdout, "Number of elements in AVFormatContext.streams: %d\n", format_ctx->nb_streams);
	for (int i = 0; i < format_ctx->nb_streams; ++i) {
		const AVStream* stream = format_ctx->streams[i];
		fprintf(stdout, "type of the encoded data: %d\n", stream->codecpar->codec_id);
		if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			video_stream_index = i;
			fprintf(stdout, "dimensions of the video frame in pixels: width: %d, height: %d, pixel format: %d\n",
				stream->codecpar->width, stream->codecpar->height, stream->codecpar->format);
		} else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			audio_stream_index = i;
			fprintf(stdout, "audio sample format: %d\n", stream->codecpar->format);
		}
	}
 
	if (video_stream_index == -1) {
		fprintf(stderr, "no video stream\n");
		return -1;
	}
 
	if (audio_stream_index == -1) {
		fprintf(stderr, "no audio stream\n");
	}
 
	int cnt = 0;
	AVPacket pkt;
	FILE *ofp = fopen(para->oPath,"w+");
	if(!ofp){
		s_err("fopen %s failed!",para->oPath);
		return -1;
	}
	for(;;) {
		ret = av_read_frame(format_ctx, &pkt);		
		if (ret == AVERROR(EAGAIN)) {
			perror("av_read_frame");		
			continue;
		}else if(ret == AVERROR_EOF ) {
			s_inf("AVERROR_EOF read done!!!");
			break;
		}
 
		if (pkt.stream_index == video_stream_index) {
			fprintf(stdout, "video stream, packet size: %d\n", pkt.size);
			int res = fwrite(pkt.data,1,pkt.size,ofp);
			if(res < 0){
				s_err("fwrite failed:%d",res);
			}
		}
 
		if (pkt.stream_index == audio_stream_index) {
			fprintf(stdout, "audio stream, packet size: %d\n", pkt.size);
		}
		
		av_packet_unref(&pkt);
	}
	fclose(ofp);
	avformat_free_context(format_ctx);
	return 0;
}
int help_info(int argc ,char *argv[]){
	printf("%s help:\n",argv[0]);
	printf("\t-i [share path]\n");
	printf("\t-o [share path]\n");
	printf("\t-h show help\n");
	return 0;
}
#include <getopt.h>
int main(int argc,char *argv[]){
	clientExParam cliPara={0};
	int opt = 0;
	while ((opt = getopt_long_only(argc, argv,"i:o:h",NULL,NULL)) != -1) {
		switch (opt) {
		case 'i':
			cliPara.iPath = optarg;
			break;
		case 'o':
			cliPara.oPath = optarg;
			break;			
		default: /* '?' */
			return help_info(argc ,argv);
	   }
	}
	return rtsp_client(&cliPara);
}