# 源码排版与注释

教程自有 C++、GLSL/HLSL 和 CMake 源码统一排版。使用 4 空格缩进，目标行宽 100 列；函数体、循环和条件分支展开，多个语句分别换行。特别长的字符串、不可拆分标识符可能超过行宽。第三方源码及 nvpro_core2 子模块保持上游格式。

## 一次安装，反复整理

在仓库根目录运行，建议使用独立 Python 虚拟环境：

```powershell
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r tools/requirements-format.txt
.\.venv\Scripts\python.exe tools/format_sources.py --clang-format .venv/Scripts/clang-format.exe
```

只检查、不改文件：

```powershell
.\.venv\Scripts\python.exe tools/format_sources.py --check --clang-format .venv/Scripts/clang-format.exe
```

固定 clang-format 19.1.7 和 cmakelang 0.6.13，避免工具版本差异反复改写。`.clang-format`、`.cmake-format.json` 和 `.editorconfig` 是规则来源；工具在遍历前排除第三方、子模块、缓存和构建目录。Python 与这些工具只供维护源码使用，阅读和运行课程不需要安装。

## 编辑器里自动整理

VS Code 已提供 C/C++ 保存时格式化与 100 列标尺配置。安装 Microsoft C/C++ 扩展即可按项目 `.clang-format` 排版；如需与批量检查完全一致，把 `C_Cpp.clang_format_path` 指向上述虚拟环境中的 `clang-format.exe`。GLSL/HLSL 的批量排版通过统一脚本完成。

Visual Studio 可以对 C++ 执行“格式化文档”，并读取仓库 `.clang-format`。编辑器的自动换行只改变显示；批量工具会实际整理源码文件，克隆到其他电脑也保持一致。

## 注释阅读顺序

- 每课 `main.cpp` 开头说明本课重点；较短的 Vulkan/DX12 入口还标出公共实现位置。
- 公共实现重点解释资源生命周期、数据布局、CPU/GPU 同步、屏障和颜色空间。
- 模型与 IBL 注释说明矩阵转换、骨骼权重、环境预计算等容易混淆的步骤。
- 多遍渲染例子标出几何、遮挡、降噪和光照合成阶段，着色器注释说明输入用途与公式含义。

维护时优先解释“为什么”和前置条件；代码或参数变化时同步修改附近注释。图形行为的复验仍执行 README 中的编译与 CTest 流程，格式检查不能替代运行检查。
