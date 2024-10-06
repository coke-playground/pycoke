#include "pycoke.h"
#include "coke/coke.h"

class MyException : public std::exception {
public:
    MyException(const std::string &err) : err(err) {}

    virtual const char *what() const noexcept override { return err.c_str(); }

private:
    std::string err;
};


coke::Task<> sleep(double sec) {
    co_await coke::sleep(sec);
    co_return;
}

coke::Task<int> return_int(int val) {
    co_await coke::sleep(0.01);
    co_return val;
}

/**
 * ATTENTION: DO NOT USE REFERENCE AS PARAMETERS.
 * The exported coroutine needs to be executed in the background, so the
 * parameters cannot use references to ensure that the life cycle is consistent
 * with the coroutine.
 */
coke::Task<std::string> return_string(std::string val) {
    co_await coke::sleep(0.01);
    co_return val;
}

coke::Task<void> throw_exception(int x) {
    co_await coke::sleep(0.01);

    if (x % 3 == 0)
        throw std::runtime_error("this is runtime error");
    else if (x % 3 == 1)
        throw std::invalid_argument("this is invalid argument error");
    else
        throw MyException("this is my exception");

    co_return;
}


void init_simple_test(py::module_ &pycoke) {
    py::register_exception<MyException>(pycoke, "MyException");

    pycoke.def("sleep", [](double sec) {
        return CokeTaskAwaiter(sleep(sec));
    }, py::arg("sec"));

    pycoke.def("return_int", [](int val) {
        return CokeTaskAwaiter(return_int(val));
    }, py::arg("val"));

    pycoke.def("return_string", [](const std::string &val) {
        return CokeTaskAwaiter(return_string(val));
    }, py::arg("val"));

    pycoke.def("throw_exception", [](int x) {
        return CokeTaskAwaiter(throw_exception(x));
    }, py::arg("x"));
}
