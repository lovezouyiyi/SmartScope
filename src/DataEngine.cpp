#include "DataEngine.h"

void DataEngine::push(int v) {
    std::lock_guard<std::mutex> l(mtx);
    buf.push_back({t, v});
    t += 0.01;

    if(buf.size() > 2000)
        buf.erase(buf.begin());
}

std::vector<Sample> DataEngine::get() {
    std::lock_guard<std::mutex> l(mtx);
    return buf;
}
