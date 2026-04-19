#pragma once
#include <deque>
#include <vector>

class TriggerEngine {
public:
    int level = 2500;
    bool rising = true;
    int pre = 200;
    int post = 800;

    void push(int v);
    bool ready();
    void reset();

private:
    std::deque<int> preBuf;
    std::vector<int> cap;
    bool triggered=false;
    bool done=false;
    int last=0;
};
