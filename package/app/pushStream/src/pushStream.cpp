/*
 * 学习ffmpeg使用例子.
 * 如有不足之处，请指出。在此谢谢各位
 * 源码链接：http://ffmpeg.org/doxygen/2.8/index.html
 * ffmpeg使用版本 3.3.2
 */
extern "C"{
	#include <stdio.h>
	#include <stdlib.h>
	#include "libavformat/avformat.h"
	#include "libavutil/mathematics.h"
	#include "libavutil/time.h"
	#include "libavcodec/avcodec.h"
}
#include <lzUtils/base.h>
#include <getopt.h>
//输入对应的format context
static AVFormatContext *input_format_context = NULL;
//输出对应的format context
static AVFormatContext *output_format_context = NULL;

//输出流
static AVStream *input_stream = NULL;
//输出流
static AVStream *output_stream = NULL;

//包
static AVPacket pkt;


const char *input_file = "jc.mp4"; //带路径的视频文件(绝对路径或相对路径，本例子使用相对路径，1.flv表示与c文件同一目录下)

const char *output_file = "rtmp://localhost:1935/live/hello";//输出 URL（Output URL）[RTMP], 例子使用red5作为流媒体服务器


static int video_index = 0;
static int frame_index = 0;


void init_register_and_network_for_ffmpeg()
{
    // 第一步av_register_all();
    // 注册复用器，编码器等。注册所有的muxers、demuxers和protocols
    av_register_all();

    //Network,Do global initialization of network components.
    //初始化网络容器。
    avformat_network_init();
}

/*
 * 创建输入流
 */
AVStream *create_input_avstream()
{
    int ret; // ret保存函数调用结果

    // 第一步av_register_all();
    // 注册复用器，编码器等。注册所有的muxers、demuxers和protocols
    //av_register_all();

    //利用ffmpeg默认方法,初始化context。此步骤可以忽略，input_format_context可以为NULL
    //第二步中avformat_open_input()函数中有判断，若为NULL自动调用此函数。
    input_format_context = avformat_alloc_context();
    //第二步,avformat_open_input();
    //打开多媒体数据并且获得一些相关的信息
    if ((ret = avformat_open_input(&input_format_context, input_file, 0, 0)) < 0)
    {
        s_war("Can't open input file.");
        return NULL;
    }

    //第三步,avformat_find_stream_info();
    //读取一部分视音频数据并且获得一些相关的信息
    if ((ret = avformat_find_stream_info(input_format_context, 0)) < 0) {
        s_war( "Failed to retrieve input stream information");
        return NULL;
    }

    //初始化完成后，AVFormatContext中存放有AVStream的引用
    //通过AVStream中存放的AVCodecContext引用获取 （enum AVMediaType）枚举中视频对应的值。
    //   ***enum AVMediaType
    //AVMEDIA_TYPE_UNKNOWN,AVMEDIA_TYPE_VIDEO,AVMEDIA_TYPE_AUDIO,AVMEDIA_TYPE_DATA,AVMEDIA_TYPE_SUBTITLE,AVMEDIA_TYPE_ATTACHMENT,AVMEDIA_TYPE_NB
    //例：首先通过循环遍历获取视频
	int i=0;
    for (i = 0; i < input_format_context->nb_streams; i++) //nb_streams为stream数量
    {
        //如果stream的codec为AVCodecContext变量，AVCodecContext变量中的codec为AVMediaType枚举值
        if (input_format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            input_stream = input_format_context->streams[i];
            video_index = i;
            break;
        }
    }

    //Print detailed information about the input or output format, such as duration, bitrate, streams, container, programs, metadata, side data, codec and time base
    //打印输入输出format的详细信息，该函数可选调用。
    av_dump_format(input_format_context, 0, input_file, 0);

    return input_stream;
}


/*
 * 创建输出流
 */
AVStream *create_output_avstream()
{
    int ret;

    // 第一步av_register_all();
    // 注册复用器，编码器等。注册所有的muxers、demuxers和protocols
    //av_register_all();

    // 第二步avformat_alloc_output_context2();
    //基于FFmpeg的视音频编码器程序中，该函数通常是第一个调用的函数（除了组件注册函数av_register_all()）
    // return >= 0 in case of success, 返回值大于等于0表示成功
    avformat_alloc_output_context2(&output_format_context, NULL, "mp4", output_file);

    if (!output_format_context) {
        s_war( "Could not create output context");
        ret = AVERROR_UNKNOWN;
        return NULL;
    }

    //根据输入流创建输出流（Create output AVStream according to input AVStream）
    AVStream *output_stream = avformat_new_stream(output_format_context, input_stream->codec->codec);

    if (!output_stream) {
        s_war( "Failed allocating output stream");
        ret = AVERROR_UNKNOWN;
        return NULL;
    }

    //复制AVCodecContext的设置（Copy the settings of AVCodecContext）
    ret = avcodec_copy_context(output_stream->codec, input_stream->codec);

    if (ret < 0)
    {
        s_war( "Failed to copy context from input to output stream codec context");
        return NULL;
    }

    output_stream->codec->codec_tag = 0;

    if (output_format_context->oformat->flags & AVFMT_GLOBALHEADER)
        output_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

    //Print detailed information about the input or output format, such as duration, bitrate, streams, container, programs, metadata, side data, codec and time base
    //打印输入输出format的详细信息，该函数可选调用。
    av_dump_format(output_format_context, 0, output_file, 1);

    return output_stream;
}

int open_url_and_write()
{
    int ret;
    int64_t start_time = 0; // 开始时间

    //打开输出URL（Open output URL）
    if (!(output_format_context->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open2(&output_format_context->pb, output_file, AVIO_FLAG_WRITE, NULL, NULL); //根据源代码，调用avio_open同效,avio_open中调用avio_open2。
        //ret = avio_open(&output_format_context->pb, output_file, AVIO_FLAG_WRITE);
        if (ret < 0) {
            s_war( "Could not open output URL '%s'", output_file);
            return -1;
        }
    }

    //写文件头（Write file header
    //0 on success, negative AVERROR on failure.
    ret = avformat_write_header(output_format_context, NULL);

    if (ret < 0) {
        s_war( "Error occurred when opening output URL");
        return -1;
    }

    start_time = av_gettime();

    while (1) {
        AVStream *in_stream, *out_stream;
        //获取一个AVPacket（Get an AVPacket）
        ret = av_read_frame(input_format_context, &pkt);
        if (ret < 0)
            break;
        //FIX：No PTS (Example: Raw H.264)
        //Simple Write PTS
        if(pkt.pts == AV_NOPTS_VALUE)
        {
            //Write PTS
            //AVRational time_base1=input_format_context->streams[video_index]->time_base;
            AVRational time_base1 = input_format_context->streams[video_index]->time_base;

            //Duration between 2 frames (us)
            int64_t calc_duration = (double)AV_TIME_BASE/av_q2d(input_format_context->streams[video_index]->r_frame_rate);
            //Parameters
            pkt.pts = (double)(frame_index*calc_duration) / (double)(av_q2d(time_base1)*AV_TIME_BASE);
            pkt.dts = pkt.pts;
            pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1)*AV_TIME_BASE);
        }
        //Important:Delay
        if(pkt.stream_index == video_index)
        {
            AVRational time_base = input_format_context->streams[video_index]->time_base;
            AVRational time_base_q = {1, AV_TIME_BASE};
            int64_t pts_time = av_rescale_q(pkt.dts, time_base, time_base_q);
            int64_t now_time = av_gettime() - start_time;
            if (pts_time > now_time)
                av_usleep(pts_time - now_time);
        }
        in_stream  = input_format_context->streams[pkt.stream_index];
        out_stream = output_format_context->streams[pkt.stream_index];
        /* copy packet */
        //转换PTS/DTS（Convert PTS/DTS）
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
        //Print to Screen
        if(pkt.stream_index == video_index){
            s_war("Send %8d video frames to output URL",frame_index);
            frame_index++;
        }
        //ret = av_write_frame(ofmt_ctx, &pkt);
        ret = av_interleaved_write_frame(output_format_context, &pkt);

        if (ret < 0) {
            s_war( "Error muxing packet");
            break;
        }

        av_free_packet(&pkt);
    }

    //写文件尾（Write file trailer）
    av_write_trailer(output_format_context);

    return 0;
}

int close_input_and_output()
{
    int ret;

    // close input
    avformat_close_input(&input_format_context);

    /* close output */
    if (output_format_context && !(output_format_context->oformat->flags & AVFMT_NOFILE))
        avio_close(output_format_context->pb);

    avformat_free_context(output_format_context);

    if (ret < 0 && ret != AVERROR_EOF) {
        s_war( "Error occurred.");
        return -1;
    }

    return 0;
}
int userage(int argc,char *argv[]){
	printf("%s help:\n",argv[0]);
	printf("-m [mode]    :mode:decode or encode\n");
	printf("-i [iPath]   :input file path\n");
	printf("-o [oPath]   :output file path\n");
	return 0;
}
int main(int argc, char *argv[])
{
	s_inf("%s build in:[%s %s].",argv[0],__DATE__,__TIME__);
	int opt = 0;
	while ((opt = getopt(argc, argv, "i:o:m:h")) != -1) {
		switch (opt) {
		case 'i':input_file =  optarg;
			break;
		case 'o':output_file =  optarg;
			break;				
		case 'h':userage(argc,argv);
			return 0;
		default:printf("invaild option!\n");
			userage(argc,argv);
			return -__LINE__;
	   }
	}
    init_register_and_network_for_ffmpeg();

    input_stream = create_input_avstream(); //创建输入流、初始化输入context

    if (NULL == input_stream)
    {
        close_input_and_output(); //创建失败,调用close关闭输入输出。
        return -1;
    }

    output_stream = create_output_avstream();//创建输出流、初始化输出context

    if (NULL == output_stream)
    {
        close_input_and_output(); //创建失败,调用close关闭输入输出。
        return -1;
    }

    if(0 != open_url_and_write())
    {
        close_input_and_output(); //调用close关闭输入输出。
        return -1;
    }

    //close
    close_input_and_output();
}