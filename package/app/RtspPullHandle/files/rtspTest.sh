ltrace -f -o 111.log RtspPullHandle -i rtsp://192.168.63.7:8554/xjv.264 -o .mp4 -s 30 -l1111 &
sleep 20
killall -USR1 RtspPullHandle
sleep 20
killall RtspPullHandle
# rm -rf pic_dir/*
# ffmpeg -i "cut264.mp4" -r 1 -q:v 2 -f image2 pic_dir/pic-%03d.jpeg
#ffmpeg -i 20191209023633.mp4 -vf select='eq(pict_type\,I)' -vsync 2 jpg_dir/xj-%04d.jpg
#ffmpeg -i xjv.264 -vf select='eq(pict_type\,I)' -vsync 2 pic_dir/pic-%03d.jpeg
# ffmpeg -i 20191209023633.mp4 -vf select='eq(pict_type\,I)' -vsync 2 iframe_dir/i-%04d.jpg

