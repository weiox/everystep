---
title: 把 init/free 写进类型，编译器替你收尾。
description: 站在 C 风格 struct 的视角，理解 C++ 中 struct 和 class 的关系、构造函数与析构函数的基本职责，以及如何用最小的面向对象观念配合 RAII 写出更安全的代码。
order: 30
---

在 C 里，你的人生基本被两件小事反复支配：`init` 和 `free`。

写着写着，一个 `struct` 又长出两个字段；  
某个 `malloc` 写完，心想“等会儿再补个 free”；  
函数多加了一个早退分支，`return` 一敲，转头就忘了清理资源。

当时敲代码气势如虹，  
几小时后你在内存泄漏和未初始化的值之间查来查去，  
一边调试一边问自己一句老问题：

> “这破代码是谁写的？”  
> “哦，是三个月前的我自己。”

如果你已经能顺手写出一堆 C 风格的 `struct` 和 `init_xxx / destroy_xxx`，  
C++ 在你面前其实不必一上来就扔“封装继承多态”这一整套。  
你可以先只盯住一件很现实的小事：

> **把 `init` / `free` 写进类型里，让编译器替你收尾。**

这篇文章想做的事，就是把这句话拆开给你看清楚：

- 在 C++ 里，`struct` 和 `class` 本质是一种东西，只是默认设置略有不同；
- 构造函数和析构函数，可以把“初始化”和“清理”这两件最容易忘的小事，  
  从“靠大家讲武德”升级成“写不对就编不过”。

我们不会从“面向对象设计原则”这种大词讲起，而是从你已经熟到闭眼都能写出来的 C 风格 `struct` 开始，一步步把它改造成一个小而完整的 C++ `class`，顺手把 RAII 的味道带出来。

如果一切顺利，读完之后，你再看到某个需要先 `init` 再 `free` 的资源时，脑子里蹦出来的第一反应就会变成：

> “这玩意儿该写成一个小类，让编译器替我盯着。”

### 1. C 风格 struct：只有数据，没有“保证”

先把开头的 `Point` 当成一个“API 套餐”来看：

```c
struct Point {
    double x;
    double y;
};

void init_point(struct Point* p, double x, double y);
void move_point(struct Point* p, double dx, double dy);
void reset_point(struct Point* p);
```

函数名清清楚楚，签名也很整齐，看着安心。  
问题在于：所有真正重要的规则，都写在了**调用者的脑子里**，而不是写在类型上。

你得自己记住：

- 指针不能是空的。
- 必须先 `init_point` 再用，中间随便 `move_point`，用完记得 `reset_point`。
- 某天你给 `Point` 加了个 `double z;`，所有初始化函数都要一起改，少改一个就埋雷。

这些约定，代码本身其实毫不知情。  
项目越大，参与的人越多，“靠大家讲武德”的成本就越高。

一段时间以后，对话往往会变成这样：

> “这里怎么又是未初始化的值？”  
> “谁写的？”  
> “哦，是三个月前的我自己。”

这一节想讲的其实就一句话：

> C 风格的 `struct`，给了你数据，却几乎不给任何“保证”。

C++ 做的其中一件小事，就是让你把这些“保证”写进类型里，而不是写进 README 和聊天记录里。

### 2. C++ 里的 struct 和 class：长得像，心态不一样

先看一个尽量“温柔”的 C++ 版本：

```cpp
struct Point {
    double x;
    double y;

    void move(double dx, double dy) {
        x += dx;
        y += dy;
    }
};
```

这和纯 C 版本相比，就多做了两件事：

- 把操作塞回了 `struct` 里面，变成成员函数。
- 调用方式从 `move_point(&p, dx, dy)` 变成 `p.move(dx, dy)`，看上去是“点在自己移动”。

风格立刻不一样了。  
虽然成员还是完全敞开着，任何地方都能乱改：

```cpp
Point p{0.0, 0.0};
p.x = 123.0;        // 谁都能改
p.move(1.0, 2.0);
```

数据还是“摊平”的，但至少操作已经收拢到对象身上了。

再往前走半步：把 `struct` 换成 `class`，顺手把数据藏起来：

```cpp
class Point {
public:
    void move(double dx, double dy) {
        x_ += dx;
        y_ += dy;
    }

    double x() const { return x_; }
    double y() const { return y_; }

private:
    double x_ = 0.0;
    double y_ = 0.0;
};
```

语法上，变化其实就两层：

- 默认访问控制不同：`struct` 默认 `public`，`class` 默认 `private`。
- 这次我们顺势把成员数据放进了 `private`，外面想改，只能通过成员函数。

从语言角度说，`struct` 和 `class` 几乎是同一种东西。  
但从心理上说，它们代表的是两种不同的态度：

- `struct`：我就是个老老实实的数据袋子，你要怎么拿随你。
- `class`：我不只存数据，我还要对这些数据的**规则**负责。

在很多团队里，约定会变成这样：简单传参用 `struct`，一旦涉及不变式和行为，就用 `class`。  
这一节的核心，其实是给后面铺一层地基：

> 关键不在于“该不该用 class”，而在于“你愿不愿意让类型替你守住某些规则”。

### 3. 构造函数：从一开始就挡掉脏状态

你先看一个很典型的 C 写法：

```c
struct Range {
    int begin;
    int end;
};

void init_range(struct Range* r, int begin, int end) {
    if (!r) return;
    if (begin > end) {
        int tmp = begin;
        begin = end;
        end = tmp;
    }
    r->begin = begin;
    r->end = end;
}
```

这里你心里其实有一条“潜规则”：

> 这个区间必须满足 `begin <= end`。

于是你在 `init_range` 里帮自己收拾烂摊子：  
谁要是传了 `(10, 3)`，你就自动帮他调换一下，保证出去的 `Range` 是干净的。

问题是，这条规则**只写在函数里和你脑子里**，**没写在类型上**。

- 任何人都可以绕开 `init_range`，直接这么玩：

  ```c
  struct Range r;
  r.begin = 10;
  r.end = 3;   // 完全合法的赋值语句，但立刻把不变式搞脏了
  ```

- 你自己也可能忘记调用 `init_range`，或者在某个分支“先用后 init”。

于是，每次你拿到一个 `Range`，都要先在心里打个问号：

> “这个东西现在是干净的，还是已经被谁搞脏了？”

---

换成 C++，我们不再写“init 函数”，而是把这条规则**塞进构造函数**：

```cpp
class Range {
public:
    Range(int begin, int end) {
        if (begin <= end) {
            begin_ = begin;
            end_ = end;
        } else {
            begin_ = end;
            end_ = begin;
        }
    }

    int begin() const { return begin_; }
    int end() const { return end_; }

private:
    int begin_;
    int end_;
};
```

用的时候长这样：

```cpp
Range r(10, 3);  // 你可以乱传，但构造函数会帮你收拾好
```

这里发生了两件很关键的事：

- **没人能绕开初始化了。**  
  你要一个 `Range`，就必须先“通过构造函数这一关”。  
  也就是说：**“先用后 init”在语法层面已经做不到了。**

- **不变式被绑在了“出生”这一步。**  
  构造函数里写死了“无论你怎么传参数，出来的对象都满足 `begin <= end`”。  
  于是你可以大胆假设：

  > 任何一个“活着”的 `Range`，要么是合法区间，要么根本构造不出来。

这就是构造函数真正帮你做的事：

> 把“对象必须是干净的”从“大家记着点”  
> 变成“不过关就生不出来”。

在 C 风格里，你靠纪律提醒所有人按顺序调用 `init`；  
在 C++ 里，你可以把话说狠一点：

> **只要这个对象存在，它就已经通过了体检。**

### 4. 析构函数：收尾这件事，别再靠嘴说

出生的问题解决了，还有一件老大难：收尾。

C 里的资源管理，大多长这样：

```c
struct Buffer {
    size_t size;
    unsigned char* data;
};

void buffer_init(struct Buffer* b, size_t size) {
    b->size = size;
    b->data = malloc(size);
}

void buffer_destroy(struct Buffer* b) {
    free(b->data);
}
```

只要人稍微一累，就可能发生：

- 用完忘记 `buffer_destroy`，一小块内存就这样泄漏了。
- 函数中途多了一条 `return`，结果某个分支根本没走到清理代码。

这些问题，最烦人的地方不在于“难修”，而在于**完全不体面**：  
你知道该怎么写，只是总有一两次没写到。

C++ 给你的新武器叫“析构函数”：

```cpp
class Buffer {
public:
    Buffer(std::size_t size)
        : size_(size), data_(new unsigned char[size]) {
    }

    ~Buffer() {
        delete[] data_;
    }

    std::size_t size() const { return size_; }
    unsigned char* data() { return data_; }

private:
    std::size_t size_ = 0;
    unsigned char* data_ = nullptr;
};
```

使用的时候，世界瞬间清爽了很多：

```cpp
void use_buffer() {
    Buffer buf(1024);
    auto p = buf.data();
    (void)p; // 用就完了
}           // 离开作用域时自动调用 ~Buffer，资源必定被释放
```

你不再需要记住某个 `destroy` 函数名，也不用在所有分支结尾补一段 `free`。  
你只需要记住一句话：

> 资源“跟着对象走”，对象“走完一生”时，资源自然会被释放。

这就是 RAII 的核心味道：  
**把“必须做的清理动作”，写进类型的生命周期里，而不是写进调用者的良心里。**

### 5. 把操作收敛到对象身上：成员函数、this 和“少传一个参数”

再看一眼 C 风格的计数器：

```c
struct Counter {
    int value;
};

void counter_init(struct Counter* c) {
    c->value = 0;
}

void counter_inc(struct Counter* c) {
    ++(c->value);
}

int counter_get(const struct Counter* c) {
    return c->value;
}
```

这组函数有一个明显的共同点：

- 第一个参数永远是 `struct Counter*`，也就是“我要操作的那个对象”。

你可以在脑子里把它们抽象成一种统一的写法：

```c
// 伪代码：
void counter_xxx(struct Counter* self, ...);
```

也就是说，在 C 里，你是**手动**把“当前这一个对象”当成第一个参数传进去的。

到了 C++，我们做的事可以粗暴理解成两步：

1. 把这个“第一个参数 self”藏起来，换一个名字叫 `this`；
2. 把所有以 `counter_` 开头的相关操作，收拢进 `Counter` 这个类型里面。

于是就有了这样的写法：

```cpp
class Counter {
public:
    Counter() : value_(0) {}

    void inc() {          // 原来的 counter_inc
        ++value_;
    }

    int value() const {   // 原来的 counter_get
        return value_;
    }

private:
    int value_;           // 原来的 struct 成员 value
};
```

如果把成员函数的“语法糖”拆开看，你可以这么对照：

```cpp
// C++ 里的成员函数写法：
void Counter::inc() {
    ++value_;
}

// 可以类比成 C 里的：
void counter_inc(Counter* this_) {
    ++(this_->value);
}
```

也就是说，成员函数本质上就是“多了一个默认的第一个参数”，这个参数就是 `this` 指针，指向当前正在被操作的对象。调用方式也从：

```c
struct Counter c;
counter_init(&c);
counter_inc(&c);
int v = counter_get(&c);
```

变成了：

```cpp
Counter c;      // 构造时自动初始化
c.inc();        // 相当于 counter_inc(&c)
int v = c.value();
```

你可以记住一个简单的心智模型：

> 以前写的是：`f(&obj, 其它参数...)`；
> 现在可以写成：`obj.f(其它参数...)`。

从功能上看，差别不大；从阅读和设计上看，差别非常大：

- 你不再到处传一个 `Counter*`，而是直接对 `Counter` 这个对象下指令。
- 想知道一个类型“能干什么”，只要看它的成员函数列表，而不用在各个 `.c` 文件里搜 `counter_` 开头的函数名。
- 成员函数天然可以访问 `private` 成员，于是你可以放心把实现细节藏起来，只暴露真正需要的行为。

这一节想留下的印象是：

> 成员函数 = “把第一个 struct* 参数变成隐式的 `this`，
> 顺便把操作搬进类型里”。

理解到这个程度，其实就足够你在 C 风格代码和 C++ 风格之间来回改写了；从编译器生成代码的角度粗略看，成员函数最终也会被“降解”成带一个隐藏指针参数的普通函数，本质思路和你在 C 里手动传 `struct*` 是一回事。

### 6. 最小 RAII：把“三件套函数”折叠成一个小类

现在把“成员函数 + 构造/析构”一起用一下，做一件 C 程序员天天干的事：
手动管理一块堆内存。

先看一个典型的 C 写法：

```c
struct IntArray {
    size_t size;
    int* data;
};

void int_array_init(struct IntArray* a, size_t size) {
    a->size = size;
    a->data = malloc(size * sizeof(int));
}

void int_array_destroy(struct IntArray* a) {
    free(a->data);
}

int* int_array_at(struct IntArray* a, size_t index) {
    return &a->data[index];
}
```

用的时候，大概是这样一条“仪式流程”：

```c
struct IntArray arr;

int_array_init(&arr, 100);      // 先 init

// 中间各种使用
int* p = int_array_at(&arr, 0);
*p = 42;

int_array_destroy(&arr);        // 最后 destroy
```

只要你哪一步忘了，或者中途提前 `return` 却没补上 `destroy`，就会留下内存泄漏。这就是典型的“init / use / destroy 三件套”。

现在我们用一个最小的 C++ `class`，把这三件事折叠成**一个**类型：

```cpp
class IntArray {
public:
    // 构造函数：等价于 C 里的 int_array_init
    explicit IntArray(std::size_t size)
        : size_(size), data_(new int[size]) {
    }

    // 析构函数：等价于 C 里的 int_array_destroy
    ~IntArray() {
        delete[] data_;
    }

    std::size_t size() const { return size_; }

    // 下标运算符：可以理解成一个更自然的 int_array_at
    int& operator[](std::size_t index) {
        return data_[index];
    }

    const int& operator[](std::size_t index) const {
        return data_[index];
    }

private:
    std::size_t size_ = 0;
    int* data_ = nullptr;
};
```

使用方式也随之改变：

```cpp
void foo() {
    IntArray arr(100);  // 这里自动完成“init”

    arr[0] = 42;        // 访问就像用普通数组

}   // 走到这里，arr 离开作用域，自动调用 ~IntArray，相当于 int_array_destroy(&arr)
```

和刚才的 C 版本对比，你可以直接在脑子里对应起来：

- `IntArray(size)` 构造函数 ≈ `int_array_init(&arr, size)`；
- `~IntArray()` 析构函数 ≈ `int_array_destroy(&arr)`；
- `arr[index]` ≈ `*int_array_at(&arr, index)`。

关键差别不在于“代码是否更短”，而在于：

- 初始化和清理**不再依赖调用者的自觉**，而是绑在了对象的生命周期里；
- 一旦你写成局部对象（栈上变量），作用域一结束，析构一定会被调用，内存一定会被释放；
- 你不需要专门记住 `int_array_destroy` 这个函数名，也不用在每条早退路径上手动补一遍释放逻辑。

这就是最小意义上的 RAII：

> 把“获取/释放资源”这对动作，变成一个类的构造/析构，
> 让资源的生命周期自动跟着对象走，而不是跟着人脑里的 checklist 走。

### 7. 什么时候用 struct，什么时候用 class？

说到这里，可以回过头整理一下心里那条分界线。

你以后在写 C++ 时，经常会遇到这种犹豫：

> “这个东西，要不要整成一个 class？还是随便一个 struct 凑合用算了？”

可以用一个简单的判断顺序来帮自己做决定。

- **只是被动的数据袋子时，用 struct 比较顺手。**  
  比如一个函数需要返回三个简单字段，里面没有什么“不合法态”。这时候用 `struct` 把这些字段绑在一起就够了。你看到的就是“几项数据”，而不是“一个会做事的对象”。

- **一旦你开始在脑子里念叨规则，就该想起 class。**  
  比如“区间的起点必须小于等于终点”“文件必须先打开再读写”“锁加了之后一定要记得解”，这些都是不变式。只要你觉得“忘了做就会炸”，就适合写一个 `class` 把规则固化进去。

- **如果这个东西要负责管理资源，优先考虑 RAII 风格的 class。**  
  动态内存、文件描述符、互斥锁、套接字……只要你看到 `new`、`malloc`、`open`、`lock` 这些字，就尽量想办法让它们出现在构造函数里，让对应的 `delete`、`free`、`close`、`unlock` 出现在析构函数里。

还有一个过渡姿势，也很适合从 C 代码迁移过来的时候用：

> 先在 `struct` 里加成员函数，等不变式越来越清楚，再把成员改成 `private`，顺手引入构造函数和析构函数。

不必一口吃成 OOP 专家。  
先让“类型帮我记住规则”这件事真正落地，已经是质变。

### 8. 读完之后，你可以这样看 struct 和 class

最后，用一张非常粗糙、但好记的图像收个尾。

- 把 `struct` 当成“带字段名的记录”。它更多是在帮你**捆绑数据**，没打算替你承担什么责任。
- 把 `class` 当成“会守规矩的小机器人”。它不仅存数据，还对这些数据的**一生**负责：出生时要合法，活着时可操作，临走前把该收拾的都收拾好。
- 构造函数和析构函数，就是这个机器人入场和退场时的两道必经工序。你把初始化和清理写进去，相当于在类型上刻了两行：  
  “没准备好就不出场。”  
  “不收拾干净不下班。”
- RAII 做的，就是利用这一点，把“资源的生命周期”和“对象的生命周期”强行绑在一起，让资源管理这件烦心事，变成编译器可以帮忙盯的事情。

如果你已经习惯了 C 风格的代码，可以先从很小的地方开始改：

今天写一个 `struct`，顺手在里面放一个成员函数；  
明天遇到需要先 init 再 destroy 的资源时，尝试写一个最小的 RAII `class`；  
慢慢地，你会发现：代码里那些本来靠记忆力维持的约定，开始一点点被挪进类型里。

那一刻起，`class` 对你来说就不再只是“面向对象”的一个名词，而是真的在帮你干活的工具。