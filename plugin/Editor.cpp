#include "Editor.h"
namespace {
const juce::Colour bg{0xff171a1f},panel{0xff101318},line{0xff303740},text{0xffd4dbe1},muted{0xff89939f},cyan{0xff59c9dc},gold{0xffe4b65e};
}
CompLook::CompLook() {
    setColour(juce::Slider::textBoxTextColourId,text);
    setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId,juce::Colour{0xff252b33});
    setColour(juce::ComboBox::textColourId,text); setColour(juce::ComboBox::outlineColourId,line);
    setColour(juce::PopupMenu::backgroundColourId,bg); setColour(juce::PopupMenu::textColourId,text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId,juce::Colour{0xff34505c});
    setColour(juce::TextButton::buttonColourId,juce::Colour{0xff252b33});
    setColour(juce::TextButton::textColourOffId,text);
    setColour(juce::ToggleButton::textColourId,text); setColour(juce::ToggleButton::tickColourId,cyan);
    setColour(juce::Label::textColourId,muted);
}
void CompLook::drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float start,float end,juce::Slider&) {
    const auto bounds=juce::Rectangle<float>(static_cast<float>(x),static_cast<float>(y),static_cast<float>(w),static_cast<float>(h)).reduced(6);
    const float r=std::min(bounds.getWidth(),bounds.getHeight())*0.5f,cx=bounds.getCentreX(),cy=bounds.getCentreY();
    juce::Path track,active;
    track.addCentredArc(cx,cy,r,r,0,start,end,true);
    active.addCentredArc(cx,cy,r,r,0,start,start+pos*(end-start),true);
    g.setColour(line); g.strokePath(track,juce::PathStrokeType(2.5f));
    g.setColour(cyan); g.strokePath(active,juce::PathStrokeType(2.5f));
    g.setColour(juce::Colour{0xff262d35}); g.fillEllipse(cx-r+5,cy-r+5,2*r-10,2*r-10);
    const float a=start+pos*(end-start);
    g.setColour(text); g.drawLine(cx+std::sin(a)*(r*0.45f),cy-std::cos(a)*(r*0.45f),cx+std::sin(a)*(r*0.75f),cy-std::cos(a)*(r*0.75f),2);
}
Editor::Editor(Processor& p):AudioProcessorEditor(p),processor(p),alternate(p.snapshot()) {
    setLookAndFeel(&look); setOpaque(true);
    for(std::size_t i=0;i<knobs.size();++i) {
        const auto& s=arson::specs[i]; auto& k=knobs[i];
        k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle(juce::Slider::TextBoxBelow,false,90,20);
        k.setTextValueSuffix(s.unit); k.setNumDecimalPlacesToDisplay(i==arson::Attack || i==arson::Ratio ? 2 : 1);
        k.setDoubleClickReturnValue(true,s.initial); k.setScrollWheelEnabled(false);
        k.setName(s.name); k.setTooltip(juce::String(s.name)+" — drag to adjust, double-click to reset; click value to type.");
        addAndMakeVisible(k); labels[i].setText(s.name,juce::dontSendNotification);
        labels[i].setFont(juce::FontOptions(12)); labels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(labels[i]);
        attachments[i]=std::make_unique<SliderAttachment>(p.state,s.id,k);
    }
    presets.addItemList({"Default","Vocal · gentle","Vocal · level","Mix bus · glue","Drums · punch","Podcast · steady"},1);
    presets.setSelectedId(1,juce::dontSendNotification); presets.onChange=[this]{applyPreset(presets.getSelectedId());};
    presets.setTooltip("Factory starting points. Adjust threshold to your input level."); addAndMakeVisible(presets);
    detector.addItemList({"Peak","RMS"},1); addAndMakeVisible(detector);
    detectorAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"detector",detector);
    detector.setTooltip("Peak responds to transients. RMS follows energy over 10 ms without delaying audio.");
    juce::ToggleButton* toggles[]{&autoRelease,&external,&listen,&bypass};
    const char* ids[]{"autoRelease","external","listen","bypass"};
    for(int i=0;i<4;++i) { addAndMakeVisible(*toggles[i]); buttons[static_cast<std::size_t>(i)]=std::make_unique<ButtonAttachment>(p.state,ids[i],*toggles[i]); }
    external.setTooltip("Uses the host sidechain bus. If unconnected, detector receives silence.");
    listen.setTooltip("Audition the filtered detector signal. Turn off for normal output.");
    compare.setTooltip("Swap the current settings with the alternate A/B snapshot.");
    compare.onClick=[this] { const auto current=processor.snapshot(); for(std::size_t i=0;i<arson::Count;++i) setValue(i,alternate[i]); alternate=current; alternateActive=!alternateActive; compare.setButtonText(alternateActive ? "B / A" : "A / B"); presets.setSelectedId(0,juce::dontSendNotification); presets.setText("Custom",juce::dontSendNotification); };
    addAndMakeVisible(compare);
    arson::MeterFrame discard; for(std::uint32_t i=0;i<arson::MeterQueue::capacity && p.meters.pop(discard);++i) {} // discard closed-editor backlog
    presets.setSelectedId(0,juce::dontSendNotification); presets.setText("Custom / restored",juce::dontSendNotification);
    setResizable(true,true); setResizeLimits(860,570,1280,820); setSize(960,600); startTimerHz(30);
}
Editor::~Editor() { stopTimer(); setLookAndFeel(nullptr); }
void Editor::setValue(std::size_t i,float value) {
    if(auto* param=processor.state.getParameter(arson::specs[i].id)) {
        param->beginChangeGesture(); param->setValueNotifyingHost(param->convertTo0to1(value)); param->endChangeGesture();
    }
}
void Editor::applyPreset(int id) {
    if(id<1) return;
    arson::Parameters p;
    if(id==2) { p.value[arson::Ratio]=2.5f;p.value[arson::Attack]=15;p.value[arson::Release]=180;p.value[arson::Knee]=12; }
    if(id==3) { p.value[arson::Threshold]=-24;p.value[arson::Ratio]=3;p.value[arson::Detector]=1;p.value[arson::AutoRelease]=1;p.value[arson::Highpass]=100; }
    if(id==4) { p.value[arson::Ratio]=2;p.value[arson::Attack]=30;p.value[arson::Release]=250;p.value[arson::Knee]=12;p.value[arson::Range]=6;p.value[arson::Highpass]=80; }
    if(id==5) { p.value[arson::Ratio]=5;p.value[arson::Attack]=25;p.value[arson::Release]=80;p.value[arson::Knee]=3;p.value[arson::Mix]=75; }
    if(id==6) { p.value[arson::Threshold]=-24;p.value[arson::Ratio]=4;p.value[arson::Attack]=5;p.value[arson::Release]=180;p.value[arson::Knee]=12;p.value[arson::Highpass]=100;p.value[arson::AutoRelease]=1; }
    presetSnapshot=p;
    for(std::size_t i=0;i<arson::Count;++i) if(i!=arson::External && i!=arson::Listen && i!=arson::Bypass) setValue(i,p[i]);
}
void Editor::resized() {
    presets.setBounds(getWidth()-420,16,210,27); compare.setBounds(getWidth()-198,16,64,27); bypass.setBounds(getWidth()-122,16,100,27);
    display={20,64,getWidth()-40,getHeight()-318};
    const int y=display.getBottom()+20, width=(getWidth()-40)/8;
    const std::array<std::size_t,8> primary{arson::Threshold,arson::Ratio,arson::Attack,arson::Release,arson::Knee,arson::Range,arson::Makeup,arson::Mix};
    for(std::size_t j=0;j<primary.size();++j) {
        const auto i=primary[j]; const int x=20+static_cast<int>(j)*width;
        labels[i].setBounds(x,y,width,18); knobs[i].setBounds(x+6,y+18,width-12,88);
    }
    const int bottom=y+121, small=(getWidth()-300)/5;
    const std::array<std::size_t,5> secondary{arson::Input,arson::Hold,arson::Link,arson::Highpass,arson::Lowpass};
    for(std::size_t j=0;j<secondary.size();++j) {
        const auto i=secondary[j]; const int x=20+static_cast<int>(j)*small;
        labels[i].setBounds(x,bottom,small,16); knobs[i].setBounds(x+12,bottom+16,small-24,72);
    }
    const int x=getWidth()-260;
    detector.setBounds(x,bottom+5,98,25); autoRelease.setBounds(x+106,bottom+5,130,25);
    external.setBounds(x,bottom+41,120,25); listen.setBounds(x+126,bottom+41,110,25);
    grid=juce::Image(juce::Image::RGB,display.getWidth(),display.getHeight(),true);
    juce::Graphics g(grid); g.fillAll(panel); g.setFont(juce::FontOptions(11));
    for(int db=0;db>=-60;db-=12) {
        const float yy=26+static_cast<float>(-db)/60*(display.getHeight()-52);
        g.setColour(line.withAlpha(0.55f)); g.drawHorizontalLine(static_cast<int>(yy),200.0f,static_cast<float>(display.getWidth()-140));
        g.setColour(muted);g.drawText(juce::String(db),display.getWidth()-140,static_cast<int>(yy)-7,28,14,juce::Justification::centredRight);
    }
}
void Editor::timerCallback() {
    if(!isShowing()) return;
    if(presets.getSelectedId()>0) {
        const auto current=processor.snapshot();
        for(std::size_t i=0;i<arson::Count;++i) if(i!=arson::External && i!=arson::Listen && i!=arson::Bypass && std::abs(current[i]-presetSnapshot[i])>0.001f) {
            presets.setSelectedId(0,juce::dontSendNotification); presets.setText("Custom",juce::dontSendNotification); break;
        }
    }
    arson::MeterFrame frame; float in=0,out=0,reduction=0; int received=0;
    // Fixed upper bound; coalesce meters to one UI update, retain 100 Hz trace history.
    while(received<static_cast<int>(arson::MeterQueue::capacity) && processor.meters.pop(frame)) {
        history[head]=frame;head=(head+1)%history.size();++received;
        in=std::max(in,frame.input);out=std::max(out,frame.output);reduction=std::max(reduction,frame.reduction);
    }
    inputDb=std::max(arson::gainToDb(in),inputDb-1.2f); outputDb=std::max(arson::gainToDb(out),outputDb-1.2f); gr=std::max(reduction,gr-0.8f);
    repaint(display);
}
void Editor::paint(juce::Graphics& g) {
    g.fillAll(bg);g.setColour(text);g.setFont(juce::FontOptions(19));g.drawText("ArSonComp",22,16,170,27,juce::Justification::centredLeft);
    g.setColour(muted);g.setFont(juce::FontOptions(11));g.drawText("LIVE COMPRESSOR",170,18,150,24,juce::Justification::centredLeft);
    g.drawImageAt(grid,display.getX(),display.getY());drawDisplay(g);
    g.setColour(line);g.drawHorizontalLine(display.getBottom()+133,20.0f,static_cast<float>(getWidth()-20));
    g.setColour(muted);g.setFont(juce::FontOptions(11));g.drawText("0 samples latency  ·  No lookahead  ·  Native processing",22,getHeight()-25,400,18,juce::Justification::centredLeft);
    g.drawText("Double-click to reset  /  Click value to type",getWidth()-310,getHeight()-25,288,18,juce::Justification::centredRight);
}
void Editor::drawDisplay(juce::Graphics& g) {
    juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion(display);g.setOrigin(display.getPosition());
    const float h=static_cast<float>(display.getHeight()),w=static_cast<float>(display.getWidth());
    const auto p=processor.snapshot();
    g.setFont(juce::FontOptions(11));g.setColour(muted);g.drawText("TRANSFER",16,10,150,18,juce::Justification::centredLeft);
    const juce::Rectangle<float> curve{18,42,154,h-78};
    const auto cy=[&](float db){return curve.getBottom()-juce::jlimit(0.0f,1.0f,(db+60)/60)*curve.getHeight();};
    g.setColour(line);g.drawRect(curve);g.drawLine(curve.getX(),curve.getBottom(),curve.getRight(),curve.getY());
    juce::Path transfer;
    for(int i=0;i<=154;++i) {
        const float db=-60+60.0f*i/154,red=arson::reduction(db,p[arson::Threshold],p[arson::Ratio],p[arson::Knee],p[arson::Range]);
        const float yy=cy(db-red);if(i==0)transfer.startNewSubPath(curve.getX()+i,yy);else transfer.lineTo(curve.getX()+i,yy);
    }
    g.setColour(gold);g.strokePath(transfer,juce::PathStrokeType(2));
    const float dotX=curve.getX()+juce::jlimit(0.0f,1.0f,(inputDb+60)/60)*curve.getWidth();
    const float dotY=cy(inputDb-arson::reduction(inputDb,p[arson::Threshold],p[arson::Ratio],p[arson::Knee],p[arson::Range]));
    g.fillEllipse(dotX-3,dotY-3,6,6);
    g.setColour(muted);g.drawText("−60            Input dB             0",14,static_cast<int>(h)-28,168,18,juce::Justification::centred);
    const float left=200,right=w-148,top=26,bottom=h-26;
    const auto y=[&](float level){return bottom-juce::jlimit(0.0f,1.0f,(level+60)/60)*(bottom-top);};
    juce::Path input,output,reduction;
    for(std::size_t i=0;i<history.size();++i) {
        const auto& f=history[(head+i)%history.size()];const float x=left+(right-left)*static_cast<float>(i)/(history.size()-1);
        const float a=y(arson::gainToDb(f.input)),b=y(arson::gainToDb(f.output)),c=top+std::min(f.reduction/36.0f,1.0f)*(bottom-top);
        if(i==0) { input.startNewSubPath(x,a);output.startNewSubPath(x,b);reduction.startNewSubPath(x,c); }
        else { input.lineTo(x,a);output.lineTo(x,b);reduction.lineTo(x,c); }
    }
    g.setColour(muted.withAlpha(0.5f));g.strokePath(input,juce::PathStrokeType(1));
    g.setColour(cyan);g.strokePath(output,juce::PathStrokeType(1.4f));
    g.setColour(gold);g.strokePath(reduction,juce::PathStrokeType(1.5f));
    g.setColour(gold.withAlpha(0.35f));g.drawHorizontalLine(static_cast<int>(y(p[arson::Threshold])),left,right);
    g.setColour(muted);g.drawText("IN",202,5,30,18,juce::Justification::left);g.setColour(cyan);g.drawText("OUT",238,5,40,18,juce::Justification::left);
    g.setColour(gold);g.drawText("GR  0–36 dB",285,5,110,18,juce::Justification::left);
    const float values[]{inputDb,gr,outputDb};const char* names[]{"IN","GR","OUT"};
    for(int i=0;i<3;++i) {
        const int x=static_cast<int>(w)-106+i*35;
        g.setColour(muted);g.drawText(names[i],x-4,8,34,18,juce::Justification::centred);
        g.setColour(line);g.fillRect(static_cast<float>(x+7),36.0f,9.0f,h-76);
        const float amount=i==1 ? juce::jlimit(0.0f,1.0f,values[i]/36) : juce::jlimit(0.0f,1.0f,(values[i]+60)/60);
        const float bar=(h-76)*amount;g.setColour(i==1 ? gold : cyan);
        g.fillRect(static_cast<float>(x+7),i==1 ? 36.0f : h-40-bar,9.0f,bar);
        g.setColour(text);g.drawText(values[i]<-99 ? "−∞" : juce::String(values[i],1),x-8,static_cast<int>(h)-30,42,18,juce::Justification::centred);
    }
    if(p[arson::Listen]>0.5f || p[arson::Bypass]>0.5f) {
        g.setColour(gold);g.drawText(p[arson::Bypass]>0.5f ? "BYPASSED" : "SIDECHAIN LISTEN",static_cast<int>(left),static_cast<int>(h)-23,250,18,juce::Justification::left);
    }
}
