#include <algorithm>
#include <exception>
#include <format>
#include <string>
#include <vector>
#include <regex>

#include <unistd.h>
#include <fcntl.h>

#include "pycoke.h"
#include "coke/fileio.h"
#include "coke/go.h"
#include "coke/http_client.h"
#include "coke/http_utils.h"
#include "coke/sleep.h"

class TaskException : public std::exception {
public:
    TaskException() = default;
    TaskException(TaskException &&) = default;
    TaskException(const std::string &w) : w(w) { }

    TaskException &operator=(TaskException &&) = default;

    virtual const char *what() const noexcept override {
        return w.c_str();
    }

private:
    std::string w;
};

coke::HttpClientParams params {
    .retry_max = 2,
    .send_timeout = 2000,
    .receive_timeout = 2000,
    .redirect_max = 4,
};

coke::Task<std::string> get_html(const std::string &url) {
    coke::HttpClient cli(params);
    coke::HttpResult res;

    res = co_await cli.request(url);

    if (res.state != coke::STATE_SUCCESS) {
        auto err = std::format("Http request failed state:{} error:{} errstr:{}",
                               res.state, res.error,
                               coke::get_error_string(res.state, res.error));
        throw TaskException(err);
    }

    std::string html;
    for (auto view : coke::HttpChunkCursor(res.resp))
        html.append(view);

    co_return html;
}

coke::Task<std::vector<std::string>> parse_url(const std::string &html) {
    co_await coke::switch_go_thread();

    std::vector<std::string> urls;
    std::regex url_re(R"url_re(https?://[a-zA-Z0-9\./\?=_-]+)url_re");
    std::cregex_iterator it(html.c_str(), html.c_str() + html.size(), url_re), end;

    for (; it != end; ++it) {
        std::cmatch match = *it;
        urls.push_back(match.str());
    }

    std::sort(urls.begin(), urls.end());

    auto uit = std::unique(urls.begin(), urls.end());
    urls.erase(uit, urls.end());

    co_return urls;
}

coke::Task<> write_to_file(const std::string &filepath,
                           const std::vector<std::string> &urls)
{
    int fd = open(filepath.c_str(), O_WRONLY|O_CREAT|O_TRUNC, 0644);
    if (fd < 0)
        throw TaskException(std::format("Open {} error {}", filepath, (int)errno));

    std::unique_ptr<int, void(*)(int *)> fd_guard(&fd, [](int *p) { close(*p); });

    std::string data;
    for (const auto &url : urls)
        data.append(url).append("\n");

    coke::FileResult res;
    const char *p = data.c_str();
    off_t offset = 0, total = (off_t)data.size();

    while (offset < total) {
        res = co_await coke::pwrite(fd, p, total - offset, offset);
        if (res.state != coke::STATE_SUCCESS || res.nbytes == 0)
            throw TaskException(std::format("Write file error {}", res.error));

        offset += res.nbytes;
    }
}

/**
 * This coroutine is executed in the background and does not use reference
 * parameters. The above coroutines will be explicitly co_awaited and can use
 * reference parameters.
 */
coke::Task<> complex_work(std::string url, std::string filepath) {
    std::string html = co_await get_html(url);
    std::vector<std::string> urls = co_await parse_url(html);
    co_await write_to_file(filepath, urls);
}

void init_complex_work(py::module_ &pycoke) {
    py::register_exception<TaskException>(pycoke, "TaskException");

    pycoke.def("complex_work", [](std::string url, std::string filepath) {
        return CokeTaskAwaiter(complex_work(url, filepath));
    });
}
