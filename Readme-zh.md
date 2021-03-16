# fJson (friendly and fast JSON): 致力于平衡易用性和性能的 C++ JSON 库

[Click here for English Version.](Readme.md)

## 如何使用
只需在需要的源代码中引入头文件 `fjson.h` 即可, 下面是一个简单的使用案例
```cpp
// 构造一个 JSON 对象
Json json = {
    {"test", 0.2 + 0.1}, 
    {"pi", 3.14}, 
    {"nested", {
            {"key", "value"}, 
            {"ok", true}
        }
    }, 
    {"1." , 2. }
};
```