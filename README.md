# T-Deck

| Setting | Value |
|---:|---|
| __Board:__ | ESP32S3 Dev Module |
| __USB CDC On Boot:__ | Enabled |
| __Flash Mode:__ | QIO 120MHz |
| __Flash Size:__ | 16MB(128Mb) |
| __Partition Scheme:__ | 16M Flash(2M APP/12.5MB FATFS) |
| __PSRAM:__ | OPI PSRAM |

## AVI conversion

```console
ffmpeg -i input.mp4 -c:a mp3 -c:v cinepak -q:v 10 -vf "fps=15,scale=-1:240:flags=lanczos,crop=320:240:(in_w-320)/2:0" AviMp3Cinepak240p15fps.avi

ffmpeg -i input.mp4 -c:a mp3 -c:v cinepak -q:v 10 -vf "fps=30,scale=-1:240:flags=lanczos,crop=320:240:(in_w-320)/2:0" AviMp3Cinepak240p30fps.avi
```

## MPEG conversion

```console
ffmpeg -i input.mp4 -c:v mpeg1video -vb 160k -c:a mp2 -ab 64k -vf "fps=25,scale=224:128:flags=lanczos,crop=224:128:(in_w-224)/2:0" -y 224x128.mpg
```
