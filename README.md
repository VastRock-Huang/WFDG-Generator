# WFDG-Generator
C语言加权特征依赖图生成工具  
A weighted feature dependency graph generation tool for C language
## 环境依赖
操作系统:
* Linux Ubuntu >=18.04

程序依赖:
* [pybind11](https://github.com/pybind/pybind11)
* CMake >=3.14
* Clang 9.0.0
* LLVM

## 项目构建
```shell
cd ./3rdparty
git clone https://github.com/pybind/pybind11.git
cd ..
mkdir build && cd build
cmake ..
make
```

## 项目使用
构建项目后会在 `build/bin` 目录下生成可执行文件 `wfdg_gen_tool`, 在 `build/lib` 目录下生成 Python 可直接调用的链接库文件 `wfdg_generator` (可使用 `cmake .. -DPY_MOD=OFF` 关闭链接库的生成)
1. 直接作为工具使用
```shell
./wfdg_gen_tool [options] <source0> [... <sourceN>]
```
2. 在Python脚本中调用
```python
import wfdg_generator
filepath = ["demo.c"]
config = wfdg_generator.Configuration()
compile_args = ['-I/usr/local/lib/clang/9.0.0/include']
wfdgs = wfdg_generator.gen_WFDGs(filepath, config, compile_args)
```

## 参考文献
[1] Lei Cui, Zhiyu Hao, Yang Jiao, et al. VulDetector: Detecting Vulnerabilities Using Weighted Feature Graph Comparison. IEEE Transactions on Information Forensics and Security, 2020, 16: 2004-2017