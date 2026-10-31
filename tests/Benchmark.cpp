#include "Compressor.h"
#include <chrono>
#include <cmath>
#include <iostream>

// Compare the three causal styles under the same 48-kHz host workload.
// Each 64-frame block receives an unchanged parameter snapshot, matching
// how JUCE/APVTS is used during ordinary playback.
int main() {
    constexpr int block=64,iterations=50000;
    float left[block]{},right[block]{},source[block]{};
    for(int i=0;i<block;++i)source[i]=0.7f*std::sin(i*0.21f);
    float* data[]{left,right};
    volatile float checksum=0.0f;
    for(int model=0;model<3;++model) {
        arson::Parameters p;
        p.value[arson::Style]=static_cast<float>(model);
        p.value[arson::Detector]=1;
        p.value[arson::AutoRelease]=1;
        p.value[arson::Highpass]=100;
        arson::Compressor c;c.prepare(48000,p);
        arson::MeterQueue q;
        // Warm up once before timing.
        for(int j=0;j<128;++j) {
            for(int i=0;i<block;++i){left[i]=source[i];right[i]=source[i]*0.5f;}
            c.setParameters(p);c.process(data,2,block,nullptr,0,&q);
        }
        const auto start=std::chrono::steady_clock::now();
        for(int j=0;j<iterations;++j) {
            for(int i=0;i<block;++i){left[i]=source[i];right[i]=source[i]*0.5f;}
            c.setParameters(p);c.process(data,2,block,nullptr,0,&q);
        }
        const double secs=std::chrono::duration<double>(
            std::chrono::steady_clock::now()-start).count();
        const double frames=static_cast<double>(iterations)*block;
        checksum+=left[block-1];
        std::cout<<(model==0?"Clean":model==1?"Feedback":"Smooth")
                 <<" | 48 kHz, stereo, 64 frames/block, RMS/Auto/HP,"
                 <<" host parameter snapshot each block: "
                 <<secs*1e9/frames<<" ns/stereo-frame; "
                 <<100.0*secs/(frames/48000.0)
                 <<"% of one real-time core budget\n";
    }
    std::cout<<"Cross-platform comparative microbenchmark, not deadline,"
                " host CPU guarantee or Pro-C performance comparison; checksum "
             <<checksum<<'\n';
}
