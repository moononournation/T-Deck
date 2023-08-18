# T-Deck

## AVI conversion

```sh
ffmpeg -i input.mp4 -c:a mp3 -c:v cinepak -q:v 10 -vf "fps=15,scale=-1:240:flags=lanczos,crop=320:240:(in_w-320)/2:0" AviMp3Cinepak240p15fps.avi

ffmpeg -i input.mp4 -c:a mp3 -c:v cinepak -q:v 10 -vf "fps=30,scale=-1:240:flags=lanczos,crop=320:240:(in_w-320)/2:0" AviMp3Cinepak240p30fps.avi
```
