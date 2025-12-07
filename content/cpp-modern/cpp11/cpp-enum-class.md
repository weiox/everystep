---
title: enum class：更安全的枚举类型
description: 告别传统 enum 的作用域污染和隐式转换陷阱，拥抱 C++11 带来的强类型、强作用域的 enum class。
---

还记得第一次接触 `enum` 吗？那时候我们刚从一堆 `#define` 里解放出来，能用几个词把整数包起来，感觉像换了一台新键盘。可几年下来，大家发现传统 `enum` 太像八九十年代的家电：能用，但漏水、漏电，偶尔还冒火星。C++11 给我们塞来一把保险丝——`enum class`。它不改“用枚举表达有限集合”这件事，只是把安全锁拧紧：**作用域关起来，类型收紧了**。

### 传统 enum：名字乱飘、类型随便

那会儿我在一个老项目里修 Bug，颜色和交通灯恰好撞名，经典剧情：

```cpp
// colors.h
enum Color { Red, Green, Blue };

// traffic.h
enum TrafficLight { Red, Yellow, Green };
```

两个头文件都放进一个 `.cpp`：

```cpp
#include "colors.h"
#include "traffic.h"

int main() {
    Color c = Red;
    TrafficLight t = Green;
}
```

编译器立刻红着脸说：`Red`、`Green` 重定义。因为老 `enum` 把枚举值直接摊在外层作用域，谁后到谁尴尬。

更阴的雷在类型上。早年的 C++ 默认让枚举和 `int` 暧昧来往：

```cpp
enum Color { Red, Green, Blue };
enum Animal { Dog, Cat, Bird };

void foo() {
    Color c = Blue;   // 2
    Animal a = Dog;   // 0
    if (c == a) {     // 语义瞎比，但居然能编
    }
}
```

甚至函数参数也一样模糊：

```cpp
void set_color(int color);

void use() {
    Color c = Green;
    set_color(c); // 隐式变 int
}
```

总结那代 `enum` 的通病：

* 名字往外飞，容易撞车；
* 可以偷偷变成整数，类型系统帮不上忙。

### 第一次见到 enum class：名字收盒子里

C++11 把“盒子”加了把锁：

```cpp
enum class Color {
    Red, Green, Blue
};
```

再试图裸用：

```cpp
void test() {
    Color c = Red; // ❌ 找不到 Red
    Color d = Color::Red; // ✅ 必须带前缀
}
```

这就像给每套钥匙挂上标签：`Color::Red` 和 `TrafficLight::Red` 互不干扰。强作用域（strongly scoped）这把锁，解决了“名字乱飘”。

### 强类型：别再和 int 打情骂俏

第二把锁是强类型。`enum class` 不再自动往 `int` 倒：

```cpp
enum class Color { Red, Green, Blue };

void check() {
    Color c = Color::Green;
    // int n = c;      // ❌
    int n = static_cast<int>(c); // ✅ 必须显式说明
}
```

两个不同的枚举更不能互相比：

```cpp
enum class Animal { Dog, Cat, Bird };
void foo() {
    Color c = Color::Blue;
    Animal a = Animal::Dog;
    // if (c == a) { } // ❌ 直接被拦下
}
```

强作用域 + 强类型，把当年的隐形炸雷拆了八成。

### 指定底层类型：写给硬件和协议看的

老一辈编译器会“看心情”决定枚举底层类型。到嵌入式或协议对齐时，这就像出厂没校准的万用表。C++11 允许直接写：

```cpp
#include <cstdint>
enum class WeaponType : std::uint8_t {
    Sword, Axe, Bow
};
```

好处：尺寸固定 1 字节，序列化、对齐都可控。更关键的是，一旦底层类型固定，`enum class` 也能像类一样被前向声明——在大工程里减头文件依赖，编译不至于动不动全仓重建。

### 前向声明：少 include，编译器少喘气

```cpp
// player.h
#include <cstdint>
enum class WeaponType : std::uint8_t; // 先声明

class Player {
public:
    void equip(WeaponType type);
private:
    WeaponType current_;
};
```

实现再去包含定义：

```cpp
// weapon.h
enum class WeaponType : std::uint8_t { Sword, Axe, Bow };

// player.cpp
#include "player.h"
#include "weapon.h"

void Player::equip(WeaponType type) { current_ = type; }
```

记住：前向声明和完整定义的底层类型必须一致，不然就是未定义行为的黑盒。

### 做标志位？请先“授权”

想把枚举当位掩码用，老 `enum` 默认就能 `Read | Write`。`enum class` 则要求你亲口说“我愿意”：

```cpp
#include <cstdint>
enum class Permission : std::uint8_t {
    None = 0, Read = 1 << 0, Write = 1 << 1, Execute = 1 << 2
};

#include <type_traits>
inline Permission operator|(Permission a, Permission b) {
    using U = std::underlying_type_t<Permission>;
    return static_cast<Permission>(
        static_cast<U>(a) | static_cast<U>(b)
    );
}
```

这样写的好处是，所有位运算都变成了显式设计，而不是“顺便就能用”。要补全 `&`、`^`、`~` 也按同样套路。

### switch 里怎么写？多敲几个字而已

`switch` 支持没变，只是 case 要写全名：

```cpp
enum class Direction { North, South, East, West };

void move(Direction dir) {
    switch (dir) {
    case Direction::North: /* ... */ break;
    case Direction::South: /* ... */ break;
    case Direction::East:  /* ... */ break;
    case Direction::West:  /* ... */ break;
    }
}
```

看着啰嗦，其实读起来更安心：不会再看到孤零零的 `case Red:` 然后满屋子找它是谁家的。

### 旧 enum 还要不要？

老派 `enum` 不是被扫地出门，它在两个场景仍有位置：

* 和 C 接口打交道，参数就是 `int`，直接用传统 `enum` 最顺；
* 需要二进制兼容的老协议、老 ABI，别轻易换底层表示。

除此之外，新代码默认上 `enum class`，手感多敲几个字符而已，换来的是少掉一堆莫名其妙的 Bug。

### 小结：多打几次键，省下半天排查

这一轮升级干了两件事：**名字收回自己的屋子，类型不再偷偷变身**。再加上可指定底层类型、支持前向声明，以及位运算要先授权，`enum class` 把老坑基本填平。等你在代码评审里再也遇不到“为什么这里拿 Animal 和 Color 比”的时候，就会感谢当年多打的那几个 `Color::`。
