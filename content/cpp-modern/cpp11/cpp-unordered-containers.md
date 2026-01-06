---
title: 'unordered 容器：unordered_map / unordered_set'
description: 'C++11 把哈希容器标准化：平均 O(1) 查找很香，但你得理解哈希、负载因子和 rehash 的代价。'
---

我第一次把 `std::map` 换成 `unordered_map`。

是在一个热路径上。

那段代码做的是查表。

查得很多。

改得很少。

map 的树结构很稳。

但 `logN` 在热点上就是肉。

C++11 把哈希容器标准化。

给了 `unordered_map` 和 `unordered_set`。

它们的承诺很诱人。

平均 O(1) 查找。

但它们也更“现实”。

因为性能和行为更依赖数据分布。

## 一个最常见的用法

```cpp
#include <unordered_map>

std::unordered_map<std::string, int> freq;

++freq["alice"];
```

这行写起来很舒服。

你把 key 当索引。

它背后用哈希定位桶。

## 你需要记住的一件事：rehash 会发生

unordered 容器会随着元素增多。

扩大桶数组。

这叫 rehash。

rehash 会让迭代器失效。

也会带来一次性的开销。

如果你大概知道要插多少元素。

你可以提前 reserve。

```cpp
freq.reserve(100000);
```

这在批量导入时特别值。

## 哈希质量决定你的命

unordered 的平均 O(1)。

建立在哈希分布均匀上。

哈希很差。

桶里全挤一堆。

性能就退化。

你可以自定义 hash。

```cpp
struct Key {
    int a;
    int b;
};

struct KeyHash {
    std::size_t operator()(const Key& k) const noexcept {
        return std::hash<int>{}(k.a) ^ (std::hash<int>{}(k.b) << 1);
    }
};

std::unordered_map<Key, int, KeyHash> m;
```

别写得太花。

但也别随便。

哈希写不好。

你得到的不是 unordered。

你得到的是“随机性能”。

## 小洞见

unordered 容器不是 map 的替代品。

它更像另一把刀。

你需要有序遍历。

用 map。

你需要快速查找。

并且不关心顺序。

unordered 往往更合适。

你用对场景。

它会很香。

你用错场景。

它会让你以为自己写了快代码。

其实只是写了不稳定代码。
