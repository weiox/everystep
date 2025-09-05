# Everystep C++ 教程文档 🚀

> 一步一个脚印，踏实学好 C++！每一步都算数，每一步都精彩！

欢迎来到 **Everystep C++ 教程**！这里是你的 C++ 学习伙伴，从基础入门到现代 C++，从面试突击到工程实践，我们陪你走过每一步。

## 🎯 项目简介

这个仓库是 [Everystep 前端项目](https://github.com/weiox/everystep-fe) 的子模块，专门存放 C++ 相关的教程文档内容。所有的 `.mdx` 文件最终会被 Fumadocs 渲染成精美的在线文档。

### 🌟 为什么选择 Everystep？

- **📚 内容全面**：从 C++ 基础到现代特性，从面试题到工程实践
- **🎨 风格轻松**：用人话讲技术，让学习不再枯燥
- **💡 实战导向**：每个概念都有实际例子，学完就能用
- **🔄 持续更新**：跟随 C++ 标准演进，内容与时俱进

## 📖 文档结构

### 🎯 [C++ 基础](./cpp-basics/)
> 万丈高楼平地起，C++ 的摩天大厦也需要坚实的地基！

- [文件 I/O 与 RAII 资源管理](./cpp-basics/file-io-raii.mdx) 📄
- 更多基础内容正在路上... 🚧

### 🚀 [现代 C++](./modern-cpp/)
> 从 C++11 的文艺复兴，到 C++26 的未来展望

#### C++11: 文艺复兴与工业革命 🎨🏭
- [默认与删除函数](./modern-cpp/cpp11/default-delete-functions.mdx)
- [枚举类](./modern-cpp/cpp11/enum-class.mdx)
- [explicit 关键字](./modern-cpp/cpp11/explicit-keyword.mdx)

#### C++17: 军火库大扩充 🧰📦
- [std::any - 类型安全的万能容器](./modern-cpp/cpp17/any.mdx)
- [文件系统库](./modern-cpp/cpp17/filesystem.mdx)
- [std::optional - 优雅处理空值](./modern-cpp/cpp17/optional.mdx)
- [std::string_view - 高效字符串视图](./modern-cpp/cpp17/string-view.mdx)
- [std::variant - 类型安全的联合体](./modern-cpp/cpp17/variant.mdx)

#### C++20: 迈入科幻时代 🚀🌌
- [std::format - 现代字符串格式化](./modern-cpp/cpp20/format.mdx)
- [模块系统](./modern-cpp/cpp20/modules.mdx)
- [std::span - 安全的数组视图](./modern-cpp/cpp20/span.mdx)

#### 内部机制探索 🔍
- [字符串实现原理](./modern-cpp/internals/string-implementation.mdx)

### 🎯 [C++ 面试知识图谱](./cpp-interview/)
> 终极版：一题一文，逐个击破！

#### 🔥 P0: C++ 核心语言
- [堆和栈有什么区别？](./cpp-interview/heap-vs-stack.mdx)
- [new 和 malloc 有什么区别？](./cpp-interview/new-delete-malloc-free.mdx)
- [指针和引用有什么区别？](./cpp-interview/pointer-vs-reference.mdx)
- [static 关键字的作用](./cpp-interview/static-keyword.mdx)
- [struct 和 class 的区别](./cpp-interview/struct-vs-class.mdx)
- [this 指针详解](./cpp-interview/this-pointer.mdx)
- [野指针和悬挂指针](./cpp-interview/pointers-wild-and-dangling.mdx)

### 📐 [C++ 核心指导原则](./cpp-core-guidelines/)
> 基于官方 Core Guidelines 的最佳实践

- [I.1: 让接口显式化](./cpp-core-guidelines/i1-make-interfaces-explicit.mdx)

### 💭 [编程人生](./programming-life/)
> 技术之外的思考与感悟

- [AI 时代下的经典书籍价值](./programming-life/ai-and-classic-books.mdx)

## 🛠️ 技术栈

- **文档格式**：MDX (Markdown + JSX)
- **渲染引擎**：[Fumadocs](https://fumadocs.vercel.app/)
- **样式框架**：Tailwind CSS
- **代码高亮**：Shiki
- **图表支持**：Mermaid

## 🎨 写作风格

### 📝 我们的理念
- **人话讲技术**：拒绝生硬的教科书式表达
- **故事化叙述**：用生动的比喻和例子解释复杂概念
- **互动式学习**：通过提问和对话增强参与感
- **实战导向**：每个知识点都有具体的应用场景

### 🌈 风格特色
- **emoji 点缀**：让技术文档更有趣 ✨
- **渐进式深入**：从简单到复杂，层层递进
- **多维度解释**：从不同角度阐述同一个概念
- **错误友好**：不仅告诉你怎么做对，还告诉你为什么会做错

## 🚀 快速开始

### 本地开发
```bash
# 克隆主项目
git clone https://github.com/weiox/everystep-fe.git
cd everystep-fe

# 安装依赖
pnpm install

# 启动开发服务器
pnpm dev
```

### 贡献内容
1. Fork 本仓库
2. 创建你的特性分支 (`git checkout -b feature/amazing-tutorial`)
3. 提交你的修改 (`git commit -m 'Add amazing tutorial'`)
4. 推送到分支 (`git push origin feature/amazing-tutorial`)
5. 创建 Pull Request

## 📊 项目统计

- **文档数量**：30+ 篇
- **覆盖主题**：基础语法、现代特性、面试题、工程实践
- **代码示例**：100+ 个
- **更新频率**：持续更新中

## 🤝 参与贡献

我们欢迎各种形式的贡献：

- 🐛 **报告错误**：发现文档中的错误或不准确之处
- 💡 **建议改进**：提出更好的解释方式或示例
- 📝 **贡献内容**：添加新的教程或完善现有内容
- 🎨 **改进样式**：优化文档的展示效果

## 📞 联系我们

- **GitHub Issues**：[提交问题或建议](https://github.com/weiox/everystep.git/issues)
- **主项目地址**：[everystep-fe](https://github.com/weiox/everystep-fe)

## 📄 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

---

<div align="center">

**🎯 一步一个脚印，踏实学好 C++！**

Made with ❤️ by Everystep Team

</div>
