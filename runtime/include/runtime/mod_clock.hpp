#pragma once
#include <algorithm>
namespace nes::mods {
// Host monotonic seconds and completed simulation units; no hardware clocks changed.
class SessionClock {
public:
 explicit SessionClock(double origin):origin_(origin),last_(origin){}
 void sample(double now,bool paused){now=std::max(now,last_);if(paused_)paused_seconds_+=now-last_;last_=now;paused_=paused;}
 double real_seconds()const{return last_-origin_;}
 double paused_seconds()const{return paused_seconds_;}
private:
 double origin_,last_,paused_seconds_{};bool paused_{};
};
}
