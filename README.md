# 简易焊台

来自 https://oshwhub.com/lilinkai/jian-dan-jia-re-tai

## 编译方法

下载 sdcc 并设置到 path
``` sh
> xrepo add-repo zeromake https://github.com/zeromake/xrepo.git
> xrepo install sdcc
> xrepo info sdcc
installdir: C:\Users\ljh\AppData\Local\.xmake\packages\s\sdcc\4.5.0\e6aa3c720cd8435ba7f7806fe30454fe
# 把对应的 installdir/bin 添加到 PATH 里
# pwsh
> $env:PATH+=";$installdir\bin"
# bash
> export PATH = "$PATH:$installdir/bin"
```

```sh
# xmake f 会自动下载 fwlib_stc8 依赖并编译
xmake f -p cross -a mcs51 --toolchain=sdcc -c
xmake b root
# 编译输出在 build/cross/mcs51/release/boot.bin
```
