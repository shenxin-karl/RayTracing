## 构建

环境搭建需要 Visual Studio 2022 工具集 Clang-cl 编译器和 Xmake
下面这是使用 Scoop 配置环境.  直接在 powershell 中执行

```bash
# 安装 scoop
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
iwr -useb get.scoop.sh | iex

# 安装 llvm 
scoop install llvm

# 安装 xmake
scoop install xmake
```

### 控制台编译执行

```bash
git clone --recursive https://github.com/shenxin-karl/RayTracing.git
cd RayTracing
xmake run
```

### 生成工程文件

执行 **GenerateProject.bat** 脚本, 在 **Solution** 下有对应的解决方案. 同时在项目路径下会生成 **compile_commands.json** 可供 Vsual Studio Code 使用 clangd 开发



## 支持的效果

- [x] ToneMapper
- [x] Forward PBR + Deferred PBR
- [x] [Ray Tracing Soft Shadow](ReadmeMedia/SoftShadowReadme.md)
- [x] FSR2
