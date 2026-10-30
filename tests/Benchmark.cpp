#include "Compressor.h"
#include <chrono>
#include <iostream>
int main() {
    constexpr int block=128,iterations=100000;float left[block],right[block],source[block];
    for(int i=0;i<block;++i)source[i]=0.7f*std::sin(i*0.21f);float* data[]{left,right};
    arson::Compressor c;arson::Parameters p;p.value[arson::Detector]=1;p.value[arson::AutoRelease]=1;p.value[arson::Highpass]=100;c.prepare(48000,p);arson::MeterQueue q;
    const auto start=std::chrono::steady_clock::now();
    for(int j=0;j<iterations;++j) {for(int i=0;i<block;++i){left[i]=source[i];right[i]=source[i]*0.5f;}c.process(data,2,block,nullptr,0,&q);}
    const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    std::cout<<"Stereo RMS, auto-release, HP sidechain, 48 kHz / 128 samples\n"<<seconds*1e9/(iterations*block)<<" ns/stereo frame; "<<seconds/(iterations*block/48000.0)*100<<"% of one real-time core budget\n";
    std::cout<<"Platform-specific throughput, not a worst-case callback or hardware guarantee. checksum "<<left[127]<<'\n';
}
