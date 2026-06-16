#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <d3d12.h>
#include <wrl.h>

class Logger {
public:
    static Logger& GetInstance() {
        static Logger instance;
        return instance;
    }

    void Initialize();
    void Finalize();
    void Log(const char* format, ...);
    void CheckDirectXErrors(ID3D12Device* device);

private:
    Logger() = default;
    ~Logger() = default;

    std::ofstream logFile_;
    std::mutex mutex_;
    void* exceptionHandlerHandle_ = nullptr;
};
