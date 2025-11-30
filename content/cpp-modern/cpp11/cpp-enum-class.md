---
title: enum class：更安全的枚举类型
description: 告别传统 enum 的作用域污染和隐式转换陷阱，拥抱 C++11 带来的强类型、强作用域的 enum class。
---

如果你学过 C 或早期的 C++，一定对 `enum` 不陌生：用几个单词把一组相关的整数写在一起，看起来比一堆 `#define` 清爽多了。

但是用久了你会发现，传统的 `enum` 有点“太随便”了：名字乱飘、和整数关系暧昧，在小项目里还能凑合，在大一点的代码库里就开始埋雷。

C++11 给我们带来的 `enum class`，就是在不改变“用枚举表达有限集合”这个核心思想的前提下，给它套上了两道安全锁：**作用域更清晰，类型更严格**。这一节我们就从老的 `enum` 讲起，一步一步走到 `enum class`，中间不跳步。

### 传统 enum：方便，但有点“放飞自我”

先回到最经典的例子：颜色和交通信号灯。假设我们这么写：

```cpp
// colors.h
enum Color {
    Red,
    Green,
    Blue
};
```

在另一个头文件里，我们再定义交通灯的状态：

```cpp
// traffic.h
enum TrafficLight {
    Red,
    Yellow,
    Green
};
```

单看每个头文件都挺合理，但一旦有人在同一个 `.cpp` 里把它们都包含进来：

```cpp
#include "colors.h"
#include "traffic.h"

int main() {
    Color c = Red;
    TrafficLight t = Green;
}
```

编译器马上会抗议，说 `Red` 和 `Green` 被重定义了。

原因其实很简单：传统 `enum` 里的枚举值，会直接“摊”在外层作用域中。`enum Color` 定义完之后，外面就多了三个名字：`Red`、`Green`、`Blue`；`enum TrafficLight` 也想往外丢 `Red`、`Yellow`、`Green`，自然就撞车了。

对读代码的人来说也不友好。你在某个函数里看到一个 `Red`，如果上下文够短，可能还能猜出来它是哪一种 `Red`；但在一个大工程里，有多少人敢拍着胸口说自己一眼就能看出这个 `Red` 属于哪个枚举？

名字的问题还只是表层，更隐蔽的是类型问题。

看一眼最常见的写法：

```cpp
enum Color { Red, Green, Blue };
enum Animal { Dog, Cat, Bird };

void foo() {
    Color c = Blue;   // 实际整数值通常是 2
    Animal a = Dog;   // 实际整数值通常是 0

    if (c == a) {
        // 这段代码从语义上完全没意义，
        // 但在 C++98/03 中是“合法”的
    }
}
```

在传统规则里，`Color` 和 `Animal` 都可以**自动**转换成 `int`。`c == a` 这句，编译器的理解其实是“拿两个整数比一比”。这类代码大多数时候只是逻辑上说不通，但万一整数值碰巧相等，程序还会“看起来正常工作”，直到某天你发现有些分支永远不会走到，才开始怀疑人生。

再看一个更常见的场景：函数参数。

```cpp
void set_color(int color);

void use() {
    Color c = Green;
    set_color(c); // 这是允许的：Color 隐式转成 int
}
```

从类型上看，`set_color` 接受的是一个普通整数；从语义上看，你又希望只传进来合法的颜色值。`enum` 和 `int` 之间这层“随时能变身”的关系，让类型系统帮不上你多少忙。

总结一下：传统 `enum` 用起来很顺手，但有两个根本问题：

一是**枚举值直接跑到外层作用域**，容易和别的枚举撞名；  
二是**可以悄悄变成整数**，类型检查弱，很难帮你挡住离谱的比较和赋值。

这就是 `enum class` 要解决的两件事。

### 第一次见到 enum class：语法长得不太一样

先看最朴素的定义方式，把刚才的颜色改成 `enum class`：

```cpp
enum class Color {
    Red,
    Green,
    Blue
};
```

光看这一句，和传统 `enum` 的区别就是多了一个 `class`。真正的差别在用的时候才体现出来。

如果你像以前那样写：

```cpp
void test() {
    Color c = Red; // ❌ 这一句在 C++11 里编不过
}
```

编译器会说：`Red` 这个名字根本不在当前作用域。此时的 `Red` 不再是一个“散装”的全局名字，而是牢牢地待在 `Color` 这个类型的里面。

正确的写法要加上“前缀”：

```cpp
void test() {
    Color c = Color::Red; // ✅ 必须写成这样
}
```

这就像把原来散落一地的常量，全都装进了一个带名字的盒子里：你再想用其中某个，就必须写出“盒子名 + 里面的名字”。这样一来，`Color::Red` 和 `TrafficLight::Red` 就不会再打架了。

我们把交通灯也改成 `enum class` 看一眼：

```cpp
enum class TrafficLight {
    Red,
    Yellow,
    Green
};

void drive() {
    TrafficLight t = TrafficLight::Red;
    Color c = Color::Red;
}
```

现在，`TrafficLight::Red` 和 `Color::Red` 各回各家，即使它们在同一个源文件里出现，也不会互相污染作用域。你在代码里一看到 `Color::Red`，就知道它指的是“颜色里的红色”；一看到 `TrafficLight::Red`，就知道是“信号灯的红灯”，上下文非常明确。

这就是 `enum class` 的第一重改进：**强作用域（strongly scoped）**。

### 强类型：再也不能随便和 int 暧昧了

`enum class` 的第二重改变，是把枚举从“能随时变成整数”的状态，变成了一个**真正意义上的独立类型**。

把刚才的 `Color` 继续用下去：

```cpp
enum class Color {
    Red,
    Green,
    Blue
};

void check() {
    Color c = Color::Green;

    // if (c == 1) { }          // ❌ 不能和 int 直接比较
    // int n = c;               // ❌ 不能隐式转换为 int

    int n = static_cast<int>(c); // ✅ 需要显式 static_cast
}
```

这里有两个细节值得注意。

第一，`c == 1` 这种写法不再被接受。`c` 是一个 `Color` 类型的变量，只能和 `Color` 里的那些枚举值比较，比如 `Color::Green`。拿它去和整数比，编译器会直接拒绝。

第二，如果你确实需要知道它对应的整数值，比如写日志或者序列化，就必须通过 `static_cast<int>` 显式地转换。也就是说，你要在代码里明确地说出“我现在就是要把这个枚举当整数看”，而不是悄悄地跨过类型系统。

这听起来有点“啰嗦”，但从调错的角度看是件好事：所有“把枚举当整数用”的地方都变得一目了然，出了问题也更容易顺着线往回找。

再看刚才那个最离谱的比较：

```cpp
enum class Color { Red, Green, Blue };
enum class Animal { Dog, Cat, Bird };

void foo() {
    Color c = Color::Blue;
    Animal a = Animal::Dog;

    // if (c == a) { } // ❌ 这在 enum class 语义下根本不合法
}
```

`Color` 和 `Animal` 现在是两种完全不同的类型，哪怕它们底层的整数值刚好一样，也完全不能拿来互相比。编译器会在你写下这句的那一刻就指出来，而不是等到运行时再让你自己去猜为什么逻辑不对。

强作用域 + 强类型，这两点加在一起，已经解决了传统 `enum` 最容易踩的两个坑。

### 指定底层类型：让枚举更“专业”

在 C 时代，`enum` 的底层类型一般就是 `int`，但标准留了点弹性：编译器可以根据实际情况选择一个合适的整数类型。你平时体会不到这点差异，但在需要和硬件寄存器、网络协议、磁盘格式精确对齐的时候，这种“不确定性”就会变成麻烦。

C++11 允许你在 `enum class` 后面直接写出它的底层类型：

```cpp
#include <cstdint>

enum class WeaponType : std::uint8_t {
    Sword,
    Axe,
    Bow
};
```

这里我们把 `WeaponType` 指定为 `std::uint8_t`，也就是一个无符号的 8 位整数。这意味着：

* 这个枚举占用的空间是固定的 1 个字节，在内存、文件或网络上传输时都更可控。
* 当你需要把它和别的二进制数据结构拼在一起时，不用再猜编译器到底给了你多宽的类型。

从写代码的角度看，这一行多出来的 `: std::uint8_t` 并不会增加多少复杂度，但对底层行为的控制力却增加了很多。

更妙的是，**一旦你指定了底层类型，就可以对 `enum class` 进行前向声明**。

### 前向声明：大型项目里的“松耦合”工具

在比较大的项目里，你会努力减少头文件之间的相互包含，只在真正需要实现细节的 `.cpp` 里包含对应的 `.h`。对类来说，我们可以先写个前向声明：

```cpp
class Player;

void kick(Player& p);
```

`enum class` 在指定了底层类型后，也可以有同样的待遇。

假设你有一个 `player.h`，想用 `WeaponType` 表示角色手里的武器，但又不想把整个 `weapon.h` 拉进来：

```cpp
// player.h
#include <cstdint>

enum class WeaponType : std::uint8_t; // 前向声明，只告诉编译器这个类型存在

class Player {
public:
    void equip(WeaponType type);
private:
    WeaponType current_;
};
```

直到实现 `Player::equip` 的时候，你再真正包含 `weapon.h`：

```cpp
// weapon.h
#include <cstdint>

enum class WeaponType : std::uint8_t {
    Sword,
    Axe,
    Bow
};

// player.cpp
#include "player.h"
#include "weapon.h"

void Player::equip(WeaponType type) {
    current_ = type;
}
```

这样一来，`player.h` 对 `weapon.h` 的依赖就被压到了 `.cpp` 层面。修改武器相关的内容时，受影响的编译单元更少，编译速度也更稳定，不容易陷入“改一个头文件，半个工程全重编”的尴尬。

要注意的一点是：**前向声明和完整定义里，底层类型必须一致**。如果前向声明时写了 `: std::uint8_t`，完整定义那里也必须写同样的 `: std::uint8_t`，否则就是未定义行为。

### enum class 做“标志位”：需要你亲口“授权”

很多时候，我们希望一个枚举值不是“互斥的状态”，而是一组可以组合的标志位，比如权限系统：

```cpp
#include <cstdint>

enum class Permission : std::uint8_t {
    None    = 0,
    Read    = 1 << 0, // 0000 0001
    Write   = 1 << 1, // 0000 0010
    Execute = 1 << 2  // 0000 0100
};
```

在传统 `enum` 里，你可以直接写：

```cpp
Permission p = Read | Write; // 直接按位或
```

因为它早就可以隐式当成整数了。

`enum class` 不会这么随便。它不会默认允许你对它做位运算，你必须先告诉它“这是我有意设计成标志位枚举的”。

一个常见的做法是重载按位或运算符：

```cpp
#include <type_traits>

inline Permission operator|(Permission a, Permission b) {
    using U = std::underlying_type_t<Permission>;
    return static_cast<Permission>(
        static_cast<U>(a) | static_cast<U>(b)
    );
}
```

这里的步骤可以慢慢看：

先用 `std::underlying_type_t` 拿到 `Permission` 的底层整数类型（这里是 `std::uint8_t`）；  
再把两个枚举值都转成底层整数；  
用 `|` 做按位或；  
最后再把结果转回 `Permission`。

有了这个重载之后，你就可以很自然地写：

```cpp
Permission p = Permission::Read | Permission::Write;
```

读代码的人一看到这个重载，就会明白这是一个被设计成“位掩码”的枚举类型，比起“任何枚举都可以乱按位或”要安全得多。你还可以按同样的套路重载 `&`、`^`、`~` 等运算符，把整套操作补齐。

### 在 switch 里使用 enum class：和原来几乎一样

很多人第一次接触 `enum class` 时最关心的一点，是它在 `switch` 里的表现。

好消息是：**只要你把 `switch` 的表达式写成枚举变量本身，写法几乎和以前一样**：

```cpp
enum class Direction { North, South, East, West };

void move(Direction dir) {
    switch (dir) {
    case Direction::North:
        // ...
        break;
    case Direction::South:
        // ...
        break;
    case Direction::East:
        // ...
        break;
    case Direction::West:
        // ...
        break;
    }
}
```

和传统 `enum` 的唯一区别，就是 `case` 里要写全名 `Direction::North` 之类的。这样更啰嗦一点，但可读性好很多：你永远不会在一个函数里看到一个孤零零的 `case Red:` 然后到处找它到底是颜色还是灯。

如果你非要拿一个 `int` 去 `switch`，那自然也只能先显式地 `static_cast<int>(dir)`，这和前面讲的“严格类型”是一个道理。

### enum 还要不要用？

听到这里，你可能会有个直觉反应：那以后是不是都该用 `enum class`，把传统 `enum` 扔进历史垃圾堆？

现实情况是：

大多数**自己控制的代码**里，确实可以把 `enum class` 当成默认选择。  
在需要和 C 库交互、必须和老接口保持二进制兼容的地方，传统 `enum` 仍然有用武之地。

比如你要把枚举值传给一个 C 风格的回调函数，它的参数就是 `int`，那用传统 `enum` 会比较自然；或者你要直接把枚举写进某个已经固定格式的二进制协议里，不能随便改它的表示形式，这时候也可以用兼容性更好的老写法。

但只要不用考虑这些外部约束，新代码里选 `enum class` 几乎没有坏处，只是多敲了几个字符而已。

### 小结：让类型系统帮你多看一眼

回顾一下这一路走来的变化。

传统 `enum` 给我们带来了比 `#define` 好很多的可读性，但名字会跑到外层作用域，到处乱撞；枚举值又可以悄悄变成整数，甚至和别的枚举互相比较，这在类型系统眼里问题不大，在程序员眼里却是各种隐性 bug 的温床。

`enum class` 做了两件看似简单的事情：

一是**把枚举值关在自己的作用域里**，必须通过 `Color::Red` 这样的形式访问，不会和别的枚举抢名字。  
二是**把枚举变成真正的独立类型**，既不会自动变成 `int`，也无法和不同的枚举类型混在一起比较，所有“当整数用”的地方都必须显式写出 `static_cast`。

在这两点之上，它还允许你**指定底层类型**，从而支持前向声明和更精细的内存控制；对于需要当“标志位”用的场景，则要求你通过运算符重载亲口“授权”，让危险的用法不再是默认选项。

如果你已经习惯了 C 和 C with class 的写法，刚开始可能会觉得 `enum class` 稍微有点啰嗦。但当项目规模变大、同名的枚举越来越多、类型错误越来越难排查时，你会发现这些小小的多打几次键盘，其实是在提前替自己省时间。

以后当你需要表达“一组有限且有名字的取值”时，可以先问自己一句：这里是不是该用 `enum class`？  
大多数时候，答案都会是“是的”。
