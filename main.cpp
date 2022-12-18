#include <iostream>
#include "threadpool.h"


int main() {
    ThreadPool pool;
    std::vector<std::future<int>> results;
    std::cout << std::thread::hardware_concurrency();
    for (int i = 0; i < 100; i++)
    {
        auto tp = [i] {
            //std::this_thread::sleep_for(std::chrono::seconds(5));
            return i * i;
        };
        auto ans = pool.submit(std::move(tp));
        results.emplace_back(std::move(ans));
    }
    std::cout << "--------------------------------" << std::endl;
    std::cout << results[0].get();
    std::cout << std::endl;

    return 0;
}
