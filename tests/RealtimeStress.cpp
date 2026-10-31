#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>
#include <iomanip>
#include "Compressor.h"
namespace {
constexpr int maxFrames=128;
struct Case { const char* name; bool rms; bool sc; bool automate; };
struct Instance {
    arson::Compressor engine;
    arson::Parameters p;
    arson::MeterQueue q;
    std::array<float,maxFrames> l{},r{},side{};
    std::array<float*,2> audio{{l.data(),r.data()}};
    std::array<const float*,1> sc{{side.data()}};
    explicit Instance(Case c) {
        p.value[arson::Threshold]=-24;
        p.value[arson::Ratio]=4;
        p.value[arson::Detector]=c.rms?1.f:0.f;
        p.value[arson::AutoRelease]=1;
        p.value[arson::Highpass]=c.sc?100.f:20.f;
        p.value[arson::External]=c.sc?1.f:0.f;
        p.value[arson::Style]=c.sc?2.f:0.f;
        engine.prepare(48000,p);
    }
    void fill(int frames,int block) {
        for(int i=0;i<frames;++i) {
            const auto phase=static_cast<float>((block*frames+i)%2048);
            l[i]=0.48f*std::sin(phase*0.031f);
            r[i]=0.31f*std::sin(phase*0.057f);
            side[i]=0.8f*std::sin(phase*0.073f);
        }
    }
    void process(int frames,bool external) noexcept {
        engine.setParameters(p);
        engine.process(audio.data(),2,frames,external?sc.data():nullptr,
                       external?1:0,&q);
    }
};
void run(Case scenario,int frames,int count) {
    std::vector<std::unique_ptr<Instance>> xs;xs.reserve(count);
    for(int i=0;i<count;++i)xs.emplace_back(std::make_unique<Instance>(scenario));
    for(int k=0;k<200;++k) for(auto& a:xs){a->fill(frames,k);a->process(frames,scenario.sc);}
    constexpr int blocks=1600;
    std::vector<double> durations;durations.reserve(blocks);
    double total=0;
    for(int k=0;k<blocks;++k) {
        for(auto& a:xs) {
            a->fill(frames,k+200);
            if(scenario.automate&&k%8==0)
                a->p.value[arson::Threshold]=(k%16==0)?-32.f:-18.f;
        }
        const auto begin=std::chrono::steady_clock::now();
        for(auto& a:xs)a->process(frames,scenario.sc);
        const auto end=std::chrono::steady_clock::now();
        const double us=std::chrono::duration<double,std::micro>(end-begin).count();
        durations.push_back(us);total+=us;
    }
    for(auto& a:xs) if(!std::isfinite(a->l[frames-1])||!std::isfinite(a->r[frames-1]))
        throw 1;
    std::sort(durations.begin(),durations.end());
    auto percentile=[&](double p){return durations[static_cast<size_t>(std::ceil(p*blocks)-1)];};
    const double budget=frames*1.e6/48000.;
    std::cout<<scenario.name<<" frames="<<frames<<" instances="<<count
             <<std::fixed<<std::setprecision(2)<<" mean="<<total/blocks
             <<"us p99="<<percentile(.99)<<"us p99.9="<<percentile(.999)
             <<"us max="<<durations.back()<<"us p99/deadline="
             <<100.*percentile(.99)/budget<<"%\n";
}
}
int main(){
    std::cout<<"48kHz engine-only wall-clock probe. Includes OS jitter. Not a hardware dropout test.\n";
    try {
        for(const auto scenario: {Case{"Clean Peak",false,false,false},
                                  Case{"Smooth RMS+SC",true,true,false},
                                  Case{"RMS automate",true,true,true}})
            for(int frames: {32,64,128})
                for(int n: {1,8,24})run(scenario,frames,n);
    }catch(...){std::cerr<<"Nonfinite output\n";return 1;}
    std::cout<<"PASS: finite output; actual audio device and host remain untested.\n";
}
