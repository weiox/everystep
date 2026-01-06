---
title: 'type traits（<type_traits>）：用编译期信息写更聪明的模板'
description: 'C++11 把模板元编程常用的“类型判断”标准化：is_integral、is_same、enable_if、decay 让泛型代码能讲条件，也能讲约束。'
---

写模板。

你迟早会想问一个问题。

“这个 T 到底是什么？”

它是整数吗。

它能拷贝吗。

它是引用吗。

它是同一个类型吗。

如果你是刚写完 C with classes。

你可能会觉得这些问题很“学院派”。

但在工程里它们出现得特别现实。

比如你写了一个工具函数。

你希望它对 `int` 做一件事。

对 `std::string` 做另一件事。

再比如你写了一个容器封装。

你希望它只接受“可拷贝”的类型。

不然就别让它通过编译。

这种需求。

一句话就是。

你想让编译器替你问问题。

替你守门。

在 C++11 之前。

这些问题当然能问。

但你得靠 Boost。

靠自己写。

靠一堆看起来像魔法的偏特化。

C++11 把这些常用积木搬进了标准库。

放在 `<type_traits>`。

这很像把“祖传招式”收编成正统。

## 最常见的一个：is_integral

```cpp
#include <type_traits>

static_assert(std::is_integral<int>::value, "int is integral");
```

这里的 `.value`。

你可以先把它当成一个编译期的 `bool`。

它不会在运行时算。

它是编译器在编译时就能确定的。

你能在编译期问。

也能在编译期得到答案。

## enable_if：把条件写进模板

你想写一个函数。

只让整数走这条路。

初学阶段。

我更建议你先用 `static_assert`。

因为它报错更直观。

```cpp
template <class T>
T inc(T x) {
    static_assert(std::is_integral<T>::value, "T must be integral");
    return x + 1;
}
```

这段代码表达的意思很清楚。

不是整数。

就别来。

但你很快会遇到另一种需求。

你不是想“报错”。

你是想“换一条重载”。

这时候 `enable_if` 就登场了。

```cpp
template <class T>
typename std::enable_if<std::is_integral<T>::value, T>::type
inc(T x) {
    return x + 1;
}
```

这写法看起来有点绕。

你先别被 `::type` 吓到。

它本质上只是在说。

条件成立。

这个函数就“存在”。

条件不成立。

这个函数就“当作没写过”。

于是重载决议会去找别的版本。

这就是你经常听到的那个词。

SFINAE。

但你不用一开始就背它的全称。

你把它理解成。

“模板在编译期自己退场”。

就够用了。

条件不满足。

这个模板就“不存在”。

重载决议就会去找别的版本。

这就是 SFINAE 的味道。

你不需要在函数体里 if。

你把条件写在类型系统里。

## decay：把 T 变成“适合存储的样子”

模板参数常常带着引用、cv 限定。

你拿来存进容器。

就会不合适。

这个困惑很常见。

你以为自己在存一个值。

结果你不小心存了一个引用。

然后对象一走。

引用就悬空。

`decay` 的直觉很朴素。

它会把类型“揉一揉”。

揉成一个更像值的样子。

`std::decay` 帮你做一次“退火”。

```cpp
template <class T>
struct Box {
    using U = typename std::decay<T>::type;
    U value;
};
```

它会把 `T&` 变成 `T`。

把数组变成指针。

把函数类型变成函数指针。

让它更像“值”。

## 小洞见

type traits 的意义。

不是让你写更复杂的元编程。

而是让你把“前置条件”写清楚。

你不再靠文档写一句。

“T 必须是整数”。

你让编译器替你守门。

而编译器守门。

比任何 code review 都可靠。
