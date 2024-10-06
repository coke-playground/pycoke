#include <iostream>
#include <format>

#include "pycoke.h"

#include "coke/task.h"
#include "coke/queue.h"

using StrQueue = coke::Queue<std::string>;
using StrQueuePtr = std::shared_ptr<StrQueue>;

coke::Task<int> push(StrQueuePtr ptr, std::string value) {
    co_return co_await ptr->push(std::move(value));
}

coke::Task<void> do_work(StrQueuePtr ptr) {
    std::string str;
    while (true) {
        int ret = co_await ptr->pop(str);

        if (ret != coke::TOP_SUCCESS) {
            if (ret == coke::TOP_CLOSED)
                std::cout << std::format("The work ended normally\n");
            else
                std::cout << std::format("The work ended abnormally {}", ret);

            break;
        }

        // Simulate time-consuming processing procedures
        co_await coke::sleep(0.1);

        std::cout << std::format("ProcessSuccess {}\n", str);
    }
}

void init_background_worker(py::module_ &pycoke) {
    py::class_<StrQueue, StrQueuePtr>(pycoke, "StrQueue")
        .def(py::init([](std::size_t max_size) {
            return std::make_shared<StrQueue>(max_size);
        }))
        .def("try_push_back", [](StrQueuePtr ptr, const std::string &value) {
            return ptr->try_push(value);
        }, py::arg("value"))
        .def("push_back", [](StrQueuePtr ptr, const std::string &value) {
            return CokeTaskAwaiter(push(ptr, value));
        }, py::arg("value"))
        .def("close", [](StrQueuePtr ptr) {
            ptr->close();
        });

    pycoke.def("do_work", [](StrQueuePtr ptr) {
        return CokeTaskAwaiter(do_work(ptr));
    });
}
