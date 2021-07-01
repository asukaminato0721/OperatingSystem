# OperatingSystem

## 一个 shell 的在线模拟器

课设

[online-demo](https://wuyudi.github.io/OperatingSystem/)

目前支持的命令:

用户登录，login username password

```bash
login admin admin
cd 目录
cd 任意个点
cd folder1/folder2/folder3/../folder4/..
mkdir folder1 folder2 ...
cat folder1/folder2/../file
echo content > file
echo content >> file
touch file1 file2 ...
rm file1 file2 ...
cp folder/folder2/../file_from folder/folder1/../file_to
clear
exit
tree
```

## 目前待改进的

改成根据 path 找 id,目前只做了 path 匹配
