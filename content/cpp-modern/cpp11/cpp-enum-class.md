---
title: enum class：更安全的枚举类型
description: 告别传统 enum 的作用域污染和隐式转换陷阱，拥抱 C++11 带来的强类型、强作用域的 enum class。
---

我第一次用 `enum`。

是在一个还带着 C 味道的 C++ 项目里。

那项目很老。

老到你能在角落里闻到宏定义的烟味。

当时我们刚从 `#define RED 1` 这种写法里缓过劲。

看到 `enum`。

心里一乐。

“终于像个人话了。”

`enum` 这东西。

本来就是给人看的。

你把一堆有限的取值。

起了名字。

代码读起来。

就不靠记忆力。

但 `enum` 的老祖宗是 C。

在 C 里。

它基本等价于 `int`。

名字也直接摊在外面。

后来工程变大。

文件变多。

头文件更多。

然后故事就来了。

> 代码会长大。
> 
> 边界不立起来。
> 
> 它们就会在你不注意的时候互相踩脚。

C++11 推出 `enum class` 的时候。

我第一反应不是“新语法真酷”。

我想的是。

“终于有人来收拾这个老摊子了。”

下面就按我踩过的坑来讲。

每个坑都很真实。

也都挺常见。

### 老 enum 的第一个坑：名字在外面飘

我见过最经典的一次翻车。

不是算法。

不是并发。

是两个头文件互相看不顺眼。

```cpp
// colors.h
enum Color { Red, Green, Blue };
```

```cpp
// traffic.h
enum TrafficLight { Red, Yellow, Green };
```

你把它们一起 include。

```cpp
#include "colors.h"
#include "traffic.h"

int main() {
    Color c = Red;
    TrafficLight t = Green;
}
```

编译器当场报警。

因为 `Red`、`Green` 这俩名字没有“姓氏”。

它们不属于 `Color`。

也不属于 `TrafficLight`。

它们属于“全场”。

你可以把它理解成。

老 `enum` 把家门牌号直接贴在小区公告栏上。

谁先来都行。

谁后来谁尴尬。

### 老 enum 的第二个坑：它和 int 太熟了

接下来是更阴的。

不是编译错误。

是“编译通过”。

```cpp
enum Color { Red, Green, Blue };
enum Animal { Dog, Cat, Bird };

bool same(int a, int b) {
    return a == b;
}

void demo() {
    Color c = Blue;   // 2
    Animal a = Dog;   // 0
    (void)same(c, a); // 你觉得这句话有意义吗？
}
```

这不是编译器傻。

这是语言历史包袱。

早年 C/C++ 要照顾很多平台。

枚举就是整数。

很多人也“就这么用”。

更常见的版本是这样。

API 直接收 `int`。

然后枚举一路暗渡陈仓。

```cpp
void set_color(int value);

enum Color { Red, Green, Blue };

void paint() {
    set_color(Green); // 没人拦你
}
```

你看。

调用点很舒服。

但类型系统一点忙都帮不上。

### C++11 的做法：把门关上

`enum class` 做的第一件事。

很简单。

把名字收回去。

```cpp
enum class Color { Red, Green, Blue };
```

然后你再写裸的 `Red`。

它就不让你过。

```cpp
void demo() {
    // Color c = Red;        // ❌
    Color c = Color::Red;   // ✅
}
```

这时候 `Color::Red` 才像一个“完整的人名”。

有名。

也有姓。

顺带一提。

你有时也会看到 `enum struct`。

它和 `enum class` 是一回事。

只是写法不同。

### 再把 int 的暧昧也切断

`enum class` 做的第二件事。

是更关键的。

它不再默认转换成 `int`。

```cpp
enum class Color { Red, Green, Blue };

int to_int(Color c) {
    return static_cast<int>(c);
}
```

注意这里的 `static_cast`。

它像一个签字动作。

你在告诉读代码的人。

也在告诉编译器。

“我知道我在干什么。”

然后两个不同的枚举。

也不会再被拿来硬比。

```cpp
enum class Color  { Red, Green, Blue };
enum class Animal { Dog, Cat, Bird };

void demo() {
    Color c = Color::Blue;
    Animal a = Animal::Dog;
    // if (c == a) {} // ❌ 编译期就拦下
    (void)c;
    (void)a;
}
```

这类错误在代码评审里其实很常见。

尤其是字段名都叫 `type`、`kind`、`status` 的时候。

人眼会走神。

但编译器不会。

### 指定底层类型：写给协议和硬件的人看

有些同学第一次看到 `: std::uint8_t` 会疑惑。

“枚举不是枚举吗？”

我给你一个更工程的解释。

如果你做网络协议。

做存储格式。

做嵌入式寄存器。

你往往需要确定大小。

不然你就会在某个夜里被“对齐”和“字节序”叫醒。

```cpp
#include <cstdint>

enum class WeaponType : std::uint8_t {
    Sword,
    Axe,
    Bow,
};
```

这时它就是 1 字节。

不看编译器心情。

而且底层类型一旦固定。

你还能做前向声明。

这在大工程里很值钱。

### 前向声明：少 include

一切都安静一点。

你在头文件里不一定需要完整定义。

很多时候你只想让编译器知道“有这么个类型”。

```cpp
// player.h
#include <cstdint>

enum class WeaponType : std::uint8_t;

class Player {
public:
    void equip(WeaponType type);
private:
    WeaponType current_;
};
```

实现文件再 include 真正的定义。

```cpp
// weapon.h
#include <cstdint>
enum class WeaponType : std::uint8_t { Sword, Axe, Bow };
```

```cpp
// player.cpp
#include "player.h"
#include "weapon.h"

void Player::equip(WeaponType type) {
    current_ = type;
}
```

这里有个老坑。

前向声明时写的底层类型。

要和定义处一致。

别自作聪明。

### 把 enum class 当位掩码？你得先表态

老 `enum` 时代。

很多人把枚举当 flags 用。

`Read | Write` 一路通关。

`enum class` 不给你这个“顺手”。

你得显式写运算符。

它逼你做设计。

```cpp
#include <cstdint>

enum class Permission : std::uint8_t {
    None    = 0,
    Read    = 1 << 0,
    Write   = 1 << 1,
    Execute = 1 << 2,
};
```

```cpp
#include <type_traits>

inline Permission operator|(Permission a, Permission b) {
    using U = std::underlying_type_t<Permission>;
    return static_cast<Permission>(
        static_cast<U>(a) | static_cast<U>(b)
    );
}
```

这段代码看着麻烦。

但它把“权限可组合”变成了明确的契约。

不是碰巧能用。

你后面要加 `&`、`~` 也同理。

每个运算符都是一次“授权”。

### switch 还是那个 switch

只是名字更像人话。

很多人担心 `switch` 会不会更难写。

其实只是 `case` 要写全名。

```cpp
enum class Direction { North, South, East, West };

void move(Direction dir) {
    switch (dir) {
    case Direction::North:
        break;
    case Direction::South:
        break;
    case Direction::East:
        break;
    case Direction::West:
        break;
    }
}
```

我反而更喜欢这种。

因为你不会在一个几千行的文件里看到 `case Red:`。

然后开始寻亲。

### 旧 enum 还活着吗

活着。

而且活得挺合理。

你要对接 C 接口。

对方函数就是收 `int`。

那用传统 `enum` 也没什么。

你在维护很老的 ABI。

或者二进制兼容是第一原则。

那也别轻易动底层表示。

但如果是新代码。

我自己的习惯很粗暴。

默认 `enum class`。

需要和整数打交道时。

就写显式转换。

别偷懒。

### 小结：这不是“新语法”

是把边界补回来。

`enum class` 没有改变“枚举表达有限集合”这件事。

它只是把两条边界补回来了。

第一条是作用域。

名字不再满天飞。

第二条是类型。

不再和 `int` 暧昧。

你会多敲几个 `::`。

也会多写几个 `static_cast`。

但换来的东西很实在。

编译器帮你挡掉一类低级错误。

你少掉一堆深夜排查。

