killall RtspPullHandle 2> /dev/null
RtspPullHandle -u jonny:111111 \
	-i rtsp://192.168.43.103:8554/ffdec.264 \
	-o ftp://192.168.43.100//data/ttt.264 -l1111 &
while true
do	
	sleep 30
	killall -USR1 RtspPullHandle
done
# sleep 20
# killall RtspPullHandle
# rm -rf pic_dir/*
# ffmpeg -i "cut264.mp4" -r 1 -q:v 2 -f image2 pic_dir/pic-%03d.jpeg
# ffmpeg -i 20191209023633.mp4 -vf select='eq(pict_type\,I)' -vsync 2 jpg_dir/xj-%04d.jpg
# ffmpeg -i xjv.264 -vf select='eq(pict_type\,I)' -vsync 2 pic_dir/pic-%03d.jpeg
# ffmpeg -i 20191209023633.mp4 -vf select='eq(pict_type\,I)' -vsync 2 iframe_dir/i-%04d.jpg

