---
title: "装饰器模式 (Decorator)：在不改代码的前提下给对象加点料"
description: "从继承膨胀和 if-else 功能开关讲起，聊聊装饰器模式如何通过对象包装，在不动原类的前提下为对象动态叠加行为，以及它和继承、代理、中间件管线之间的边界。"
order: 120
---

我第一次真正“见到”装饰器。

不是在 GoF。

也不是在课堂。

是在很早的 Unix 管道里。

你写一条命令。

前面负责产出。

后面负责过滤。

再后面负责格式化。

最后丢进文件。

每一段都像个小程序。

每一段都只干一件事。

你不改前面的程序。

也不需要知道后面会怎么处理。

你只是把它们。

一段一段接起来。

后来图形界面流行。

大家又发现了一个老问题。

我有一个按钮。

但我想给它加边框。

再加阴影。

再加滚动条。

你不可能每次都去改按钮本身。

也不可能为每个组合都写个新控件。

那会把控件库写成字典。

再后来 GoF 把这些“老手艺”整理成一页 UML。

起了个名字。

叫 Decorator。

但多数工程师真正的心理阴影。

还是来自 Java I/O。

第一次见到那串构造函数。

你会下意识问一句。

这谁维护。

```java
new BufferedInputStream(
    new DataInputStream(
        new FileInputStream("...")))
```

后来你慢慢懂了。

最里层那个。

才是真正“去干活”的。

外面那些层。

长得都像“流”。

对外接口差不多。

但会在调用前后。

顺手加点东西。

有时候是缓冲。

把系统调用凑成一口气。

有时候是格式化。

把“字节”变成“你看得懂的东西”。

有时候是统计。

帮你记一笔。

别等线上炸了才想起来追。

也可能是校验。

顺手把数据的来路去路。

看得更踏实一点。

你不用改最里层。

也不用让调用方知道这些琐事。

这就是装饰器。

如果说**继承**像给类刷油漆。

一层一层刷。

刚开始挺新。

刷到后来。

墙没坏。

人先累。

那装饰器更像给对象套外套。

今天冷。

套一件。

明天要防雨。

再套一件。

房子结构不动。

门窗不改。

客人照样从正门进。

> 不改原类。
>
> 不改调用方。
>
> 只在对象外面包一层。
>
> 需要就再包一层。

接下来我按“工程里的真实路线”讲。

先聊它为什么会在项目里长出来。

再把 GoF 的结构掰开揉碎。

最后聊聊它跟代理、适配器、中间件这些邻居。

到底怎么分家。

---

### 1. 经典痛点：继承树越长，需求越难伺候

先从一个很多人项目里都出现过的故事说起。

假设你有一个最朴素的日志类：

```cpp
struct Logger {
    void log(std::string msg) {
        // 直接写文件
        writeToFile(std::move(msg));
    }
};
```

刚上线的时候，大家用得挺开心：

```cpp
Logger logger;
logger.log("user logged in");
```

过了一阵子，运营说：

> 能不能给每条日志自动带上时间戳？

你心想，这还不简单，我改一行：

```cpp
void log(std::string msg) {
    msg = nowAsString() + " " + std::move(msg);
    writeToFile(std::move(msg));
}
```

再后来，安全同学说：

> 线上日志太敏感了，
>
> 能不能先按某种规则**脱敏 / 加密**一下再落盘？

你继续在这个类里加逻辑：

```cpp
void log(std::string msg) {
    msg = nowAsString() + " " + std::move(msg);
    msg = maskSensitive(std::move(msg));
    writeToFile(encrypt(std::move(msg)));
}
```

再后来，另一个业务说：

> 我这条链路比较金贵，
>
> 想**额外打一份日志到监控系统**，
>
> 但别影响别的模块。

你开始犹豫了。

要不就继续往 `Logger` 里塞 if-else。

开关越塞越多。

最后你写的不是 logger。

是一个“日志功能管理系统”。

要不就走继承。

开一个子类。

叫 `FancyLogger` 也行。

然后剧情很熟。

你先有一个带时间戳的。

再有一个带加密的。

再有一个带监控的。

再然后。

业务说“我两个都要”。

你就开始拼名字。

`TimestampEncryptedLogger`。

`TimestampMetricLogger`。

名字越来越像化学式。

但你也没更好的办法。

这就是典型的**继承组合爆炸**。

每多一个可选功能。

就多一个维度。

类的数量从线性开始拐弯。

慢慢走向指数。

最要命的是。

你还会遇到一些现实问题。

有时候你想做“按对象粒度开关”。

同样是 `Logger`。

这个实例要加密。

那个实例只要时间戳。

继承更像家规。

一立下来。

就很难细颗粒度地改。

还有时候你根本动不了原类。

第三方库。

公共组件。

甚至是别的组的模块。

你 override 之后。

到底要不要 `Base::log()`。

你心里没底。

因为你不知道它内部还做了多少事。

装饰器模式，就是在这些日常纠结里长出来的：

> **别再想着一次性继承出“全功能版子类”，
>
>  而是把这些“小功能”拆成一个个“可叠加外壳”，
>
>  需要的时候临时给对象加几层就行。**

---

### 2. GoF 版结构：一层一层往外包

GoF 书里对 Decorator 的定义很书面：

> 动态地给对象添加额外职责。就扩展功能来说，Decorator 模式比生成子类更为灵活。

翻译成工程师语言。

其实就几句话。

你先得有个“组件接口”。

大家都认它。

都按它来写。

```cpp
struct Logger {
    virtual void log(std::string msg) = 0;
    virtual ~Logger() = default;
};
```

然后你得有个“真正干活的实现”。

这层通常最朴素。

也最不该被各种杂事污染。

```cpp
struct FileLogger : Logger {
    void log(std::string msg) override {
        writeToFile(std::move(msg));
    }
};
```

重点来了。

装饰器要做一件有点“狡猾”的事。

它也实现 `Logger`。

长得像组件。

但它不真干活。

它内部攥着另一个 `Logger`。

然后转调。

```cpp
// 装饰器基类：也是 Logger，但内部包着一个 Logger
struct LoggerDecorator : Logger {
    explicit LoggerDecorator(std::unique_ptr<Logger> inner)
        : inner_(std::move(inner)) {}

    void log(std::string msg) override {
        inner_->log(std::move(msg));
    }

protected:
    std::unique_ptr<Logger> inner_;
};
```

你看它的 `log`。

啥也不做。

就把球传下去。

这就是“壳”的感觉。

壳本身不解决业务。

壳只负责把业务包起来。

接着你开始往壳上加料。

比如时间戳。

```cpp
struct TimestampLogger : LoggerDecorator {
    using LoggerDecorator::LoggerDecorator;

    void log(std::string msg) override {
        msg = nowAsString() + " " + std::move(msg);
        LoggerDecorator::log(std::move(msg));
    }
};
```

再比如加密。

注意它也不需要知道“写文件”怎么写。

它只管在调用前动一下消息。

```cpp
struct EncryptedLogger : LoggerDecorator {
    using LoggerDecorator::LoggerDecorator;

    void log(std::string msg) override {
        msg = encrypt(std::move(msg));
        LoggerDecorator::log(std::move(msg));
    }
};
```

用起来就像 Java I/O。

从里往外套。

```cpp
std::unique_ptr<Logger> logger =
    std::make_unique<EncryptedLogger>(
        std::make_unique<TimestampLogger>(
            std::make_unique<FileLogger>()));

logger->log("user logged in");
```

这时候你手里拿着一个 `Logger`。

但它已经是“带时间戳 + 加密”的版本了。

调用方不需要知道外面包了几层。

想只要时间戳。

把最外层那圈去掉就行。

你改的是“组装”。

不是“类型层次”。

用一句话概括这套结构：

> **Decorator 长得像组件，
>
>  干的却是“包装组件”的活。**

这也是为什么它被归类到“结构型模式”：

它不太关心“创建哪种对象”。

那是工厂那帮人爱管的事。

它也不太关心“对象之间怎么协作完成流程”。

那更像行为型模式在干的活。

它关心的是结构。

这些对象怎么一层套一层拼在一起。

在不动原类的前提下。

还能继续扩展。

---

### 3. C++ 里的实战版：不仅是日志，还有流、客户端、中间件

上面那段是“教科书 Logger”，

现实项目里。

装饰器最常出现的地方。

其实都挺“脏”。

也挺真实。

最早闻到这个味。

通常是在 I/O。

老 Unix 时代就喜欢“管道”。

一个程序只做一件事。

然后串一串。

GoF 把这种“层层包裹”的结构抽象成 Decorator。

Java I/O 又把它发扬光大。

后来你会在客户端 SDK 里再遇到它。

你先有一个最朴素的 `RealClient`。

然后线上抖了。

你加一层重试。

再后来要观测。

你加一层打点。

再后来要链路追踪。

你又加一层 Trace 透传。

最后你拿到的东西。

往往就长成“套娃”。

再往后。

你会在中间件管线里看见同一件事。

Web 框架里。

你先过鉴权。

再挨一下限流。

顺手被记录一笔日志。

最后才轮到熔断这种“保命开关”。

实现方式各有各的脾气。

但很多本质都是“外面包一层 handler”。

在调用前后插一刀。

味道就是 Decorator。

比如我们给一个最简单的 HTTP 客户端。

加“计时”。

再加“重试”。

就两层。

别一上来就十层。

先把接口定下来。

这是所有人都要遵守的门。

```cpp
struct HttpClient {
    virtual HttpResponse get(const std::string& url) = 0;
    virtual ~HttpClient() = default;
};
```

然后是最朴素的实现。

它只负责请求。

别让它背锅。

```cpp
struct SimpleHttpClient : HttpClient {
    HttpResponse get(const std::string& url) override {
        return doRealRequest(url);
    }
};
```

接着是装饰器基座。

套路一样。

自己也实现 `HttpClient`。

里面握着一个 `HttpClient`。

然后转调。

```cpp
struct HttpClientDecorator : HttpClient {
    explicit HttpClientDecorator(std::unique_ptr<HttpClient> inner)
        : inner_(std::move(inner)) {}

    HttpResponse get(const std::string& url) override {
        return inner_->get(url);
    }

protected:
    std::unique_ptr<HttpClient> inner_;
};
```

计时这一层。

只关心时间。

它不关心网络。

也不关心重试。

```cpp
struct TimingHttpClient : HttpClientDecorator {
    using HttpClientDecorator::HttpClientDecorator;

    HttpResponse get(const std::string& url) override {
        auto start = std::chrono::steady_clock::now();
        auto resp  = HttpClientDecorator::get(url);
        auto end   = std::chrono::steady_clock::now();
        logLatency(url, end - start);
        return resp;
    }
};
```

重试这一层。

它只关心失败时再来一次。

它也不应该顺手把“日志格式化”之类的活揽走。

```cpp
struct RetryingHttpClient : HttpClientDecorator {
    using HttpClientDecorator::HttpClientDecorator;

    HttpResponse get(const std::string& url) override {
        for (int i = 0; i < maxRetries_; ++i) {
            auto resp = HttpClientDecorator::get(url);
            if (resp.ok()) return resp;
        }
        return HttpResponse::makeError("retry failed");
    }

    int maxRetries_ = 3;
};
```

最后再把它们套起来。

```cpp
std::unique_ptr<HttpClient> client =
    std::make_unique<TimingHttpClient>(
        std::make_unique<RetryingHttpClient>(
            std::make_unique<SimpleHttpClient>()));
```

哪天你想关掉重试，只留计时，

只要把中间那层换掉，或者不包那层就好：

```cpp
std::unique_ptr<HttpClient> client =
    std::make_unique<TimingHttpClient>(
        std::make_unique<SimpleHttpClient>());
```

从调用者角度看。

永远就一个 `HttpClient&`。

里面到底是素的。

还是带重试。

还是带计时。

完全由外层组装决定。

这就是 Decorator 说的“动态”。

它不是那种玄学的动态。

它很朴素。

就是运行时换不同的组合。

不靠继承改家谱。

靠组合换外套。

---

### 4. 它和继承 / 代理 / 适配器到底差在哪？

Decorator 最容易和几个亲戚打架。

尤其在 code review 里。

你一句“我写了个装饰器”。

别人回你一句“这不就是代理吗”。

然后会议就结束了。

我习惯用一个很土的划分。

看你到底在解决什么问题。

继承。

你在“类”这个层面改家谱。

你是在说：以后凡是 `FancyLogger`。

都得这样干。

这事儿改动面大。

适合统一升级。

也适合统一背锅。

装饰器。

你在“对象”这个层面套外套。

你是在说：这次就给这个实例加点料。

那个实例先别动。

它最擅长的是按实例粒度做配置。

同时还允许叠加。

代理（Proxy）。

它也长得像被代理对象。

但它更像门卫。

它要么控制访问。

要么延迟加载。

要么把远程调用伪装成本地调用。

你以为你在调对象。

其实你在过安检。

适配器（Adapter）。

它更像转换头。

重点不在“加行为”。

重点在“换接口”。

把 A 的插头。

变成 B 的插头。

你要是怕在评审里吵起来。

我给你一个更土的判断法。

别背定义。

就看你到底在忙什么。

你主要是在把接口变成另一个接口。

那多半是 Adapter。

你主要是在把门看紧。

控制访问。

延迟加载。

或者把远程调用伪装成本地调用。

那多半是 Proxy。

你主要是在原有调用的前后。

顺手加点行为。

还希望这些行为能一层层叠。

那大概率就是 Decorator。

当然。

现实里也经常是混合拳。

先把第三方库接口用 Adapter 拉成你习惯的形状。

再用 Proxy 把跨网络访问包成本地调用。

最后在外面挂一圈 Decorator。

有人专门负责打点。

有人顺手做限流。

有人把日志埋好。

你看起来是在写业务。

其实是在给业务穿盔甲。

---

### 5. 现代 C++ 眼里的“装饰味”：不一定要一板一眼

在 GoF 时代，Decorator 还是一套很“面向对象教科书味”的结构；

到了现代 C++，你经常会看到一些**“没挂这个名字，但味儿很像 Decorator”** 的写法。

举个你在现代 C++ 里常见的场景。

函数包装器。

也就是所谓“高阶函数”。

你有一个 `Handler`。

签名是 `Response handle(Request)`。

你不想在每个 handler 里都手写日志。

你就写一个外层包装。

```cpp
template <class Handler>
auto with_logging(Handler h) {
    return [h = std::move(h)](Request req) mutable {
        log("start");
        auto resp = h(std::move(req));
        log("end");
        return resp;
    };
}
```

每包一层。

就多一个行为。

行为组合靠“函数再套函数”。

它没有画 UML。

但它就是在装饰行为。

再说 RAII。

很多 RAII 类型。

本质也在做“前后各插一刀”。

构造 acquire。

析构 release。

比如 `std::lock_guard`。

你表面上是在创建一个对象。

实际上你给 `mutex` 套了层壳。

让加锁解锁变成自动的。

还有监控打点那类 wrapper。

很多库会提供 `InstrumentedXxx`。

里面包着一个真实实现。

每次调用前后顺带记一下指标。

你把结构摊开看。

就是 Decorator。

这些写法不一定会在文档里自称“装饰器模式”，

但在设计层面，都在做同一件事：

> **别把所有行前行后的杂事都塞进主逻辑，
>
>  该拆出去的，用一层壳包起来。**

对熟练 C++ 程序员来说，

更重要的往往不是“我是不是严格画出了 GoF 那张结构图”，

而是你有没有养成这种“把附加行为抽成一层壳”的思路。

---

### 6. 用装饰器时要留心的一些坑

Decorator 一旦用顺手。

很容易一路包上去。

你一开始只是想加一层日志。

后来又补一层重试。

再后来发现还得限流。

再后来把熔断也套上。

最后鉴权也顺手包进来。

最后你得到一条很长的“洋葱链”。

这套结构本身没错。

错的是你以为它没有成本。

先说排查。

某个请求失败。

你得搞清楚它到底死在哪一层。

如果每层都顺手打一条 log。

你会看到同一条请求被刷很多遍。

你不是在排查问题。

你是在阅读噪声。

再说顺序。

顺序这事儿在纸面上很优雅。

在生产上很凶。

先重试再限流。

和先限流再重试。

语义完全不一样。

先加密再打点。

和先打点再加密。

决定了你能不能在监控里看懂 payload。

最后是状态。

装饰器链一长。

总有人忍不住在某层里塞一点“共享变量”。

然后你就会遇到并发下的踩脚印。

比如一个 `RetryingHttpClient` 被多个线程共用。

内部计数器一不小心。

就变成数据竞争。

我这边比较实用的经验是。

让每一层只做一件事。

把顺序当成设计的一等公民。

在组装那层用清晰的名字或配置表达顺序。

出了问题时。

让日志和 metrics 能把话说清楚。

这条请求经过了哪些层。

它到底挂在哪一层。

装饰器本身不复杂，

复杂的是**一长串装饰器一起工作时的整体行为**。

---

### 7. Decorator vs Adapter：壳子和翻译别混了

这俩都是结构型模式。

但干的活不一样。

老 C 时代的例子最直观。

我有个裸 `FILE*`。

`fread / fwrite` 很素。

我嫌频繁调用系统接口。

就套一层 `BufferedFile`。

```cpp
struct BufferedFile : File {
    explicit BufferedFile(File* raw) : raw_(raw) {}

    void write(std::string_view data) override {
        buf_.append(data);
        if (buf_.size() > kFlush) {
            raw_->write(buf_);
            buf_.clear();
        }
    }
private:
    File* raw_;
    std::string buf_;
};
```

接口没变。

还是 `write`。

只是顺手帮你攒一攒。

这就是 Decorator。

当年 Unix 管道也是这个味道。

`grep | sort | uniq`。

每一节都讲同一种“流”接口。

只是前后加点手艺。

---

Adapter 的味道不一样。

早期接日志后端。

老接口是：

```cpp
struct Logger {
    virtual void log(Level, std::string_view) = 0;
};
```

新后端只吃结构化：

```cpp
struct NewBackend {
    void write(LogRecord rec);
};
```

这俩接口不对头。

你不能在业务里到处拼 `LogRecord`。

也不想改老接口签名。

就只好塞个“翻译”。

```cpp
class NewLogAdapter final : public Logger {
public:
    explicit NewLogAdapter(NewBackend& b) : b_(b) {}

    void log(Level lvl, std::string_view msg) override {
        b_.write(LogRecord{lvl, std::string(msg)});
    }
private:
    NewBackend& b_;
};
```

外面看还是 `logger.log(lvl, msg)`。

里面换了口味。

把字符串翻译成结构化。

这就是 Adapter。

---

再拉回一句老话。

Decorator 保持接口不变。

在行前行后加料。

让老调用方什么都不用改。

Adapter 改的是接口。

帮老世界说老话。

新世界听懂。

所以：

如果你遇到的是**行为要多做一点**，接口不想动。

多半是 Decorator。

如果你遇到的是**接口不合群**，类型不想改。

多半是 Adapter。

一个帮你收拢“杂活”。

一个帮你收拢“转换”。

分清这个边界。

别把壳子和翻译混在一块。

### 8. 小结：下一次，你是真的要“改类”，还是只想“加一层壳”？

回过头看 Decorator 这条线，其实就是在反复提醒我们一个问题：

> 这次你是**真的需要一个新子类**，
>
>  还是只是想给某几个对象，
>
>  临时加一点点额外行为？

在 C++ 这些年的演化里。

你其实能看到一条很朴素的路线。

早年大家喜欢对着 UML 背定义。

Decorator 是“结构型模式之一”。

听起来像考试题。

后来你开始在各种库里反复遇到它。

比如 Java I/O 那串“套娃流”。

比如 HTTP 的 middleware 管线。

再比如 metrics wrapper。

每次调用。

都顺手给你记一笔。

你甚至不需要叫它 Decorator。

它也会自己长出来。

因为工程里总有一堆“行前行后”的杂活。

总得找地方放。

再往后。

现代 C++ 的模板。

高阶函数。

RAII。

用更“语言味”的方式表达同一件事。

把可叠加的行为抽出去。

让主干逻辑干净。

真正有用的，

不是你能不能一口气画出 Decorator 的 UML，

而是下次你准备在一个类里继续加 if-else、

或者再多写一个“功能略有不同的子类”时，

脑子里会先闪一下：

> 要不，
>
>  咱们别再给这棵继承树添新枝了，
>
>  换个思路——
>
>  **给这个对象，
>
>   临时加一层壳？**

等你在项目里第一次，

把一坨“行前行后杂活”抽成一串清晰的装饰器链，

你大概就会明白：

GoF 书里那些看似“教科书气十足”的模式。

其实只是在替你总结。

那些老项目里一遍遍踩坑之后。

才攒出来的几句实在经验。

---

### 下一章预告：代理模式

下一篇我们聊 Proxy。

它同样会长得像原对象。

但关注点不是加料。

也不是翻译。

而是“代为出面”。

延迟加载。

访问控制。

缓存和资源托管。

到时候把“装饰器的壳”和“代理的壳”摆在一起。

看看各自站在哪条边界上。
