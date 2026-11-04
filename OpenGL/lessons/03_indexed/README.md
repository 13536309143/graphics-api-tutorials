# 第 03 课：索引绘制与线框

[课程首页](../../README.md) · [上一课](../02_triangle/README.md) · [下一课](../04_uniform/README.md)

## 学完你应该能够

说明“复用顶点与 EBO”，运行并修改本课示例。

## 运行与观察

在教程根目录执行：

```powershell
.\scripts\run.ps1 -Lesson 03_indexed
```

预期画面：渐变矩形；按住空格显示两个三角形的线框。

## 原理与代码阅读

矩形可拆成两个三角形。直接绘制需要 6 个顶点，但实际上只有 4 个角。EBO 存索引 0,1,2,2,3,0，使两个三角形复用顶点。此课用 glDrawElements，count=6 是索引数量。

GL_ELEMENT_ARRAY_BUFFER 的绑定属于当前 VAO 状态。创建 EBO 时必须绑定对应 VAO；在 VAO 仍然绑定时把 EBO 解绑，会把 VAO 的 EBO 关联清掉。glDrawElements 的最后一个参数是 EBO 内字节偏移，此处 nullptr 表示偏移 0。

默认按逆时针顶点顺序定义正面，本课未启用面剔除。按住空格把 glPolygonMode 切为 GL_LINE，可以看到矩形的对角线。索引必须匹配 GL_UNSIGNED_INT 类型且不能超出 VBO 顶点范围。

阅读 quad/quadIndices 与 Mesh 中 indexed 分支，再比较第 02 课。VAO 与 VBO 不是同一个对象；索引复用的也是完整顶点属性组合，而不只是位置。

首先阅读 [main.cpp](main.cpp)，再追踪其中调用的 [common/app.hpp](../../common/app.hpp) 和 [GLSL 文件](../../shaders)。不要一次通读所有公共工具：先弄清本课调用路径，再展开对应实现。

## 动手练习

改成菱形；有意删除最后三个索引，观察只剩半个矩形。

每次修改 C++ 或 GLSL 后，从根目录执行 `cmake --build build --config Debug --parallel`。运行同一示例对比修改前后；尝试先预测画面，再修改代码。

## 自测

索引数据用 unsigned short 时要同时修改什么？

参考答案：EBO 上传元素大小与 glDrawElements 的 type 都要改成相应大小及 GL_UNSIGNED_SHORT。

## 出错时优先检查

初始化顺序、当前程序、VAO/纹理绑定、uniform 值与坐标范围。Shader 语法错误会显示编译日志，OpenGL 状态错误会在帧尾输出错误编号。参见 [排错手册](../../docs/06_排错手册.md)。
