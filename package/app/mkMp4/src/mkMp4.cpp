/**
实现yuv和aac合成mp4
Yuv是176x144的，pcm是16bit，44100HZ，双声道的
*/
#include <stdio.h>
#include <stdint.h>
#include <lzUtils/base.h>
#include <getopt.h>
#include <unistd.h>
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
};
#define STREAM_FRAME_RATE  25
#define STREAM_PIX_FMT AV_PIX_FMT_YUV420P 
const int width = 176, height = 144;
const char* input_pcm_file = "in.pcm";
const char* input_yuv_file = "in.yuv";
void add_stream(AVFormatContext *out_format_context, AVStream** st, AVCodecContext **out_codec_context, AVCodec** codec, AVCodecID codec_id)
{
    *codec = avcodec_find_encoder(codec_id);
    if (!codec) {
        fprintf(stderr, "Could not find encoder for '%s'\n",
            avcodec_get_name(codec_id));
        exit(1);
    }

    *st = avformat_new_stream(out_format_context,*codec);
    if (!*st) {
        fprintf(stderr, "Could not alloc stream");
        exit(1);
    }
    *out_codec_context = (*st)->codec;
    (*out_codec_context)->codec_id = codec_id;
    switch ((*codec)->type) {
    case AVMEDIA_TYPE_AUDIO:

        (*out_codec_context)->codec_type = AVMEDIA_TYPE_AUDIO;
        (*out_codec_context)->channel_layout = AV_CH_LAYOUT_STEREO;
        (*out_codec_context)->channels = av_get_channel_layout_nb_channels((*out_codec_context)->channel_layout);
        //samples per second
        (*out_codec_context)->sample_rate = 44100;
        (*out_codec_context)->sample_fmt = (*codec)->sample_fmts ?
            (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
        (*out_codec_context)->bit_rate = 64000;

        /** Allow the use of the experimental AAC encoder */
        (*out_codec_context)->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

        /** Set the sample rate for the container. */
        (*st)->time_base.den = (*out_codec_context)->sample_rate;
        (*st)->time_base.num = 1;
        break;
    case AVMEDIA_TYPE_VIDEO:
        (*out_codec_context)->codec_type = AVMEDIA_TYPE_VIDEO;

        (*out_codec_context)->bit_rate = 400000;

        (*out_codec_context)->width = width;
        (*out_codec_context)->height = height;

        (*out_codec_context)->time_base.den = STREAM_FRAME_RATE;
        (*out_codec_context)->time_base.num = 1;
        (*out_codec_context)->gop_size = 12;
        (*out_codec_context)->pix_fmt = STREAM_PIX_FMT;

        (*st)->time_base.den = 90000;
        (*st)->time_base.num = 1;

        (*out_codec_context)->qmin = 10;
        (*out_codec_context)->qmax = 51;
        (*out_codec_context)->qcompress = 0.6f;

        (*out_codec_context)->max_b_frames = 0;
        break;
    default:
        break;
    }


    // some formats want stream headers to be separate
    if (out_format_context->oformat->flags & AVFMT_GLOBALHEADER)
        (*out_codec_context)->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;


}


void open_video(AVCodecContext* codec_context,AVCodec* codec){


    AVDictionary *param = NULL;
    //H.264
    if (codec_context->codec_id == AV_CODEC_ID_H264) {
        av_dict_set(&param, "preset", "fast", 0);
        av_dict_set(&param, "tune", "zerolatency", 0);

    }
    /* open the codec */
    if (avcodec_open2(codec_context, codec, &param) < 0) {
        fprintf(stderr, "could not open codec\n");
        exit(1);
    }
}

void open_audio(AVCodecContext* audio_codec_context,AVCodec * codec){

    if (avcodec_open2(audio_codec_context, codec, NULL) < 0) {
        printf("Could not open audio codec \n");
         exit(1);
    }
}
void init_packet(AVPacket *packet){
    av_init_packet(packet);
    packet->data = NULL;
    packet->size = 0;
}
int framecount=0;
int64_t audio_last_pts, video_last_pts;
int encode_video_frame(AVFrame *frame,
    AVFormatContext *out_format_context,
    AVStream *video_st){
    AVCodecContext* out_codec_context = video_st->codec;
    int got_packet;
    AVPacket enc_pkt;
    init_packet(&enc_pkt);
    int ret = avcodec_encode_video2(out_codec_context, &enc_pkt,
        frame, &got_packet);

    if (ret < 0 || !got_packet){ //在flush的时候，如果失败 ，说明丢失帧（缓存帧）已经空了
        av_free_packet(&enc_pkt);
        av_frame_free(&frame);
        return 1;
    }

    if (enc_pkt.pts == AV_NOPTS_VALUE){
        enc_pkt.duration = av_rescale_q(1, out_codec_context->time_base, video_st->time_base);
        enc_pkt.pts = framecount*enc_pkt.duration;
        enc_pkt.dts = enc_pkt.pts;
        video_last_pts= enc_pkt.pts;

    }
    enc_pkt.stream_index = video_st->index;
    ret = av_interleaved_write_frame(out_format_context, &enc_pkt);
    av_free_packet(&enc_pkt);
    av_frame_free(&frame);
    if (ret < 0){
        return 1;
    }else{
        framecount++;
    }

    return 0;
}

int  write_video_frame(AVFormatContext* out_format_context, AVCodecContext *out_codec_context, AVStream *video_st, FILE** fp){
    uint8_t *buffer = new uint8_t[avpicture_get_size(out_codec_context->pix_fmt, out_codec_context->width, out_codec_context->height)];

    int ret = fread(buffer, out_codec_context->width * out_codec_context->height * 3 / 2, 1, *fp);
    if (ret == 0){
        return 1;
    }
    AVFrame* yuvFrame = av_frame_alloc();
    avpicture_fill((AVPicture *)yuvFrame, buffer, out_codec_context->pix_fmt, out_codec_context->width, out_codec_context->height);
    encode_video_frame(yuvFrame, out_format_context, video_st);


    return 0;
}

int encode_audio_frame(AVFrame *frame, int nbsamples,
    AVFormatContext *output_format_context,
    AVCodecContext *output_codec_context,AVStream* st){
    int got_packet;
    AVPacket enc_pkt;
    init_packet(&enc_pkt);

    int ret = avcodec_encode_audio2(output_codec_context, &enc_pkt,
        frame, &got_packet);

    if (ret < 0 || !got_packet){
        av_free_packet(&enc_pkt);
        av_frame_free(&frame);
        return 1;
    }

    /** Set a timestamp based on the sample rate for the container. */
    if (enc_pkt.pts == AV_NOPTS_VALUE){
        audio_last_pts += nbsamples;
        enc_pkt.pts = audio_last_pts;
        enc_pkt.dts = enc_pkt.pts;

        double duration_s = output_codec_context->frame_size*(1 / output_codec_context->sample_rate);
        enc_pkt.duration = duration_s / av_q2d(st->time_base);


    }
    enc_pkt.stream_index = st->index;
    ret = av_interleaved_write_frame(output_format_context, &enc_pkt);
    av_free_packet(&enc_pkt);
    av_frame_free(&frame);
    if (ret < 0){
        return 1;
    }

    return 0;
}

int write_audio_frame(AVFormatContext* out_format_context, AVCodecContext *out_codec_context,AVStream* st,FILE** fp){
    AVFrame* pFrame = av_frame_alloc();
    pFrame->nb_samples = out_codec_context->frame_size;
    pFrame->format = out_codec_context->sample_fmt;
    int size = av_samples_get_buffer_size(NULL, out_codec_context->channels, out_codec_context->frame_size, out_codec_context->sample_fmt, 1);
    uint8_t * frame_buf = (uint8_t *)av_malloc(size);
    int ret=fread(frame_buf, 1, size, *fp);
    if (ret == 0){
        return 1;
    }
    avcodec_fill_audio_frame(pFrame, out_codec_context->channels, out_codec_context->sample_fmt, (const uint8_t*)frame_buf, size, 1);

    encode_audio_frame(pFrame, out_codec_context->frame_size, out_format_context, out_codec_context,st);

    return 0;
}


int main_handle(int argc, char **argv){

    av_register_all();
    const char *out_file = "output.mp4";
    AVOutputFormat* fmt = av_guess_format(NULL, out_file, NULL);
    if (!fmt) {
        fprintf(stderr, "Could not find suitable output format");
        exit(1);
    }
    AVFormatContext* out_format_context = avformat_alloc_context();
    out_format_context->oformat = fmt;
    AVCodecContext *audio_codec_context = NULL, *video_codec_context = NULL;
    AVCodec* video_codec=NULL, *audio_codec=NULL;
    AVStream *video_st=NULL,*audio_st=NULL;
    int have_video = 0, have_audio = 0;

    if (fmt->video_codec != AV_CODEC_ID_NONE) {
        add_stream(out_format_context, &video_st, &video_codec_context, &video_codec, fmt->video_codec);
        have_video = 1;

    }
    if (fmt->audio_codec != AV_CODEC_ID_NONE) {
        add_stream(out_format_context, &audio_st, &audio_codec_context, &audio_codec, fmt->audio_codec);
        have_audio = 1;

    }

    if (have_video)
        open_video(video_codec_context,video_codec);

    if (have_audio)
        open_audio(audio_codec_context,audio_codec);



    int ret = 0;
    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&out_format_context->pb, out_file, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open '%s': %s\n", out_file,
                "");
            return 1;
        }
    }

    ret = avformat_write_header(out_format_context, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file: %s\n","");
        return 1;
    }
    FILE* yuvfp = fopen(input_yuv_file, "rb");
    FILE* pcmfp = fopen(input_pcm_file, "rb");
    int encode_video = 1, encode_audio = 1;
    while (encode_video && encode_audio) {
        //printf("video_last_pts:%lld, \t timebase:%f  timestamp:\t%f\n", video_last_pts, av_q2d(video_st->time_base), video_last_pts*av_q2d(video_st->time_base));
        //printf("audio_last_pts:%lld, \t timebase:%f  timestamp:\t%f\n", audio_last_pts, av_q2d(audio_st->time_base), audio_last_pts*av_q2d(audio_st->time_base));

        //< -1,> 1,= 0
        if (av_compare_ts(video_last_pts, video_st->time_base,
            audio_last_pts, audio_st->time_base) <= 0) {
            if (encode_video){
                encode_video = !write_video_frame(out_format_context, video_codec_context, video_st, &yuvfp);

            }

        }else {
            if (encode_audio){
                encode_audio=!write_audio_frame(out_format_context, audio_codec_context, audio_st, &pcmfp);

            }

        }
    }
    if (video_codec_context->codec->capabilities &AV_CODEC_CAP_DELAY){
        while (!encode_video_frame(NULL, out_format_context, video_st)){ ; }
    }
    if (audio_codec_context->codec->capabilities &AV_CODEC_CAP_DELAY){
        while (!encode_audio_frame(NULL, audio_codec_context->frame_size, out_format_context,audio_codec_context, audio_st)){;}
    }


    av_write_trailer(out_format_context);
    if (video_codec_context)
        avcodec_close(video_codec_context);
    if (audio_codec_context)
        avcodec_close(audio_codec_context);
    if (out_format_context) {
        avio_closep(&out_format_context->pb);
        avformat_free_context(out_format_context);
    }
    fclose(yuvfp);
    fclose(pcmfp);
    system("pause");
    return 0;
}

int help_info(int argc, char *argv[]){
	printf("%s help:\n",get_last_name(argv[0]));
	printf("\t-i [input device]\n");
	printf("\t-o [output device]\n");
	printf("\t-l [logLvCtrl]\n");
	printf("\t-p [logPath]\n");
	printf("\t-h show help\n");
	printf("\t-h default idev:plughw:0,3\n");
	printf("\t-h default odev:plughw:0,2\n");
	return 0;	
}

int main(int argc, char *argv[]){
	int opt = 0;
	char * recDdevide=NULL,* plyDdevide=NULL;
	while ((opt = getopt_long_only(argc, argv, "i:o:l:p:dh",NULL,NULL)) != -1) {
		switch (opt) {
		case 'l':
			lzUtils_logInit(optarg,NULL);
			break;
		case 'p':
			lzUtils_logInit(NULL,optarg);
			break;
		case 'i':
			recDdevide=optarg;
			break;
		case 'o':
			plyDdevide=optarg;
			break;
		default: /* '?' */
			return help_info(argc ,argv);
	   }
	}	
//打印编译时间
	showCompileTmie(argv[0],s_war);
	main_handle(argc,argv);
	for(;;){
		sleep(1);
	}
	return -1;
}

