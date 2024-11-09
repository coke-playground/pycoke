#ifndef PYCOKE_PYCOKE_H
#define PYCOKE_PYCOKE_H

#include <cstdio>
#include <condition_variable>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>
#include <string_view>

#include "pybind11/pybind11.h"
#include "coke/task.h"

namespace py = pybind11;

constexpr const char *get_file_name(std::string_view sv) {
    const auto last_slash = sv.find_last_of('/');
    if (last_slash == std::string_view::npos)
        return sv.data();
    else
        return sv.data() + last_slash + 1;
}


class ExitGuard {
    static std::mutex mtx;
    static std::condition_variable cv;
    static int task_count;

    static void inc_task_count();
    static void dec_task_count();

public:
    static void wait() {
        std::unique_lock<std::mutex> lk(mtx);
        cv.wait(lk, [](){ return task_count == 0; });
    }

public:
    ExitGuard() { inc_task_count(); }
    ~ExitGuard() { dec_task_count(); }
};


struct PyAwaiterBase {
    static py::object create_future() {
        PYBIND11_CONSTINIT static py::gil_safe_call_once_and_store<py::object> storage;

        auto &asyncio = storage.call_once_and_store_result([]() {
            return py::module_::import("asyncio");
        }).get_stored();

        return asyncio.attr("Future")();
    }

    static void set_future(py::object &fut, const char *attr, py::object val) {
        auto loop = fut.attr("get_loop")();

        py::bool_ is_closed(loop.attr("is_closed")());
        if (is_closed)
            return;

        py::bool_ cancelled(fut.attr("cancelled")());
        if (cancelled)
            return;

        loop.attr("call_soon_threadsafe")(fut.attr(attr), std::move(val));
    }

    virtual py::object await() = 0;
};


template<typename T>
struct ResultHelper {
    std::optional<T> opt;

    void set_value(T &&t) {
        opt.emplace(std::move(t));
    }

    T &value() {
        return opt.value();
    }
};

template<>
struct ResultHelper<void> { };


template<typename T>
struct PyAwaiter : public PyAwaiterBase {
    static coke::Task<> start(coke::Task<T> task, py::object fut);

    PyAwaiter(coke::Task<T> &&task)
        : task(std::move(task))
    { }

    virtual py::object await() override {
        py::object fut = create_future();
        coke::detach(start(std::move(task), fut));

        return fut.attr("__await__")();
    }

private:
    coke::Task<T> task;
};


struct CokeTaskAwaiter {
    template<typename T>
    CokeTaskAwaiter(coke::Task<T> &&task)
        : ptr(std::make_unique<PyAwaiter<T>>(std::move(task)))
    { }

    py::object await() {
        return ptr->await();
    }

private:
    std::unique_ptr<PyAwaiterBase> ptr;
};


void init_simple_test(py::module_ &);
void init_complex_work(py::module_ &);
void init_background_worker(py::module_ &);


template<typename T>
coke::Task<> PyAwaiter<T>::start(coke::Task<T> task, py::object fut) {
    ExitGuard exit_guard;
    ResultHelper<T> val;
    std::exception_ptr eptr;

    try {
        if constexpr (std::is_void_v<T>)
            co_await std::move(task);
        else
            val.set_value(co_await std::move(task));
    }
    catch (...) {
        eptr = std::current_exception();
    }

    // Handle py::object under GIL
    py::gil_scoped_acquire _;

    try {
        const char *attr;
        py::object obj;

        if (eptr) {
            try {
                std::rethrow_exception(eptr);
            }
            catch (...) {
                // TODO
                // I don't know how to implement it without using the detail
                // function yet. If you know, please issue me.
                py::detail::try_translate_exceptions();
                py::error_already_set err;
                attr = "set_exception";
                obj = err.value();
            }
        }
        else {
            attr = "set_result";

            if constexpr (std::is_void_v<T>)
                obj = py::none();
            else
                obj = py::cast(std::move(val.value()));
        }

        set_future(fut, attr, std::move(obj));
    }
    catch (const std::exception &e) {
        fprintf(stderr, "PYCOKE_ERROR %s:%d Exception:%s\n",
                get_file_name(__FILE__), __LINE__, e.what());
        std::terminate();
    }
    catch (...) {
        fprintf(stderr, "PYCOKE_ERROR %s:%d Unknown exception\n",
                get_file_name(__FILE__), __LINE__);
        std::terminate();
    }

    // Release python object in gil
    fut.release().dec_ref();
}

#endif // PYCOKE_PYCOKE_H
