# 简易焊台

来自 https://oshwhub.com/lilinkai/jian-dan-jia-re-tai

## 编译方法

自行安装 [sdcc](https://sdcc.sourceforge.net).

```sh
# 如果是包管理器安装，sdcc可以直接执行，可以不加 --sdk 选项
# xmake f 会自动下载 fwlib_stc8 依赖并编译
xmake f -p cross -a mcs51 --toolchain=sdcc --sdk=${SDCC_DIR} -c
xmake b root
# 编译输出在 build/cross/mcs51/release/boot.bin
```
