#include "pycoke.h"
#include "coke/coke.h"

// ExitGuard

std::mutex ExitGuard::mtx;
std::condition_variable ExitGuard::cv;
int ExitGuard::task_count = 0;

void ExitGuard::inc_task_count() {
    std::unique_lock<std::mutex> lk(mtx);
    task_count += 1;
}

void ExitGuard::dec_task_count() {
    std::unique_lock<std::mutex> lk(mtx);
    task_count -= 1;

    if (task_count == 0)
        cv.notify_all();
}


PYBIND11_MODULE(cpp_pycoke, pycoke) {
    pycoke.doc() = "Coke pybind example";

    py::class_<CokeTaskAwaiter>(pycoke, "CokeTaskAwaiter")
        .def("__await__", [](CokeTaskAwaiter &c) -> py::object {
            return c.await();
        });

    pycoke.def("wait", []() {
        py::gil_scoped_release rel;
        ExitGuard::wait();
    });

    init_simple_test(pycoke);
    init_complex_work(pycoke);
    init_background_worker(pycoke);
}
