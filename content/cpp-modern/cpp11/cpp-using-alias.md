---
title: 'using 类型别名与别名模板：typedef 的升级版'
description: 'C++11 的 using 把类型别名变得更直观，也补上 typedef 做不到的“别名模板”，让模板代码少一层绕口令。'
---

typedef 这东西。

老。

但不坏。

它救过很多人。

尤其是在那种类型名字长得像火车的年代。

```cpp
typedef std::vector<std::string> StrVec;
```

能用。

但 typedef 有一个很尴尬的限制。

它对模板别名。

不太友好。

你想写“把 vector 这个壳拿出来，里面装什么都行”。

typedef 做不到。

你只能写一层 struct。

再加一个 `::type`。

读起来像绕口令。

C++11 给了 `using`。

它做了两件事。

让类型别名更好读。

也让别名模板成为正经语法。

## using 类型别名：更像赋值

```cpp
using StrVec = std::vector<std::string>;
```

它读起来很自然。

StrVec 就是 vector<string>。

没有 typedef 那种“语序倒装”。

## 别名模板：typedef 做不到的那半步

```cpp
template <class T>
using Vec = std::vector<T>;
```

这句的价值很大。

你终于可以写出“模板的模板别名”。

比如把 allocator、deleter、traits 之类的细节藏起来。

让接口更像你真正想表达的语义。

再举一个更现实的。

你项目里可能有统一的字符串类型。

```cpp
template <class Char>
using BasicStr = std::basic_string<Char>;

using Utf8Str = BasicStr<char>;
```

这就很干净。

没有 `::type`。

没有额外 struct。

## 小洞见

using 的价值不是语法糖。

它是“减少模板噪音”。

模板代码最怕的不是难。

是密。

密到你看不出意图。

using 把意图拉了出来。

而把细节藏在后面。

这就是它在大型工程里。

特别值钱的原因。
