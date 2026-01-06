---
title: '= default 和 = delete：驯服编译器的「小脾气」'
description: 'C++11 用 = default 和 = delete，把“编译器到底替我干了什么”这件事说清楚。'
order: 1
---

我写 C 的那几年。

`struct` 就是一块内存。

你负责填。

编译器不插嘴。

你也不用去猜它的心思。

后来开始写 C++。

一开始还挺爽。

我什么也没写。

对象居然能“默认就能用”。

还能拷贝。

还能赋值。

那会儿我以为。

这就是“语言给的福利”。

直到有一天。

我给类加了一个构造函数。

然后一个老代码突然全红。

编译器说：你没有默认构造。

我当场就愣住了。

“我只是更认真了一点。”

“你怎么还闹脾气了？”

这其实是 C++ 历史遗产的副作用。

从 C 到 C with classes。

再到 C++98。

最后到 C++11。

编译器替你做的事情越来越多。

默认规则也越来越像“规矩”。

规矩一多。

人就容易踩线。

`= default` 和 `= delete` 是 C++11 给出的答案。

它们不神秘。

它们只是让你把话说清楚。

> “让接口更容易被正确使用，让错误更难发生。”
>
> ——Scott Meyers（大意）

这篇文章就按这个思路来。

先讲它们为什么会出现。

再讲它们在真实代码里怎么救命。

### 先把历史讲清楚：编译器为什么会“自作主张”？

在 C 时代。

你写一个 `struct`。

它就是一堆字段。

没有“生命周期”。

也谈不上什么“构造/析构”。

到了 80 年代。

Bjarne Stroustrup 搞出 C with classes。

他要的是一件事。

对象像值一样好用。

创建它。

拷贝它。

离开作用域自动清理。

这样资源管理才靠谱。

于是编译器开始当“管家”。

你不写。

它就悄悄帮你补。

补出来的那几样东西。

后来在 C++98 时代被总结成“Rule of Three”。

到了 C++11，又加上移动语义。

变成“Rule of Five”。

问题也从这里开始。

管家很好。

但管家爱猜。

你一插手。

它就觉得：你要自己来。

然后它把原来那份“默认服务”收回去。

你不一定意识到。

等报错时才发现。

这就是 `= default` 出现的背景。

而 `= delete` 则更像是对 C++98 时代一个“江湖偏方”的官方修正。

以前大家靠 `private` + 不实现。

把复制函数“藏起来”。

能用，但不体面。

也不够直接。

C++11 说：别绕弯。

想禁用就直说。

于是有了 `= delete`。

### 一个看似无辜的改动：为什么多了一个构造函数就编译失败？

先从一个很普通的二维点开始。

你只想存两个整数。

你什么构造函数都不写。

编译器就会给你“白送”一个默认构造。

```cpp
struct Point {
    int x;
    int y;
};

int main() {
    Point p1;      // 默认构造：x 和 y 的值是未初始化的
    Point p2{1, 2}; // 聚合初始化：x = 1, y = 2
}
```

这段代码能跑。

但也埋着“老 C 程序员的经典雷”。

`p1` 的成员未初始化。

所以你开始变得认真。

你说：不行。

我得让每个点都带坐标。

于是你加了一个有参构造。

```cpp
struct Point {
    int x;
    int y;

    Point(int x_, int y_) {
        x = x_;
        y = y_;
    }
};
```

然后你写下面这段。

你以为没问题。

```cpp
int main() {
    Point p1(0, 0);  // 有参构造：没问题
    Point p2;        // 想当然地以为还能这么写
}
```

编译器这时会冷冷地说。

`Point` 没有默认构造函数。

`Point p2;` 不合法。

这就是第一种“编译器小脾气”。

你什么都不写。

它帮你写。

你写了一个构造。

它就觉得：你接管了。

于是它把默认构造收起来。

原因很朴素。

只要你自己声明了任何一个构造函数。

哪怕只是带参数的。

编译器就认为：对象怎么构造，你要自己负责。

它不会替你猜：你是不是还想要那个无参版本。

在 C++98 的年代。

你只能手写一个“空的默认构造”。

```cpp
struct Point {
    int x;
    int y;

    Point() {}                 // 手写一个“什么也不做”的默认构造
    Point(int x_, int y_) {    // 再写一个带参数的
        x = x_;
        y = y_;
    }
};
```

这段代码本身不难。

难的是它很“多余”。

你其实想要的是编译器那一版默认行为。

但你不得不手抄一次。

类一复杂。

这种抄写就是事故源。

改成员时漏改。

或者写了空函数体。

结果把本来可以是“平凡构造”的类型弄成不平凡。

性能调优的时候还得挨个排查。

所以 C++11 给了一个很直白的说法。

我不是忘了写。

我是要你按默认规则生成。

这就是 `= default`。

### 那些年，编译器默默塞给你的“特殊成员函数”

要理解 `= default` 和 `= delete`。

得先承认一件事。

你写一个类。

编译器其实在背后给你配了“隐形员工”。

你不写。

它就生成。

生成的那套东西。

大概就是：默认构造。

复制构造。

复制赋值。

析构。

到了 C++11。

又加上移动构造。

移动赋值。

你平时不一定把它们当回事。

但它们决定了一个类型“像不像值”。

也决定了你会不会突然被报错糊脸。

最简单的例子是这样。

```cpp
struct Widget {
    int value;
};
```

你只写了一个 `int`。

但下面这些操作都没意见。

```cpp
int main() {
    Widget a;          // 从无到有：默认构造
    Widget b = a;      // 从另一个对象“复制”构造
    b = a;             // 赋值：把 a 的内容拷贝给 b
}
```

它能过编译。

不是因为你写得多。

而是因为编译器替你写得多。

当你什么也不写时。

编译器通常会生成一个“成员逐个处理”的朴素版本。

也就是：成员怎么拷贝，它就怎么拷贝。

成员怎么销毁，它就怎么销毁。

从这里开始。

只要你自己写了其中一项。

编译器就会开始猜你的设计意图。

有些函数它就不再自动生成。

你看到的那些“神秘报错”。

多数就是猜错之后的副作用。

### 用 = default 明确地说：“这一项请按标准来”

回到刚才的 `Point`。

你想要两个构造。

无参的。

有参的。

同时你还想让无参那份保持“编译器默认”。

那就写这一句。

```cpp
struct Point {
    int x;
    int y;

    Point() = default;          // 请求编译器生成“标准默认构造”
    Point(int x_, int y_) {     // 自己定义一个带参数的构造
        x = x_;
        y = y_;
    }
};
```

`Point() = default;` 的意思很直白。

这个函数我声明了。

但实现请你按默认规则生成。

别因为我写了别的构造就把它收回去。

这样做的好处也很现实。

你少写一段“空函数体”。

更重要的是。

你把意图写给了读代码的人。

以后再改类。

你不会靠回忆去猜“当年我到底想不想要默认构造”。

`= default` 也不只用在默认构造上。

那些“特殊成员函数”。

你想要哪一个保持默认。

就把哪一个点名。

```cpp
class Buffer {
public:
    Buffer() = default;                          // 默认构造

    Buffer(const Buffer&) = default;             // 复制构造
    Buffer& operator=(const Buffer&) = default;  // 复制赋值

    Buffer(Buffer&&) = default;                  // 移动构造（C++11）
    Buffer& operator=(Buffer&&) = default;       // 移动赋值（C++11）

    ~Buffer() = default;                         // 析构函数
private:
    int size_ = 0;
};
```

你不需要全写。

你也不应该为了“看起来完整”而全写。

你只需要在编译器容易误会的地方。

把话说清楚。

还有一个小技巧。

你可以只在类里声明。

在类外再 `= default`。

读起来更像“我明确用默认实现定义了它”。

```cpp
class Line {
public:
    Line();                     // 只声明，不写实现
private:
    int length_ = 0;
};

// 在类外面说：这个构造函数用默认实现
Line::Line() = default;
```

这里顺便提一句。

在一些追求极致的底层场景里。

“类内 default” 和 “类外 default”。

会影响函数是不是“平凡的”。

你暂时记住一句就够了。

`= default` 的价值不是省代码。

是省猜测。

### 有些对象可以复制，有些对象绝对不能复制

讲完 `= default`。

我们讲更像“救命绳”的那个。

`= delete`。

我见过最多的事故现场。

不是算法写错。

是资源管理类被偷偷复制了一份。

你以为自己拿的是一把钥匙。

结果同一把钥匙被复印了两张。

然后两个人都去开同一把锁。

锁坏不坏不重要。

程序先坏。

比如文件句柄。

先写一个最小的 `File`。

它只做两件事。

构造时打开。

析构时关闭。

```cpp
#include <cstdio>

class File {
public:
    File(const char* filename, const char* mode) {
        handle_ = std::fopen(filename, mode);
    }

    ~File() {
        if (handle_) {
            std::fclose(handle_);
        }
    }

private:
    std::FILE* handle_ = nullptr;
};
```

单独看。

它很正常。

离开作用域自动关闭。

```cpp
void write_something() {
    File f("data.txt", "w");
    // 在这里向文件里写数据……
}
```

你觉得一切都挺好。

直到有人写了“看起来也挺好”的代码。

```cpp
void bad() {
    File f1("data.txt", "w");
    File f2 = f1;  // 看起来只是“复制一个文件对象”
}
```

你并没有写复制构造。

但编译器会帮你生成。

它的生成方式也很“老实”。

逐成员拷贝。

也就是把 `handle_` 那个指针值原封不动复制过去。

于是 `f1` 和 `f2` 指向同一个 `FILE*`。

函数结束。

`f2` 先析构。

`fclose` 一次。

`f1` 再析构。

再 `fclose` 一次。

对同一个句柄关闭两次。

这就是未定义行为。

有时是崩溃。

有时是更糟糕的“带着错误继续跑”。

对这种“独占资源”的类型。

最安全的做法经常不是深拷贝。

而是禁止拷贝。

要么只能移动。

要么只能有一个拥有者。

### C++98 的老办法：把复制函数藏到 private 里

在 C++11 之前。

社区里有个流传很广的套路。

把复制构造和复制赋值声明成 `private`。

然后不实现。

让别人“用不了”。

```cpp
class File {
public:
    File(const char* filename, const char* mode) {
        handle_ = std::fopen(filename, mode);
    }

    ~File() {
        if (handle_) {
            std::fclose(handle_);
        }
    }

private:
    File(const File&);            // 只声明，不实现
    File& operator=(const File&); // 只声明，不实现

    std::FILE* handle_ = nullptr;
};
```

这招能用。

但它有股“江湖味”。

有时是编译阶段报错。

有时是链接阶段报错。

更糟的是。

读代码的人得自己推理。

“哦，这个类应该是不可复制的。”

你看。

意图没有直接写在“接口层”。

而是藏在访问控制里。

这和现代 C++ 的设计哲学有点拧。

所以 C++11 给了一个更直接的表达。

这个函数就是不允许。

别给我生成。

谁调用谁错。

### 用 = delete 明确地说：“这个函数禁止使用”

还是同一个 `File`。

用 C++11 的写法。

你可以把态度写得很硬。

```cpp
class File {
public:
    File(const char* filename, const char* mode) {
        handle_ = std::fopen(filename, mode);
    }

    ~File() {
        if (handle_) {
            std::fclose(handle_);
        }
    }

    File(const File&) = delete;             // 禁止复制构造
    File& operator=(const File&) = delete;  // 禁止复制赋值

private:
    std::FILE* handle_ = nullptr;
};
```

这两行意思很字面。

这个函数被删除了。

它在设计上就不存在。

不是“你访问不到”。

而是“我就是不提供”。

所以当有人写：

```cpp
void bad() {
    File f1("data.txt", "w");
    File f2 = f1;       // 这里会因为调用了被 delete 的构造函数而报错
}
```

编译器会明确告诉你。

你调用了一个被 `= delete` 的函数。

这条错误信息本身就在讲道理。

比链接错误友好太多。

再比如赋值：

```cpp
void bad_assign() {
    File f1("data.txt", "w");
    File f2("data.txt", "w");
    f2 = f1;            // 同样会在编译时报错
}
```

一样当场拦下。

如果你希望它“可以转移所有权”。

那就让它变成只能移动。

一个很常见的写法是手写移动操作。

把指针从对方身上“摘下来”。

```cpp
#include <cstdio>
#include <utility>

class File {
public:
    File(const File&) = delete;
    File& operator=(const File&) = delete;

    File(File&& other) noexcept
        : handle_(std::exchange(other.handle_, nullptr)) {}

    File& operator=(File&& other) noexcept {
        if (this != &other) {
            if (handle_) std::fclose(handle_);
            handle_ = std::exchange(other.handle_, nullptr);
        }
        return *this;
    }

    // 省略构造/析构...
private:
    std::FILE* handle_ = nullptr;
};
```

这样 `File` 就不会被复制。

但可以被移动。

你拿走资源。

对方就清空。

这才符合“独占句柄”的语义。

还有一种更常见的工程做法。

直接用标准库的 `std::unique_ptr` 搭配自定义 deleter。

那更接近“Rule of Zero”。

不过我们今天的主角不是智能指针。

主角是：用语言把意图写死。

你经常还会看到一个更朴素的基类。

它专门表达“不可复制”。

```cpp
class NonCopyable {
public:
    NonCopyable() = default;
    ~NonCopyable() = default;

    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};
```

继承它的类。

就天然不可复制。

读者也不会误会。

### 不只是类成员：用 = delete 禁止某些调用方式

`= delete` 还有个我很喜欢的用法。

它可以帮你“封口”。

封掉某些你不希望发生的调用。

比如隐式类型转换。

你想让函数只接收 `double`。

不想让 `int` 这种调用悄悄被转换进来。

你可以这么写。

```cpp
 #include <cstdio>

void print_double(double x) {
    std::printf("value = %f\n", x);
}

void print_double(int) = delete;
```

你提供了一个正常重载。

再提供一个“被删除”的重载。

然后看调用方。

```cpp
int main() {
    print_double(3.14);  // 调用 double 版本：正常
    print_double(10);    // 尝试调用 int 版本：编译错误
}
```

`3.14` 正常。

`10` 本来可以转成 `double`。

但因为你专门给 `int` 开了一个“禁用通道”。

编译器会优先匹配到这个更精确的重载。

然后发现它是 `= delete`。

于是直接报错。

这招很像你在接口上贴了一张纸。

“这个入口我故意留着。”

“但我不让你走。”

你也可以用同样的方法。

封掉危险的指针转换。

封掉你不希望发生的隐式构造。

只要你认为：从语义上讲，它就不该被调用。

那就 `= delete`。

### 小结：= default 和 = delete 都是在“跟编译器讲清楚话”

从 C 到 C with classes。

再到 C++11。

编译器帮你干的事情越来越多。

默认规则也越来越复杂。

`= default` 和 `= delete` 看起来只是两个小标记。

但它们带来的是一种更“显式”的风格。

当你写下 `= default`。

你是在说。

这一项请按默认规则生成。

我不是忘了写。

我是要把它写得谁都看得见。

当你写下 `= delete`。

你是在说。

这个接口从语义上就不该被调用。

谁调用谁错。

请在编译阶段就拦住。

理解了这两句。

你再遇到那种老问题。

“我只是多写了一个构造，怎么默认构造没了？”

“这个资源类怎么被复制了一份？”

你就不用猜编译器的脾气。

你只要把意图写进接口。

写死。

让编译器替你当坏人。