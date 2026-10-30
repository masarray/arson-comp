#include "Compressor.h"
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <thread>
#include <vector>
std::atomic<std::size_t> allocations{0};
void* operator new(std::size_t n) { allocations.fetch_add(1); if(auto* p=std::malloc(n ? n : 1)) return p; throw std::bad_alloc{}; }
void* operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p,std::size_t) noexcept { std::free(p); }
void operator delete[](void* p,std::size_t) noexcept { std::free(p); }
int checks=0;
void require(bool ok,const char* message) { ++checks;if(!ok) {std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);} }
bool near(float a,float b,float tolerance=1.0e-4f) { return std::abs(a-b)<=tolerance; }
int main() {
    using namespace arson;
    require(Compressor::latencySamples()==0,"reported zero latency");
    require(near(reduction(0,-20,4,0,60),15),"4:1 static curve");
    require(near(reduction(-20,-20,4,8,60),0.75f),"soft-knee midpoint");
    require(near(reduction(0,-20,4,0,6),6),"range limit");
    for(float knee : {0.0f,6.0f,48.0f}) {
        float previous=-1;
        for(int i=0;i<=1200;++i) {
            const float x=-90+i*0.1f,gr=reduction(x,-20,4,knee,36);
            require(gr>=previous-1e-5f && gr>=0 && gr<=36,"monotonic bounded curve");previous=gr;
        }
        for(float boundary : {-20-knee/2,-20+knee/2}) require(std::abs(reduction(boundary-0.0001f,-20,4,knee,36)-reduction(boundary+0.0001f,-20,4,knee,36))<0.001f,"knee continuity");
    }
    for(double rate : {44100.0,48000.0,96000.0,192000.0}) {
        Compressor c; Parameters p;p.value[Threshold]=-20;p.value[Knee]=0;p.value[Attack]=10;
        c.prepare(rate,p);const int samples=static_cast<int>(rate*0.01);
        std::vector<float> data(static_cast<std::size_t>(samples),1);float* ptr[]{data.data()};
        c.process(ptr,1,samples);
        require(near(-gainToDb(data.back()),15*(1-std::exp(-1.0f)),0.02f),"attack time constant across sample rates");
        data.assign(static_cast<std::size_t>(rate),1);ptr[0]=data.data();c.process(ptr,1,static_cast<int>(data.size()));
        require(near(-gainToDb(data.back()),15,0.02f),"steady compression");
    }
    {
        Compressor c;Parameters p;p.value[Ratio]=1;c.prepare(48000,p);
        float a[]{1,0,0,0},b[]{0.5f,0,0,0};float* ptr[]{a,b};c.process(ptr,2,4);
        require(a[0]==1 && b[0]==0.5f && a[1]==0,"impulse no sample delay and unity ratio");
    }
    {
        Compressor c; Parameters p;p.value[Bypass]=1;p.value[Input]=12;p.value[Makeup]=12;c.prepare(48000,p);
        float a[]{0.123f,-0.4f,0,0.9f};float* ptr[]{a};c.process(ptr,1,4);
        require(near(a[0],0.123f)&&near(a[1],-0.4f)&&near(a[3],0.9f),"bypass ignores input and makeup");
        p.value[Bypass]=0;p.value[Input]=0;p.value[Mix]=0;c.prepare(48000,p);
        float b[]{0.123f,-0.4f};ptr[0]=b;c.process(ptr,1,2);require(b[0]==0.123f && b[1]==-0.4f,"dry mix transparency");
    }
    {
        constexpr int count=8192;std::vector<float> whole(count),split(count);
        for(int i=0;i<count;++i) whole[i]=split[i]=std::sin(i*0.071f)*0.6f;
        Compressor a,b;Parameters p;p.value[AutoRelease]=1;p.value[Detector]=1;p.value[Highpass]=100;a.prepare(48000,p);b.prepare(48000,p);
        p.value[Threshold]=-30;a.setParameters(p);b.setParameters(p);
        float* ptr[]{whole.data()};a.process(ptr,1,count);
        for(int pos=0;pos<count;) {const int n=std::min(37,count-pos);float* part[]{split.data()+pos};b.setParameters(p);b.process(part,1,n);pos+=n;}
        require(whole==split,"block partition invariance with smoothing");
    }
    {
        constexpr int count=48000;std::vector<float> a(count,0.05f),b(count,0.025f),side(count,1);
        float* ptr[]{a.data(),b.data()};const float* sc[]{side.data()};Compressor c;Parameters p;p.value[External]=1;p.value[Threshold]=-20;p.value[Knee]=0;c.prepare(48000,p);
        c.process(ptr,2,count,sc,1);
        require(near(a.back()/0.05f,dbToGain(-15),0.001f),"external mono sidechain drives stereo");
        require(near(a.back(),2*b.back()),"stereo image preserved at full link");
        c.prepare(48000,p);a.assign(count,0.05f);b.assign(count,0.025f);c.process(ptr,2,count);
        require(near(a.back(),0.05f),"disconnected external sidechain is silent");
        p.value[External]=0;p.value[Link]=0;c.prepare(48000,p);a.assign(count,1);b.assign(count,0.01f);c.process(ptr,2,count);
        require(near(b.back(),0.01f),"unlinked low channel uncompressed");
    }
    {
        Compressor c;Parameters p;for(auto& v:p.value)v=std::numeric_limits<float>::quiet_NaN();c.prepare(0,p);
        float a[]{std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN(),1e-38f,1};float* ptr[]{a};c.process(ptr,1,4);
        for(auto v:a)require(std::isfinite(v),"invalid input contained");
        c.process(nullptr,2,128);c.process(ptr,0,0);
    }
    {
        Compressor c;c.prepare(48000);Parameters p;MeterQueue q;float a[128]{};float* ptr[]{a};
        const auto before=allocations.load();
        for(int block=0;block<10000;++block) {p.value[Threshold]=-10-static_cast<float>(block%40);c.setParameters(p);c.process(ptr,1,128,nullptr,0,&q);}
        require(allocations.load()==before,"no allocation in process or parameter updates");
        MeterFrame f;int count=0;while(q.pop(f))++count;require(count==511,"bounded telemetry drops on saturation");
    }
    {
        MeterQueue queue;constexpr int count=100000;std::atomic<bool> good{true};
        std::thread producer([&]{for(int i=0;i<count;++i)while(!queue.push({static_cast<float>(i),0,0}))std::this_thread::yield();});
        std::thread consumer([&]{for(int i=0;i<count;++i){MeterFrame f;while(!queue.pop(f))std::this_thread::yield();if(f.input!=static_cast<float>(i))good=false;}});
        producer.join();consumer.join();require(good,"SPSC publication order");
    }
    std::cout<<"PASS: "<<checks<<" checks\n";
}
