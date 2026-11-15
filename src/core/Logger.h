#pragma once

#include "core/Types.h"

#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace er {

class Logger {
public:
    static Logger& instance();

    bool init();
    void shutdown();

    void info(std::string_view msg);
    void warn(std::string_view msg);
    void error(std::string_view msg);

    std::vector<std::string> recent(usize maxLines = 200) const;
    std::string logPath() const;

private:
    Logger() = default;
    void write(std::string_view level, std::string_view msg);

    mutable std::mutex mutex_;
    void* file_ = nullptr;  // FILE*
    std::string path_;
    std::vector<std::string> ring_;
};

}  // namespace er
