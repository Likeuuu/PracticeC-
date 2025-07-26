#include <iostream>
#include <functional>
#include <thread>
#include <chrono>

// 异步模拟下载函数
void asyncDownload(const std::string& url, std::function<void(std::string)> onComplete) {
    std::thread([=]() {
        std::cout << "Downloading from " << url << "...\n";
        std::this_thread::sleep_for(std::chrono::seconds(2));  // 模拟耗时下载
        std::string result = "Data from " + url;

        // 下载完成，调用回调
        onComplete(result);
    }).detach();  // 分离线程，让它后台运行

}

int main() {
    std::cout << "Starting async download...\n";

    asyncDownload("https://example.com", [](const std::string& data) {
        std::cout << "Download completed! Result:\n" << data << std::endl;
    });

    // 主线程继续做别的事
    std::cout << "Main thread doing other work...\n";

    std::this_thread::sleep_for(std::chrono::seconds(3));  // 等待后台线程完成（临时用法）
    return 0;
}
