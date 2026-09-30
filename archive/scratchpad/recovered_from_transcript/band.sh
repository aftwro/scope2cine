for v in 2 1 0 2; do
  v4l2-ctl -d /dev/video0 -c power_line_frequency=$v
  timeout 20 ffmpeg -hide_banner -loglevel quiet -f v4l2 -input_format mjpeg -video_size 1920x1080 -framerate 30 -i /dev/video0 -frames:v 45 -c copy -y band_$v.mjpeg
  ffmpeg -hide_banner -loglevel quiet -i band_$v.mjpeg -vf "select=gte(n\,40),scale=1:1080,format=gray" -frames:v 1 -f rawvideo - | python3 -c "
import sys,statistics as s;d=sys.stdin.buffer.read();print('plf=$v mean=%.1f rowstd=%.2f min=%d max=%d'%(s.mean(d),s.pstdev(d),min(d),max(d)))"
done
