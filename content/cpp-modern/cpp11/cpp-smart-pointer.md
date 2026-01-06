---
title: '智能指针：unique_ptr、shared_ptr、weak_ptr'
description: '从 new/delete 的“人肉 GC”讲起，聊聊 auto_ptr 的历史包袱，以及 C++11 如何把资源管理变成默认安全。'
---

我记得第一次在生产代码里抓到内存泄漏。

不是靠工具。

是靠“直觉”。

那种直觉来自很多年写 C++ 的经验。

你看见 `new`。

你就会下意识去找 `delete`。

找不到，就开始心里发毛。

那时候的 C++。

资源管理这件事。

基本等于“把责任交给每一个人”。

而人。

会忘。

会被打断。

会被异常打脸。

```cpp
Foo* p = new Foo();
work();
return; // delete 呢？
```

这段代码没有语法错误。

它的错误在更后面。

在某个你以为已经结束的夜里。

在某台机器的内存曲线上。

你会看到它一点点往上爬。

然后你开始怀疑人生。

C++ 很早就给了一个答案。

只是不够“顺手”。

那个答案叫 RAII。

也就是。

“资源的生命期，绑定到对象的生命期”。

```cpp
struct Guard {
    Foo* p;
    ~Guard() { delete p; }
};
```

你把释放动作塞进析构函数。

就算中途 `return`。

就算抛异常。

析构都还会跑。

这件事很像老程序员常说的一句话。

> 让编译器替你记事。

后来标准库开始把这个思路做成“工具”。

C++98/03 时代出了一个名字很像救世主的东西。

`std::auto_ptr`。

它确实救过人。

也确实坑过人。

坑点不在于它“不能用”。

坑点在于它那种尴尬的历史折中。

它为了让“复制”语法能过编译。

把拷贝做成了“转移所有权”。

```cpp
std::auto_ptr<Foo> a(new Foo());
std::auto_ptr<Foo> b = a; // a 被掏空了
```

你没看错。

`a` 复制给 `b` 之后。

`a` 变成空。

这在容器里就更刺激。

因为容器里到处是拷贝。

于是 `auto_ptr` 很快就被大家默认拉黑。

那段时间社区其实已经在自救了。

Boost 的 `shared_ptr`。

各种 `scoped_ptr`。

都是工程实践在前面跑。

委员会在后面追。

到了 C++0x（后来改名 C++11）。

标准库终于把“所有权”这件事说清楚。

不是靠口头约定。

是靠类型。

## unique_ptr：你负责到底

`unique_ptr` 的气质很直接。

独占。

你拥有它。

别人就别想再拥有。

```cpp
std::unique_ptr<Foo> p(new Foo());
```

重点不在于“它会自动 delete”。

重点在于。

它用类型把你的意图写死了。

“这玩意只有一个主人。”

所以它不能拷贝。

但它可以移动。

```cpp
std::unique_ptr<Foo> a(new Foo());
a = std::unique_ptr<Foo>(new Foo());
```

这段看起来有点土。

因为 C++11 没有 `make_unique`（那是 C++14 才补的）。

但思路很现代。

要换资源。

就换掉整个拥有者。

你也可以把所有权交出去。

```cpp
std::unique_ptr<Foo> make() {
    return std::unique_ptr<Foo>(new Foo());
}
```

以前你写工厂函数。

要么返回裸指针。

要么靠文档写一句“记得 delete”。

现在你返回 `unique_ptr`。

读代码的人就知道。

谁创建。

谁拥有。

谁释放。

如果你要把 `unique_ptr` 传进函数。

你得先想清楚。

你是“借给它看一眼”。

还是“把家当交给它”。

```cpp
void observe(const Foo* p);
void take(std::unique_ptr<Foo> p);
```

第一种是观察。

第二种是接管。

签名已经把交易写在合同里了。

这就是它最值钱的地方。

还有一种常见资源。

不是 `new/delete`。

是系统句柄。

比如 `FILE*`。

你也可以用 `unique_ptr` 管。

```cpp
std::unique_ptr<FILE, int(*)(FILE*)> f(std::fopen("a.txt", "r"), &std::fclose);
```

你把关闭动作当成“删除器”。

析构时就会自动调用。

这类写法看起来有点怪。

但它的效果很朴素。

你不用再在每个 `return` 前手动补一行 `fclose`。

## shared_ptr：大家一起扛

有些场景。

资源确实不止一个地方用。

比如缓存。

比如图结构。

比如异步回调。

你没法把“唯一主人”讲清楚。

那就用 `shared_ptr`。

```cpp
auto p = std::shared_ptr<Foo>(new Foo());
a(p);
b(p);
```

它背后有一套计数机制。

最后一个 `shared_ptr` 走的时候。

才释放资源。

这很方便。

也很容易让人上头。

因为它会让“谁拥有谁”变得模糊。

模糊久了。

你会发现系统里到处都是 `shared_ptr`。

然后性能开始慢。

调试开始难。

你开始怀念当初那种“责任明确”的日子。

所以我更喜欢把它当成一种声明。

声明“这里的生命周期是共享的”。

一旦你写了 `shared_ptr`。

你就要为这个共享付账。

还有一个细节。

很多人第一次用 `shared_ptr` 就踩。

“不要把同一个裸指针塞进两个 shared_ptr”。

这句话如果你第一次听。

可能会觉得有点玄。

“我只是把指针交给两个地方用。”

“怎么就埋雷了？”

原因其实很朴素。

每个 `shared_ptr` 都会带着一份“记账本”。

记账本里记录。

现在有多少个 `shared_ptr` 在用这个对象。

最后一个走的时候。

它就负责把对象 delete 掉。

这个记账本。

很多资料叫它“控制块”。

你不需要一开始就记住名词。

你只要记住。

`shared_ptr` 不是只有一个指针。

它还有一套所有权的账。

```cpp
Foo* raw = new Foo();
std::shared_ptr<Foo> a(raw);
std::shared_ptr<Foo> b(raw); // 两份控制块，等于埋雷
```

这段代码看起来像复用。

其实是双重释放的邀请函。

因为 `a` 有一份账。

`b` 也有一份账。

它们都以为。

“自己才是这根 raw 的合法主人”。

于是等它们分别析构的时候。

就会出现两次 delete。

这类 bug。

特别像定时炸弹。

你可能跑一阵都没事。

但它一旦炸。

堆栈往往很难看。

如果你要共享。

就从一开始就共享。

让它只有一份控制块。

最推荐的写法。

是直接用 `make_shared`。

```cpp
auto sp = std::make_shared<Foo>();
auto a = sp;
auto b = sp;
```

你如果手里已经有一个 `shared_ptr`。

那就复制它。

别再回到裸指针。

```cpp
std::shared_ptr<Foo> a = /* ... */;
std::shared_ptr<Foo> b = a;
```

## weak_ptr：别让循环把你困死

`shared_ptr` 最经典的坑。

不是性能。

是循环引用。

你写个双向链表。

前后都用 `shared_ptr`。

看起来很合理。

```cpp
struct Node {
    std::shared_ptr<Node> next;
    std::shared_ptr<Node> prev;
};
```

然后你发现。

节点永远不会析构。

因为 `next` 和 `prev` 互相把对方的计数顶着。

这时候 `weak_ptr` 就登场了。

它像一个“旁观者”。

它能看见对象。

但不参与拥有。

```cpp
struct Node {
    std::shared_ptr<Node> next;
    std::weak_ptr<Node> prev;
};
```

当你需要用 `prev` 的时候。

你得先“试着借一把钥匙”。

```cpp
if (auto p = node->prev.lock()) {
    use(*p);
}
```

`lock()` 成功。

说明对象还活着。

失败。

说明它已经走了。

这种写法有点像现实世界。

你能记得一个人的联系方式。

但你不能保证他永远在线。

## 这三种指针，到底怎么选

如果你问我一个“默认选择”。

我会说。

先用 `unique_ptr`。

因为它把责任写得最清楚。

只有当你真的需要共享生命周期。

才拿出 `shared_ptr`。

而 `weak_ptr`。

更多时候是给 `shared_ptr` 还债用的。

你会发现。

C++11 的智能指针并不神秘。

它们只是把一件老事。

用类型讲清楚。

讲清楚之后。

编译器就能帮你盯。

你就不用把脑子浪费在“记得 delete”这种事上。

你可以把注意力放回正经问题。

比如。

你的业务到底想表达什么。
