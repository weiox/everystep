---
title: "结构型模式总览：从类拼接到稳定结构"
description: "把适配器、装饰器、代理、组合、外观、桥接、享元串成一条演化故事线，聊聊代码长胖之后，怎么给结构加护栏而不是乱打补丁。"
order: 10
---

如果说创建型模式关心的是“对象怎么出生”。
谁来拍板 `new` 谁。

那结构型模式盯着的，就是另一件更麻烦的事。

对象都已经出生了。
还活蹦乱跳。

你拿它们怎么办。

让它们互相认识。
互相协作。
互相背锅。

很多人第一次意识到“结构这玩意不太对劲”，通常不是在课堂上。
而是在一个写了好几年的老项目里。

你会看到模块嘴上都说“解耦”。
手上却互相 `#include` 得像意大利面。

你会看到为了接一个新接口。
老代码旁边贴了一圈临时 wrapper。
还顺手糊了几坨胶水类。

你会看到为了“方便调用”。
裸指针满天飞。
单例句柄一把抓。
全局 getter 像自来水一样，拧开就有。

你也会看到只想改一块 UI。
或者换个存储后端。
结果牵一发动全身。

这时候你再翻开 GoF。
那一串名字——适配器、装饰器、代理、组合、外观、桥接、享元——突然就不再像考点。

更像几位看过无数烂尾楼的前辈。
拍拍你肩膀。
递你一支烟。

> “房子已经盖歪了？”
> “先别炸。”
> “咱们看看能不能在不大动老代码的前提下，把结构慢慢拎清楚。”

下面这篇，我就按“项目一点点长胖”的时间线讲。
你会发现这些模式不是凭空从书里跳出来的。
它们更像是工程史里，反复踩坑之后留下的护栏。

### 从“能用就行的粘合代码”，到“给结构单独立个案”

绝大多数 C++ 项目的前两年，结构都很接地气。

业务模块直接 `#include` 第三方库的具体类型。
哪儿需要就地 `new` 两下。

你可以想象出那种代码的味道。
写的人当时还挺开心。
读的人后来都得皱眉。

```cpp
#include "ThirdPartyLog.h"

void checkout() {
    ThirdPartyLog::instance().write("pay ok");
}
```

这段东西当然能跑。
问题也不在“能不能跑”。

问题在于。
你把项目的血管。
直接焊在第三方库的骨头上。
以后想换库。
就得拆骨。

新需求来了。
就在老类上多塞一个 `bool`。
再加一个 `enum`。
然后写一层 `if (type == ...)`。

协议升级。
接口变更。
就在调用点边上硬贴一个“过渡函数”。
承上启下。

这一阶段的关键词其实是“好改”。

谁最熟这块代码。
谁就去那几行 `if` 里再拧一把。

测试也勉强能过。
毕竟调用链还算短。
大家大致记得哪儿接哪儿。

但项目熬到第三年。
你再回头看。
心里就会有点发凉。

同样一个“下单”流程。
UI 层一套。
服务层一套。
存储层又一套。
每套都像。
又都不完全一样。

你想在中间加一层缓存。
或者加一圈打点统计。
你会发现你得从入口一路改到最底层 I/O。

你想把 A 库换成 B 库。
才发现 API 散落在整个代码库。
像撒了一地的图钉。
你根本无从下手。

这就是结构型模式出现的历史背景。

项目已经长胖。
但谁都不敢大动骨头。

那时候人的直觉很朴素。

接口对不上。
先写个东西翻译一下。

功能不想再往类里塞。
绕一圈挂外面。

对象不好直接见客。
让一个壳子先挡着。

数据结构长得像树。
那就别装看不见。

子系统复杂得没人敢直视。
先装一扇门。
外面的人只跟门说话。

抽象和实现老是绑死。
拆开。
别焊在一起。

对象多到内存喘不过气。
能不能共用点东西。

这些一开始都只是“工程师凭直觉写出来的套路”。
后来 GoF 把它们翻出来。
起了名字。
画了图。
放进 1994 年那本书里。

很多人读完才恍然大悟。

原来我这些年写的“粘合代码”。
早就有人帮忙归档成一套语言了。

### 接口边缘三兄弟：适配器、装饰器、代理

先说三个最容易搞混的角色。

适配器（Adapter）。
装饰器（Decorator）。
代理（Proxy）。

它们看起来都很像。

都挂在接口边缘。
都“长得像目标对象”。
都实现同一套接口。
都在内部转发调用。

所以新人常问一句。

“这不就是多包了一层吗？”

是。
但你包这一层的时候。
心态完全不一样。

**适配器：别动老接口，翻译新世界**

我见过最常见的适配器出现方式，是“领导突然想换库”。

你在老系统里已经用了一套稳定的日志接口。
比如一个老的 `OldLogger::log(...)`。

有一天公司拍脑袋要换一套新日志库。
异步。
批量。
打标签。
应有尽有。

就是 API 跟你现在这套完全不是一回事。

你既不想立刻改完几百个调用点。
也不想让新库的概念污染整个代码库。

于是你会很自然写出这么一个类。

先把“老接口”写出来。
别搞复杂。
它就是团队已经用顺手的那套说法。

```cpp
struct OldLogger {
    virtual ~OldLogger() = default;
    virtual void log(int lvl, std::string_view msg) = 0;
};
```

然后是“新世界”。
新库往往不关心你那套 `lvl`。
它有自己的节奏。

```cpp
class NewBackend {
public:
    void push(int lvl, std::string_view msg);
};
```

最后才轮到适配器出场。
它做的事很脏。
但很值。

```cpp
class NewLogAdapter : public OldLogger {
public:
    explicit NewLogAdapter(NewBackend& backend) : backend_(backend) {}
    void log(int lvl, std::string_view msg) override { backend_.push(lvl, msg); }

private:
    NewBackend& backend_;
};
```

从老代码的视角看。
它还是那个 `OldLogger`。

只是背地里。
它已经帮你接上了新世界。

适配器的心态是这样的。

> “别动我熟悉的接口。”
> “转换的苦活，都塞进这一个类里。”

你要记住一个很实用的判断。
适配器不是为了“加功能”。
它是为了“接口对齐”。

最好的适配器。
往往不是“聪明”。
而是“憋屈”。

它把新库的怪脾气。
都关进一个小房间。
让你的业务代码继续讲人话。

**装饰器：原类别不动，我在外面给它加点料**

装饰器更像给对象“穿衣服”。

这套思路其实一点都不新。
你要是用过 `std::iostream`。
大概率早就“被装饰过”。

`ifstream` 外面套一个 `istream` 视角。
再叠上各种格式化器。
最后你写一行 `<<`。
背后已经走了好几层。

GoF 只是把这种老手艺。
从“大家都会写但没人叫它名字”。
变成了“可以拿来讨论的共识”。

你有一个最朴素的 `Stream`。
一开始只负责读写。

先把这条“流水线”抽象出来。
记住。
装饰器喜欢稳定的接口。

```cpp
struct IStream {
    virtual ~IStream() = default;
    virtual void write(std::string_view data) = 0;
};
```

有了接口。
你就可以写最原始的实现。

```cpp
class FileStream : public IStream {
public:
    void write(std::string_view data) override;
};
```

过一阵想加压缩。
再后来要加加密。
又想加打点统计。

如果都往 `Stream` 里塞。
它会长成“上帝类”。
没人敢动。

老工程里最常见的景象就是。
你打开一个类。
发现它像一锅大杂烩。
谁都往里扔过东西。

装饰器的做法更克制。

压缩是一件事。
就做成一个类。

加密也是一件事。
也做成一个类。

日志也是一件事。
再做一层。

它们都实现同一个接口。
里面包着一个别的 `Stream`。
像套娃。
一层叠一层。

装饰器的壳子长这样。
就一件事。
拿到“下一层”。
然后转发。

```cpp
class LoggingStream : public IStream {
public:
    explicit LoggingStream(IStream& next) : next_(next) {}
    void write(std::string_view data) override {
        sink("write", data.size());
        next_.write(data);
    }

private:
    IStream& next_;
    void sink(std::string_view, std::size_t);
};
```

你想叠几层。
就叠几层。
业务代码只看见同一个接口。

```cpp
FileStream file;
LoggingStream logged{file};
logged.write("hello");
```

到这一步。
很多人会突然明白。

你不是在“加类”。
你是在把“变化”从原类身上拆下来。
拆成可以增删的外套。

调用方眼里永远只是 `Stream&`。
但数据真实地经过压缩。
经过加密。
再经过打点。

装饰器的出发点一句话。

不改原类。
但一件件往外面挂功能。

**代理：对象先别出来见人，我帮你接客**

代理看起来也像装饰器。
同样实现同一个接口。
同样在内部转发调用。

但它更关心的是“时空”。

对象在哪儿。
什么时候才真的创建。
谁能用。
谁不能用。

你可以把代理当成前台。
你以为你在跟业务谈。
其实前台先看一眼你的工牌。

远程代理是经典老故事。
早年 CORBA、COM 那一票东西。
你写起来像本地调用。
实际背后在跨进程。
跨机器。

虚代理也很常见。
你手里先拿着一个“看起来像大对象”的壳。
真正的大对象。
等到第一次用到才 `new` 出来。
这招在 GUI、图片、模型加载里特别实用。

举个最老套。
但最实用的例子。

你有一张大图。
加载一次很慢。
但 UI 上不一定真的会滚到它。

```cpp
struct IImage {
    virtual ~IImage() = default;
    virtual void draw() = 0;
};
```

真实对象很重。
它一构造就去读文件。

```cpp
class RealImage : public IImage {
public:
    explicit RealImage(std::string path);
    void draw() override;
};
```

代理先站在门口。
第一次有人真要画的时候。
它才把重家伙请出来。

```cpp
class ImageProxy : public IImage {
public:
    explicit ImageProxy(std::string path) : path_(std::move(path)) {}

    void draw() override {
        if (!real_) real_ = std::make_unique<RealImage>(path_);
        real_->draw();
    }

private:
    std::string path_;
    std::unique_ptr<RealImage> real_;
};
```

这就是“时空”。
不是多加功能。
也不是翻译接口。

它是在控制。
什么时候付出代价。
谁有资格付出代价。

保护代理就更像权限门禁。
接口一样。
调用一样。
但代理会先检查权限、配额。
再决定要不要把请求转给真实对象。

所以你可以用一条粗暴但好记的线来分。

适配器解决“接口对不上”。
装饰器解决“功能要叠加”。
代理解决“对象要控制”。

三者都像是在接口边缘加一层。
但一个补语义。
一个补功能。
一个补时空。

### 大结构四兄弟：组合、外观、桥接、享元

如果说适配器、装饰器、代理是在接口边缘抠细节。

那组合（Composite）、外观（Facade）、桥接（Bridge）、享元（Flyweight）。
更多是在整体结构上动手。
它们出场的时候。
通常项目已经不是“几个人的小工具”。
而是一片森林。

**组合：承认这就是一棵树**

菜单。
UI 元素。
场景节点。
文件系统。

你别装。
它们本质上就是树。

很多早期代码偏偏不承认。
父节点自己维护一堆 `children_`。
每一层都手搓一遍遍历、统计、渲染。

等你想对整棵树做个统一操作。
调用栈里全是几乎一样的 `for`。
复制。
粘贴。
然后某一层忘了改。

组合的心态很朴素。

> “既然它就是一棵树。”
> “那就让整体和部分在代码里说同一种话。”

于是有了 `Component` 抽象。
叶子节点和中间节点都实现它。
调用方只管对着 `Component&` 调 `draw()` / `update()`。

至于底下是一片叶子。
还是一整棵子树。
交给实现自己递归。

代码写出来其实很朴素。
朴素得像文件系统。

```cpp
struct Node {
    virtual ~Node() = default;
    virtual void render() = 0;
};
```

叶子只管做叶子的事。

```cpp
class Sprite : public Node {
public:
    void render() override;
};
```

中间节点负责“把孩子叫来”。
别小看这句。
这是统一操作的关键。

```cpp
class Group : public Node {
public:
    void add(std::unique_ptr<Node> n) { children_.push_back(std::move(n)); }
    void render() override {
        for (auto& c : children_) c->render();
    }

private:
    std::vector<std::unique_ptr<Node>> children_;
};
```

很多年后你回头看。
组合真正救你的。
不是“少写几个 for”。

而是让你敢于把“统一操作”这件事。
写成一个单点。
然后放心地扩展。

你会发现真正省下来的不是代码行数。
而是脑子里的噪音。

**外观：先给这坨复杂子系统装一扇门**

每个大系统里。
总有一坨东西。
谁都不想直视。

初始化要按严格顺序调五六个函数。
出错要按特定姿势回滚三四步。
文档早就跟不上实现。
只有两个老同事还记得怎么用。

这时候你会有一个非常健康的冲动。

> “我能不能只认识一个类。”
> “它帮我把这一坨东西伺候好。”

这就是外观。

对外只暴露一个门面类。
里面该多复杂多复杂。
但调用方只管 `facade.init()`。
只管 `facade.doWork()`。

你可以把它写得很俗。
甚至像一段脚本。
但它能救命。

```cpp
class MediaFacade {
public:
    void exportMp4(std::string_view in, std::string_view out) {
        demux_.open(in);
        codec_.init();
        mux_.open(out);
        mux_.write(codec_.decode(demux_.read()));
        mux_.close();
    }

private:
    Demuxer demux_;
    Codec codec_;
    Muxer mux_;
};
```

外面的人只需要记住一件事。

```cpp
MediaFacade f;
f.exportMp4("a.mkv", "b.mp4");
```

你把复杂度关进房间。
并不代表房间里不复杂。

但至少。
走廊干净了。

外观的价值也经常被低估。
它不是“偷懒”。

它是在给你留退路。
让你以后能重构内部。
而不用把所有调用点一起拖下水。

**桥接：别再把抽象和实现焊死在一起**

桥接往往出现在“维度爆炸”的那一刻。

一开始你有一个 `Shape` 抽象。
底下挂 `Circle` / `Rectangle`。

后来又冒出“渲染后端”这个维度。
OpenGL。
Vulkan。
软件渲染。

如果你继续靠继承硬堆。
很快就会长出 `OpenGLCircle`、`VulkanCircle`、`OpenGLRect` 这一长串名字。

这不是面向对象。
这是面向组合爆炸。

桥接的想法是。

> “抽象一棵继承树。”
> “实现再一棵继承树。”
> “两边用组合在运行时接起来。”

于是 `Shape` 里握着一个 `Renderer&`。
具体用哪家渲染。
由外面注入。

你也可以把桥接理解成一种“编译防火墙”。
老 C++ 项目里。
很多人靠 `pImpl` 挡 `#include`。
表面上是为了“少编译”。

但更深一层。
其实是在承认一件事。

抽象这边。
想稳定。

实现那边。
想随便折腾。

那就别把它们焊死。

桥接写出来也不玄学。
就是把“继承”拆成两条。
然后用组合接上。

```cpp
struct Renderer {
    virtual ~Renderer() = default;
    virtual void circle(float x, float y, float r) = 0;
};
```

```cpp
class Shape {
public:
    explicit Shape(Renderer& r) : r_(r) {}
    virtual ~Shape() = default;
    virtual void draw() = 0;

protected:
    Renderer& r_;
};
```

```cpp
class Circle : public Shape {
public:
    Circle(Renderer& r, float x, float y, float rad) : Shape(r), x_(x), y_(y), rad_(rad) {}
    void draw() override { r_.circle(x_, y_, rad_); }

private:
    float x_, y_, rad_;
};
```

你突然获得了一种很“现实主义”的自由。

形状的继承树继续长。
渲染后端的继承树也继续长。

它们不是用“类名爆炸”互相绑架。
而是到运行时再握手。

这类设计的好处是。
你扩展形状。
不需要碰渲染后端。

你扩展渲染后端。
也不用碰形状。

**享元：对象太多了，大家能不能共用点东西**

写游戏。
做富客户端。
搞大规模缓存。
你迟早会遇到那一天。

对象太多。
内存顶不住。

屏幕上成千上万棵树。
但九成都长得差不多。

文本编辑器里成百万个字符对象。
每个对象都傻乎乎地拷贝一份字体、颜色信息。

享元模式干的事。
说白了就是“把大家都一样的那部分抽出来”。

抽出一个 `TreeModel` / `Glyph` 当成内部状态。
位置、缩放、当前高亮这些外部状态。
由调用方每次传入。

一万个对象。
共用几十个模型。
内存压力就这样硬生生压下去。

享元的关键是。
别把“位置”这种每次都不一样的东西。
塞进共享对象里。

共享的。
放里面。
变化的。
每次调用时传进去。

```cpp
struct GlyphStyle {
    int fontId;
    int color;
};
```

```cpp
class Glyph {
public:
    explicit Glyph(std::shared_ptr<const GlyphStyle> style) : style_(std::move(style)) {}
    void draw(char ch, int x, int y);

private:
    std::shared_ptr<const GlyphStyle> style_;
};
```

这时候缓存工厂就出现了。
它负责“同样的 style 只造一次”。

```cpp
class GlyphFactory {
public:
    std::shared_ptr<const GlyphStyle> style(int fontId, int color);
};
```

老程序员对享元的评价常常很直白。

> “这玩意不是为了优雅。”
> “是为了别 OOM。”

它听起来有点抠。
但你真到了性能墙前。
就会感谢这种“抠门”。

### 把它们串成一条工程线：当代码从“能跑”长成“一整片森林”

把这七位结构型选手放到同一条时间线上。
你会突然顺很多。

早期项目。
能跑就行。
结构基本靠 `#include` + `if` + 全局 getter 扛着。

开始接外部世界。
要接新库。
要接新协议。
适配器先上。
它替你守住老接口。

功能越堆越多。
你想加日志。
想加缓存。
想加加密。
装饰器开始发挥。
代理也开始挡人挡事。

数据结构长成树。
组合站出来。
让整体和部分说同一种话。

子系统复杂到没人敢直视。
外观替你装门。
先让外面的人活下去。

抽象维度和实现维度互相绑死。
桥接把两边拆开。
别再焊死。

对象数量压到内存喘不过气。
享元出来抠细节。
把“大家其实一样”的那部分拿去共享。

最后再抽象一句。

> 创建型模式回答“对象该怎么出生”。
> 结构型模式回答“这些已经出生的对象，该怎么手拉手，才不至于把系统绞成一团麻”。

以后你在老项目里准备再贴一块“粘合代码”。
或者再造一个“万能 Manager”。
先停一下。

问自己两个问题。

我现在是在跟哪个“历史阶段”的问题打交道。
这个问题。
前人是不是已经踩过无数次坑。
顺手给它起过名字。

当你开始用“结构型模式”这套语言。
去复盘自己项目里那些存在多年的奇怪拼接方式。
你就已经走出了“见招拆招”。

你开始在搭骨架。
搭一个能让代码长期生长的骨架。