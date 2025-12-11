---
title: "建造者模式 (Builder)：给复杂对象找个装修队长"
description: "从构造函数参数地狱和配置对象乱飞讲起，聊聊建造者模式如何用分步构建和链式调用，让复杂对象的创建过程变得可读、可测试、可演进。"
order: 50
---

如果说抽象工厂是"一整套家居套餐一起换皮"，
那 Builder 更像是：请了个懂行的装修队长。

> 户型早就定好了，
> 你只管说：墙怎么刷、灯怎么装、地板用什么材质。
>
> **工厂帮你定户型，
>  Builder 盯着装修流程别翻车。**

简单工厂、工厂方法、抽象工厂这些伙伴，聊的都是一件事：
**这次要造哪一类东西。**

Builder 关心的则是另外一句灵魂拷问：

> 这个对象，光 `new` 出来还不够，
> 它是怎么一步步被配出来的？

很多人第一次听到 Builder，反应都差不多：

- 不就一个构造函数嘛，参数多一点还能接受；
- 真要复杂，搞个配置 struct 往里一塞不就行了。

一开始看起来确实都能用，我当年写业务也这么糊过。 
问题出在后面。需求一改，字段一加，调用处一复制粘贴，
原来那个“还不错”的构造函数，很快就长成下面这样一堵“参数墙”：

```cpp
Order(const std::string& userId,
      const std::string& productId,
      int quantity,
      bool needInvoice,
      bool allowSplit,
      bool useCoupon,
      bool isVipOnly,
      const std::string& remark,
      /* ... 还在继续长 ... */);
```

线上翻车通常就栽在这里：

- 某次改需求，只是往构造函数里“顺手”多塞了一个 `bool isVipOnly`；
- 有人为了省事，直接复制旧调用，把新参数随手填了个 `true/false`；
- 过几周谁也想不起来当时的业务假设，排查问题全靠猜。

这些故事听起来有点夸张，但基本都是我亲眼见过的版本：半夜上线，大家围在大屏前盯着 QPS 和报错曲线，等到最后定位到问题，才发现罪魁祸首就是某个构造函数里第七个 `bool`——当年谁拍着胸脯说“先随便写个 true 吧，回头再收拾”，结果半年过去谁也不敢动，最后只能整段回滚。

再配合一堆到处乱飞的配置对象：

- 有人只设置了其中几个字段，另外几个忘了管；
- 有人复制前人代码，结果把已经废弃的字段也照抄了一遍；
- 一年之后，谁也说不清某个布尔量到底是什么意思。

这时候你就会开始怀念：

> 要是能像点外卖那样，
> 一步步勾选：主食 → 配菜 → 饮料 → 备注，
> 最后再点一键下单就好了。

建造者模式，干的就是这件事：

- 把**复杂对象的构建过程拆成一小步一小步**；
- 让调用方**按业务语义一步步“填空”**，而不是背构造函数的参数顺序；
- 最后用一个 `build()` 或 `finish()` 收尾，拿到构造好的对象。

> **真正拖垮可读性的，往往不是几行 if-else，
>  而是那个谁也不敢动的巨型构造函数。**

下面我就从“构造函数参数地狱”这个老问题讲起，
一路聊到经典 GoF Builder 和现代 C++ 里的链式 Builder 写法，
顺便把它和工厂 / 原型这些兄弟们的边界捋一捋。

### 1. 痛点回顾：构造函数参数地狱

先看一段在业务代码里经常见到的味道：

```cpp
struct HttpRequest {
    std::string method;
    std::string url;
    std::map<std::string, std::string> headers;
    std::string body;
    int timeoutMs;
    bool followRedirect;
    bool useProxy;
};

HttpRequest makeRequest() {
    HttpRequest req{
        "POST",
        "https://api.example.com/order",
        { {"Content-Type", "application/json"} },
        R"({"id": 123, "amount": 100})",
        3000,
        true,
        false
    };
    return req;
}
```

刚写出来的时候，
你可能还觉得这种“聚合初始化 + 注释”挺清晰的。

但时间一长，`HttpRequest` 的字段越来越多：

- 多了 `retryCount`、`traceId`、`priority`；
- 某些调用场景要特殊的 header 组合；
- 有的业务必须走代理，有的绝不能走。

于是你开始在各个角落看到不同版本的初始化代码：

- 有的复制粘贴一份，再多改两行；
- 有的少填一个字段，默认值也说不清是谁定的；
- 想“看懂一次请求是怎么被配置出来的”，
  经常要在构造函数和调用点之间来回翻。

**这类场景，基本就是建造者模式的主战场。**

### 2. 一个朴素的 Builder：先说结果，再说过程

不急着上 GoF 那一套，我们先来一个“工程师自己会自然写出来”的版本，看看这个模式是怎么从一堆 if 和参数慢慢长出来的。

很多人第一次看到下面这段代码，脑子里会冒出三个问号：

- `timeoutMs = 3000` 这种在 struct 里直接写默认值是啥新语法？
- 这些 `method(...).url(...).header(...)` 连成一串，为什么编译器不骂人？
- 函数里到处 `std::move`，是在优化什么，还是必须这么写？

别急，我们一块拆开看。

先看数据长什么样：

```cpp
struct HttpRequest {
    std::string method;
    std::string url;
    std::map<std::string, std::string> headers;
    std::string body;
    int timeoutMs = 3000;
    bool followRedirect = true;
    bool useProxy = false;
};
```

这段里有一个**现代 C++ 小语法**：**成员默认值**。

在 C++98 年代，你如果想给 `timeoutMs` 一个默认值，只能在构造函数里手动写 `timeoutMs = 3000;`。
C++11 之后，就可以直接在声明的位置写 `int timeoutMs = 3000;`，构造出来的每一个 `HttpRequest`，默认就是 3 秒超时、跟随重定向、不走代理。老项目如果还在构造函数里堆默认值，可以慢慢往这种写法迁。

接下来是“装修队长”本体——Builder：

```cpp
class HttpRequestBuilder {
public:
    HttpRequestBuilder& method(std::string m) {
        req_.method = std::move(m);
        return *this;
    }

    HttpRequestBuilder& url(std::string u) {
        req_.url = std::move(u);
        return *this;
    }

    HttpRequestBuilder& header(std::string key, std::string value) {
        req_.headers.emplace(std::move(key), std::move(value));
        return *this;
    }

    HttpRequestBuilder& body(std::string b) {
        req_.body = std::move(b);
        return *this;
    }

    HttpRequestBuilder& timeoutMs(int t) {
        req_.timeoutMs = t;
        return *this;
    }

    HttpRequestBuilder& followRedirect(bool v) {
        req_.followRedirect = v;
        return *this;
    }

    HttpRequestBuilder& useProxy(bool v) {
        req_.useProxy = v;
        return *this;
    }

    HttpRequest build() const {
        return req_;
    }

private:
    HttpRequest req_;
};
```

这段最容易把人看晕的是两个点：

- 函数返回的不是 `void`，而是 `HttpRequestBuilder&`；
- 每个函数最后都 `return *this;`。

这其实就是**链式调用 (fluent interface)** 的老套路：
每一步配置完，返回当前这个 Builder 自己的引用，下一步还能在它身上继续点。你在 iostream 里看到过的

```cpp
std::cout << "hello" << 123 << std::endl;
```

如果把它拆开一点看，其实相当于依次调用几个重载的 `operator<<`：

```cpp
std::ostream& tmp1 = operator<<(std::cout, "hello");
std::ostream& tmp2 = operator<<(tmp1, 123);
std::ostream& tmp3 = operator<<(tmp2, std::endl);
```

每一次 `operator<<` 调用，做的事都是：

- 往同一条输出流里塞一点东西；
- 再把这条流本身（`std::ostream&`）原封不动地还给你。

正是因为“**把自己再还回去**”，你才能一口气写成一整串 `a << b << c`。这套设计最早是在 iostream 里流行开来的“前浪版 fluent interface”，后来大家发现这种写法又顺手又好读，就开始在各种 Builder / 配置类里复刻同一个套路——本质上也是类似的链，只是那边返回的是 `std::ostream&`，这里返回的是 `HttpRequestBuilder&`。

至于 `std::move`，在这里可以先理解成一个“小优化”：
我们把参数按值传进来（方便从字符串字面量、`std::string` 等各种来源统一接），然后通过 `std::move` 把这块内存“搬”进内部的 `req_`，避免多一次拷贝。你要是暂时还不想纠结移动语义，也可以先照抄这段模板用，等哪天项目里整体上 C++11/14 之后，再慢慢回头看。

最后是最像“点外卖”的那一串链式调用：

```cpp
HttpRequest makeRequest() {
    HttpRequestBuilder builder;
    return builder
        .method("POST")
        .url("https://api.example.com/order")
        .header("Content-Type", "application/json")
        .body(R"({"id": 123, "amount": 100})")
        .timeoutMs(3000)
        .followRedirect(true)
        .useProxy(false)
        .build();
}
```

这里顺手用了一下 **原始字符串字面量**：`R"( ... )"`。
这玩意儿是 C++11 给的福利，主要就是**不用再到处写反斜杠转义**——`R"({"id": 123})"` 里那对大括号和引号都按字面意思塞进字符串，不用写成 `\"`、`\\` 之类，看日志和调试都清爽很多。

把上面的几个点串起来，你可以这样理解这一小节里的 Builder：

- `HttpRequest` 是那套“户型图 + 默认配置”；
- `HttpRequestBuilder` 是拿着这张图，一步步帮你把细节填满的装修队长；
- 链式调用只是个“看起来像 DSL 的写法糖”，本质仍然是**普通的成员函数 + 返回自身引用**。

当你习惯了这几处现代 C++ 语法糖之后，这段代码读起来就不再是“魔法语法”，而更像是：
“按照业务语义，一行行把一个请求的配置说清楚”。

这就是最朴素的 Builder：

> **把“构造一个复杂对象”的过程，
>  拆成一系列有名字的小步骤，再通过链式调用把它们串起来。**

### 3. GoF 式 Builder：有个“施工队长”在指挥

上面的写法，其实已经能解决 80% 的“构造参数地狱”问题了。

那 GoF 书里讲的 Builder 又多了什么？

如果把时间轴往回拨一点，Builder 这个词最早是 GoF 那本《Design Patterns》（1994）里系统提出来的。那会儿大家还在写 C++ / Smalltalk 桌面程序，更关心“怎么一步步搭出一棵复杂的对象树”，比如文档、窗口、菜单这样的结构。

后来到 Java 火起来的那些年，Joshua Bloch 在《Effective Java》里又把 Builder 当成对付“伸缩式构造函数”的杀手锏，于是企业开发里到处都是 `XxxBuilder`。现代 C++ 里常见的链式 Builder，其实多少是吃了那一波 fluent API 风格的红利。

简单说，GoF 关注的是：

- **同一个构建步骤，可以造出不同表示的对象。**

经典的结构大概是这样：

- 有一个 `Builder` 抽象接口，约定“准备地基 / 搭骨架 / 装修内部”这类步骤；
- 有若干个具体的 `ConcreteBuilder`，分别实现这些步骤，造出不同的产品；
- 再有一个 `Director`（施工队长），
  负责按照某种顺序调用这些步骤，控制“怎么建”。

翻译成一个简化的 C++ 例子：

```cpp
struct House {
    std::string wall;
    std::string roof;
    std::string floor;
};

struct HouseBuilder {
    virtual void buildWall() = 0;
    virtual void buildRoof() = 0;
    virtual void buildFloor() = 0;
    virtual House getResult() = 0;
    virtual ~HouseBuilder() = default;
};

struct WoodenHouseBuilder : HouseBuilder {
    void buildWall() override { house_.wall = "wood wall"; }
    void buildRoof() override { house_.roof = "wood roof"; }
    void buildFloor() override { house_.floor = "wood floor"; }

    House getResult() override { return house_; }

private:
    House house_;
};

struct StoneHouseBuilder : HouseBuilder {
    void buildWall() override { house_.wall = "stone wall"; }
    void buildRoof() override { house_.roof = "stone roof"; }
    void buildFloor() override { house_.floor = "stone floor"; }

    House getResult() override { return house_; }

private:
    House house_;
};

struct ConstructionDirector {
    void construct(HouseBuilder& builder) {
        builder.buildWall();
        builder.buildRoof();
        builder.buildFloor();
    }
};
```

使用方式：

```cpp
ConstructionDirector director;
WoodenHouseBuilder woodenBuilder;
StoneHouseBuilder stoneBuilder;

director.construct(woodenBuilder);
House wooden = woodenBuilder.getResult();

director.construct(stoneBuilder);
House stone = stoneBuilder.getResult();
```

这里的点在于：

- `ConstructionDirector` 固定了“建房子”的流程；
- `HouseBuilder` 子类决定“这次是木屋还是石屋”；
- 如果某天想换一种建造顺序，只要动 `Director`，
  就能影响所有具体 Builder。

在现代 C++ 项目里，
大家不一定会一板一眼地把 `Director` 单独拎出来，
但“**用一系列有名字的步骤，按顺序构建一个复杂对象**”这个想法，
是一直活着的。

### 4. Builder 和 工厂 / 原型，边界在哪？

讲到这里，难免有人会问：

> 这不还是在“创建对象”吗？
> 为啥要多一个 Builder，工厂和原型不够用吗？

可以粗暴地这样划分一下：

- **工厂 / 抽象工厂**
  - 关心的是“**我这次要哪一类对象**”：
    - 是 `ImageTask` 还是 `VideoTask`；
    - 是 Light 主题的 Button 还是 Dark 主题的 Button；
  - 一般是一句 `factory.createXxx(...)` 就搞定了。
- **原型 (Prototype)**
  - 关心的是“**我有一个样板，想按它的样子再复制一份**”；
  - 比如一堆预先注册好的模板对象，
    通过 `clone()` 拿到一个新副本，再稍微改几下。
- **建造者 (Builder)**
  - 关心的是“**这个对象本身很复杂，构造过程也值得被抽象出来**”；
  - 你不只是 `new` 一下，而是要经历一整段“配置 / 组装 / 校验”的流程。

用一句比喻串一下：

> 工厂更像是“挑户型”和“挑哪栋楼”，
> 原型像是“按样板房复制一套”，
> Builder 则是在说：**这套房子怎么一步步装修出来**。

很多项目里，这几位是可以一起搭配使用的：

- 用抽象工厂选出“这一整套家居”的主题；
- 用 Builder 来描述“这套家到底怎么装修、怎么布置”；
- 用原型在某些需要复制配置的场景下快速拷一份模板。

### 5. 什么时候该上 Builder？

从经验上看，下面这些信号一出现，
就可以考虑是不是该搞一个 Builder 出来：

- 某个类的构造函数参数已经多到你不想数；
- 不同调用方对同一个对象的配置方式各不相同，
  但那套配置逻辑又挺有业务含义；
- 你开始在代码里看到“构造 + 配置 + 校验”混在一起，
  单个函数因为要“准备这个对象”变得又长又难懂。

- 你希望产出的对象一旦构造完成就基本不可变，所有“不确定”“还在商量”的状态，都留在 Builder 里慢慢折腾，最后通过 `build()` 一次性收口；
- 你想在 `build()` 里集中做一套比较严谨的校验逻辑，把各种“不该同时出现的选项”或“缺少某必选字段”在这里拦下来，而不是让每个调用方各写一遍防御性代码。

反过来，如果只是两三个参数的小对象，
或者本身就只是个简单值对象，
那就没必要为了“用上一个模式”硬搞一个 Builder，
不然只是在给自己加层间接。

顺带一提，很多代码库里的“Builder”其实只是把一堆 `setXxx()` 串起来，没有任何约束也不做校验，那只是把原来构造函数里的锅摊到了调用点，并不能算是一个有设计感的模式实现。

你可以顺手翻一翻自己项目里的代码：

- 有没有那种“为了构造一个请求 / 订单 / 配置”，
  一个函数里写满了对同一个对象的各种 `setXxx()` 调用？
- 这些 `setXxx()` 之间有没有隐含顺序和约束，
  但现在完全靠调用者自己记？

如果答案是“有，而且不少”，
那建造者模式大概率能帮你把这些“构造流程”
从业务逻辑里剥离出来，变成一段更好讲、更好测的代码。

---

这一篇先把 Builder 讲到这里。

在创建型这条线上，我们现在已经有：

- 简单工厂：从满地 `new` 到集中开工厂；
- 工厂方法：当简单工厂胖到没人敢改时的自然进化；
- 抽象工厂：一整套家居套餐一起换皮；
- 建造者：给复杂对象找一个靠谱的“装修队长”。

再往前走，就是原型模式 (Prototype)：
当你手上已经有一堆“样板房”，
怎样优雅地按这些样板复制出一栋又一栋，
而不是每次都从图纸开始重画。

如果你前面还没看简单工厂 / 工厂方法 / 抽象工厂那几篇，
可以回头对照一下：你们现在造对象这块，停在了哪一站？

等原型那一篇也补上，整个“创建型小系列”基本就成型了：
从“挑户型 / 套餐”，到“找装修队长”，再到“按样板房复制一栋又一栋”。
