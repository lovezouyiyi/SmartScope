#include "TriggerEngine.h"

void TriggerEngine::reset() {
    preBuf.clear();
    cap.clear();
    triggered=false;
    done=false;
}

void TriggerEngine::push(int v) {
    if(done) return;

    preBuf.push_back(v);
    if(preBuf.size() > pre) preBuf.pop_front();

    if(!triggered) {
        bool hit = rising ?
            (last < level && v >= level) :
            (last > level && v <= level);

        if(hit) {
            triggered=true;
            cap.insert(cap.end(), preBuf.begin(), preBuf.end());
        }
    }

    if(triggered) {
        cap.push_back(v);
        if(cap.size() >= pre + post)
            done=true;
    }

    last=v;
}

bool TriggerEngine::ready() {
    return done;
}
