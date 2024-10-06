# Pycoke Example
借助[pybin11](https://github.com/pybind/pybind11)的强大功能，本项目实现了几个在`Python`中异步调用`Coke`协程的示例。众所周知，`Python`语言十分灵活，`C++`运行非常高效，若将业务逻辑中较为稳定的步骤采用`C++`封装成库，而经常变动的步骤借助`Python`灵活调整，便可以同时实现快速开发和高效运行的目标。

## 示例

### 简单函数
代码文件[src/simple_test.cpp](src/simple_test.cpp)中导出了几个简单的异步函数和一个自定义的异常类型，该示例主要介绍了如何向`Coke`传递参数以及向`Python`返回结果，一个简单的函数如下

```cpp
coke::Task<std::string> return_string(std::string val) {
    co_await coke::sleep(0.01);
    co_return val;
}
```

**注意**，在该场景下，被导出的协程总是通过`coke::detach`被放到后台执行，所以协程的参数必须不能使用引用，以保证参数的生命周期正常。

### 复杂任务
代码文件[src/complex_work.cpp](src/complex_work.cpp)中导出了一个较为复杂的异步函数`complex_work`，它接受参数`url`和文件路径`filepath`，先发起网络任务获取HTML，再通过计算任务提取其中的新链接，最后通过文件IO任务将新链接写入到文件中，代码如下

```cpp
coke::Task<> complex_work(std::string url, std::string filepath) {
    std::string html = co_await get_html(url);
    std::vector<std::string> urls = co_await parse_url(html);
    co_await write_to_file(filepath, urls);
}
```

注意到这些被直接调用的协程可以使用引用作为参数，是因为它们会被`complex_work`显式地等待，参数的生命周期必然长于被调用的协程。

### 数据传递
代码文件[src/background_worker.cpp](src/background_worker.cpp)中导出了一个用于传递字符串的异步队列，这种方式可用于先开启一组后台协程，然后通过队列分发任务的场景。

### 其他事项
[test/test.py](test/test.py)中展示了上述几个示例的使用方法。

除此之外，`pycoke.cpp`中额外导出了一个`wait`函数，这时因为`Coke`协程是在后台执行的，当`Python`中发生`TimeoutError`等异常时，`Coke`协程不会被取消，此时若直接退出进程，`Coke`协程会处于非正常状态，并导致进程崩溃。因此需要在进程退出前，通过`pycoke.wait()`显式等待后台任务结束，以安全地退出进程。

## 构建环境
GCC >= 13
Python >= 3.8

同时依赖Python3开发组件头文件和库，build、setuptools、wheel包。

执行下述命令(或其他合理的方式)来构建安装包

```bash
python3 -m build
```

## LICENSE
Apache 2.0
