#pragma once
#include <vector>
#include <mutex>

struct Sample {
    double t;
    int v;
};

class DataEngine {
public:
    void push(int v);
    std::vector<Sample> get();

private:
    std::vector<Sample> buf;
    std::mutex mtx;
    double t = 0;
};
