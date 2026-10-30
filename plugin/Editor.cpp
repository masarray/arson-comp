#include "Editor.h"
namespace {
// Restrained original vintage palette: graphite, brushed brass, teal meters.
const juce::Colour bg{0xff151715},panel{0xff1b211f},line{0xff3b4841},text{0xffeeeae0},muted{0xffadb6a7},cyan{0xff72cdb5},gold{0xffe4b969};
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
juce::Label* CompLook::createSliderTextBox(juce::Slider& slider) {
    auto* label=juce::LookAndFeel_V4::createSliderTextBox(slider);
    label->setFont(juce::FontOptions(12));
    label->setColour(juce::Label::outlineColourId,juce::Colours::transparentBlack);
    label->setColour(juce::Label::textColourId,text);
    label->setColour(juce::TextEditor::backgroundColourId,panel);
    label->setColour(juce::TextEditor::textColourId,text);
    label->setColour(juce::TextEditor::outlineColourId,line);
    label->setColour(juce::TextEditor::focusedOutlineColourId,cyan);
    return label;
}
void CompLook::drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float start,float end,juce::Slider&) {
    const auto bounds=juce::Rectangle<float>(static_cast<float>(x),static_cast<float>(y),
        static_cast<float>(w),static_cast<float>(h)).reduced(9.0f);
    const float r=std::min(bounds.getWidth(),bounds.getHeight())*0.5f;
    const float cx=bounds.getCentreX(),cy=bounds.getCentreY();
    g.setColour(juce::Colour{0xff202a35});
    g.fillEllipse(cx-r+3,cy-r+3,2*r-6,2*r-6);
    g.setGradientFill(juce::ColourGradient(juce::Colour{0xff3a4652},cx-r,cy-r,
        juce::Colour{0xff121820},cx+r,cy+r,false));
    g.fillEllipse(cx-r+7,cy-r+7,2*r-14,2*r-14);
    juce::Path track,active;
    track.addCentredArc(cx,cy,r,r,0,start,end,true);
    active.addCentredArc(cx,cy,r,r,0,start,start+pos*(end-start),true);
    g.setColour(juce::Colour{0xff485361});
    g.strokePath(track,juce::PathStrokeType(3.2f));
    g.setColour(gold);
    g.strokePath(active,juce::PathStrokeType(3.2f));
    const float angle=start+pos*(end-start);
    g.setColour(text);
    g.drawLine(cx+std::sin(angle)*r*0.35f,cy-std::cos(angle)*r*0.35f,
               cx+std::sin(angle)*r*0.73f,cy-std::cos(angle)*r*0.73f,2.1f);
    g.setColour(juce::Colour{0xff8c9bac});
    g.fillEllipse(cx-1.6f,cy-1.6f,3.2f,3.2f);
}

Editor::Editor(Processor& p):AudioProcessorEditor(p),processor(p),alternate(p.snapshot()) {
    setLookAndFeel(&look); setOpaque(true);
    for(std::size_t i=0;i<knobs.size();++i) {
        const auto& s=arson::specs[i]; auto& k=knobs[i];
        k.setLookAndFeel(&look);
        k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle(juce::Slider::TextBoxBelow,false,90,20);
        k.setTextValueSuffix(s.unit); k.setNumDecimalPlacesToDisplay(i==arson::Attack || i==arson::Ratio ? 2 : 1);
        k.setDoubleClickReturnValue(true,s.initial); k.setScrollWheelEnabled(false);
        k.setName(s.name); k.setTooltip(juce::String(s.name)+" - drag to adjust, double-click to reset; click value to type.");
        addAndMakeVisible(k); labels[i].setText(s.name,juce::dontSendNotification);
        labels[i].setFont(juce::FontOptions(12)); labels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(labels[i]);
        attachments[i]=std::make_unique<SliderAttachment>(p.state,s.id,k);
    }
    style.addItemList({"Clean - feedforward","Feedback - classic","Smooth - dual envelope"},1);
    style.setTooltip("Genuinely different DSP models: causal feed-forward, feedback, and dual-envelope.");
    addAndMakeVisible(style);
    styleAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"style",style);
    presets.addItemList({"Default","Vocal | gentle","Vocal | level","Mix bus | glue","Drums | punch","Podcast | steady"},1);
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
    compact.setComponentID("view.compact");
    compact.setTooltip("Toggle compact hardware-style view without losing settings.");
    compact.onClick=[this] {
        compactMode=!compactMode;
        if(compactMode) {
            expandedWidth=getWidth();expandedHeight=getHeight();
            setResizeLimits(920,380,1320,380);
            compact.setButtonText("FULL");
            setSize(expandedWidth,380);
        } else {
            setResizeLimits(920,620,1320,840);
            compact.setButtonText("MINI");
            setSize(expandedWidth,expandedHeight);
        }
    };
    addAndMakeVisible(compact);
    arson::MeterFrame discard; for(std::uint32_t i=0;i<arson::MeterQueue::capacity && p.meters.pop(discard);++i) {} // discard closed-editor backlog
    presets.setSelectedId(0,juce::dontSendNotification); presets.setText("Custom / restored",juce::dontSendNotification);
    setResizable(true,true); setResizeLimits(920,620,1320,840); setSize(1040,680); startTimerHz(30);
}
Editor::~Editor() {
    stopTimer();
    // Close an active host-automation gesture if the editor is destroyed
    // while a graph drag is in progress.
    if(thresholdDragging)
        if(auto* p=processor.state.getParameter("threshold")) p->endChangeGesture();
    setLookAndFeel(nullptr);
}
void Editor::setValue(std::size_t i,float value) {
    if(auto* param=processor.state.getParameter(arson::specs[i].id)) {
        param->beginChangeGesture(); param->setValueNotifyingHost(param->convertTo0to1(value)); param->endChangeGesture();
    }
}
void Editor::applyPreset(int id) {
    if(id<1) return;
    arson::Parameters p;
    if(id==2) { p.value[arson::Ratio]=2.5f;p.value[arson::Attack]=15;p.value[arson::Release]=180;p.value[arson::Knee]=12;p.value[arson::Style]=2; }
    if(id==3) { p.value[arson::Threshold]=-24;p.value[arson::Ratio]=3;p.value[arson::Detector]=1;p.value[arson::AutoRelease]=1;p.value[arson::Highpass]=100;p.value[arson::Style]=1; }
    if(id==4) { p.value[arson::Ratio]=2;p.value[arson::Attack]=30;p.value[arson::Release]=250;p.value[arson::Knee]=12;p.value[arson::Range]=6;p.value[arson::Highpass]=80;p.value[arson::Style]=2; }
    if(id==5) { p.value[arson::Ratio]=5;p.value[arson::Attack]=25;p.value[arson::Release]=80;p.value[arson::Knee]=3;p.value[arson::Mix]=75; }
    if(id==6) { p.value[arson::Threshold]=-24;p.value[arson::Ratio]=4;p.value[arson::Attack]=5;p.value[arson::Release]=180;p.value[arson::Knee]=12;p.value[arson::Highpass]=100;p.value[arson::AutoRelease]=1;p.value[arson::Style]=2; }
    presetSnapshot=p;
    for(std::size_t i=0;i<arson::Count;++i) if(i!=arson::External && i!=arson::Listen && i!=arson::Bypass) setValue(i,p[i]);
}
void Editor::resized() {
    const int w=getWidth(),h=getHeight();
    compact.setBounds(213,26,50,27);
    style.setBounds(270,25,154,29);
    presets.setBounds(w-426,25,200,29);
    compare.setBounds(w-213,25,69,29);
    bypass.setBounds(w-135,25,113,29);
    display=compactMode ? juce::Rectangle<int>{} : juce::Rectangle<int>{18,66,w-36,h-326};
    const int y=compactMode ? 71 : display.getBottom()+14;
    const int knobWidth=(w-40)/8;
    const std::array<std::size_t,8> primary{arson::Threshold,arson::Ratio,arson::Attack,
        arson::Release,arson::Knee,arson::Range,arson::Makeup,arson::Mix};
    for(std::size_t j=0;j<primary.size();++j) {
        const auto i=primary[j];const int x=20+static_cast<int>(j)*knobWidth;
        labels[i].setBounds(x,y,knobWidth,18);
        knobs[i].setBounds(x+5,y+17,knobWidth-10,89);
    }
    const int bottom=y+118,small=(w-300)/5;
    const std::array<std::size_t,5> secondary{arson::Input,arson::Hold,arson::Link,
        arson::Highpass,arson::Lowpass};
    for(std::size_t j=0;j<secondary.size();++j) {
        const auto i=secondary[j];const int x=20+static_cast<int>(j)*small;
        labels[i].setBounds(x,bottom,small,18);
        knobs[i].setBounds(x+7,bottom+17,small-14,77);
    }
    const int x=w-272;
    detector.setBounds(x,bottom+7,103,26);
    autoRelease.setBounds(x+110,bottom+7,146,26);
    external.setBounds(x,bottom+43,125,26);
    listen.setBounds(x+135,bottom+43,120,26);
    if(compactMode) {grid=juce::Image();return;}
    grid=juce::Image(juce::Image::RGB,display.getWidth(),display.getHeight(),true);
    juce::Graphics g(grid);
    g.fillAll(panel);
    const int graphLeft=290,graphRight=display.getWidth()-132;
    const float top=32.0f,bottomY=static_cast<float>(display.getHeight()-35);
    for(int db=0;db>=-60;db-=12) {
        const float yy=bottomY-static_cast<float>(db+60)/60.0f*(bottomY-top);
        g.setColour(line.withAlpha(0.50f));
        g.drawHorizontalLine(static_cast<int>(yy),static_cast<float>(graphLeft),static_cast<float>(graphRight));
        g.setColour(muted.withAlpha(0.65f));g.setFont(juce::FontOptions(10));
        g.drawText(juce::String(db),graphRight+3,static_cast<int>(yy)-8,23,16,juce::Justification::centred);
    }
    for(int i=1;i<8;++i) {
        const float xx=static_cast<float>(graphLeft)+(graphRight-graphLeft)*i/8.0f;
        g.setColour(line.withAlpha(0.32f));g.drawVerticalLine(static_cast<int>(xx),top,bottomY);
    }
    g.setColour(line);g.drawVerticalLine(272,16.0f,static_cast<float>(display.getHeight()-16));
    g.drawVerticalLine(display.getWidth()-124,16.0f,static_cast<float>(display.getHeight()-16));
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
    if(compactMode) repaint(20,301,getWidth()-40,34);
    else repaint(display);
}
void Editor::paint(juce::Graphics& g) {
    g.fillAll(bg);
    g.setGradientFill(juce::ColourGradient(juce::Colour{0xff2b302a},0.0f,0.0f,bg,0.0f,64.0f,false));
    g.fillRect(0,0,getWidth(),65);
    g.setColour(text);g.setFont(juce::FontOptions(21));
    g.drawText("ArSonComp",21,13,190,29,juce::Justification::centredLeft);
    g.setColour(gold);g.fillRoundedRectangle(23.0f,48.0f,53.0f,2.0f,1.0f);
    g.setColour(muted);g.setFont(juce::FontOptions(10));
    g.drawText("LIVE DYNAMICS",88,44,145,14,juce::Justification::centredLeft);
    g.drawText("MODEL",271,8,146,15,juce::Justification::left);
    g.drawText("FACTORY PRESET",getWidth()-425,8,194,15,juce::Justification::left);
    if(!compactMode) {
        if(grid.isValid())g.drawImageAt(grid,display.getX(),display.getY());
        g.setColour(line);g.drawRoundedRectangle(display.toFloat(),6.0f,1.0f);
        drawDisplay(g);
    }
    g.setColour(line.withAlpha(0.75f));
    g.drawHorizontalLine(compactMode ? 296 : display.getBottom()+123,
        20.0f,static_cast<float>(getWidth()-20));
    if(compactMode) {
        // Compact mode retains essential performance metering rather than
        // becoming a knob-only view. No cached graph is allocated.
        const float values[]{inputDb,gr,outputDb};
        const char* names[]{"IN","GR","OUT"};
        const int section=(getWidth()-40)/3;
        for(int i=0;i<3;++i) {
            const float x=20.0f+static_cast<float>(i*section);
            g.setColour(muted);g.setFont(juce::FontOptions(11));
            g.drawText(names[i],static_cast<int>(x),309,34,20,juce::Justification::left);
            const float bx=x+43.0f,bw=static_cast<float>(section-105);
            g.setColour(line);g.fillRoundedRectangle(bx,314.0f,bw,7.0f,2.0f);
            const float amount=i==1 ? juce::jlimit(0.0f,1.0f,values[i]/36.0f)
                : juce::jlimit(0.0f,1.0f,(values[i]+60.0f)/60.0f);
            g.setColour(i==1 ? gold : cyan);
            if(amount>0.0f)g.fillRoundedRectangle(bx,314.0f,bw*amount,7.0f,2.0f);
            g.setColour(text);g.setFont(juce::FontOptions(11));
            g.drawText(values[i]<-99 ? "-inf" : juce::String(values[i],1),
                static_cast<int>(x)+section-55,307,48,23,juce::Justification::centredRight);
        }
    }
    g.setColour(muted);g.setFont(juce::FontOptions(10.5f));
    g.drawText("0 SAMPLES ADDED   |   REAL-TIME DSP   |   VST3",20,getHeight()-25,455,17,juce::Justification::left);
    g.drawText("Drag controls  /  double-click reset  /  type exact values",
        getWidth()-420,getHeight()-25,400,17,juce::Justification::centredRight);
}

void Editor::drawDisplay(juce::Graphics& g) {
    juce::Graphics::ScopedSaveState save(g);
    g.reduceClipRegion(display);
    g.setOrigin(display.getPosition());
    const float h=static_cast<float>(display.getHeight());
    const float w=static_cast<float>(display.getWidth());
    const auto p=processor.snapshot();
    const float top=32.0f,bottom=h-35.0f;
    g.setFont(juce::FontOptions(11.0f));
    g.setColour(muted);g.drawText("TRANSFER  |  DRAG GOLD MARKER",23,10,250,18,juce::Justification::left);
    g.drawText("DYNAMICS  |  4s",291,10,230,18,juce::Justification::left);

    const juce::Rectangle<float> curve{26.0f,top,229.0f,bottom-top};
    const auto yCurve=[&](float db) {
        return curve.getBottom()-juce::jlimit(0.0f,1.0f,(db+60.0f)/60.0f)*curve.getHeight();
    };
    g.setColour(line.withAlpha(0.75f));g.drawRect(curve,1.0f);
    g.setColour(muted.withAlpha(0.30f));
    g.drawLine(curve.getX(),curve.getBottom(),curve.getRight(),curve.getY(),1.0f);
    const float tx=curve.getX()+juce::jlimit(0.0f,1.0f,(p[arson::Threshold]+60)/60.0f)*curve.getWidth();
    g.setColour(gold.withAlpha(0.30f));g.drawVerticalLine(static_cast<int>(tx),top,bottom);
    juce::Path transfer;
    for(int i=0;i<=229;++i) {
        const float db=-60.0f+60.0f*i/229.0f;
        const float gain=arson::reduction(db,p[arson::Threshold],p[arson::Ratio],p[arson::Knee],p[arson::Range]);
        const float yy=yCurve(db-gain);
        if(i==0)transfer.startNewSubPath(curve.getX(),yy);
        else transfer.lineTo(curve.getX()+i,yy);
    }
    g.setColour(gold.withAlpha(0.18f));g.strokePath(transfer,juce::PathStrokeType(5.0f));
    g.setColour(gold);g.strokePath(transfer,juce::PathStrokeType(1.9f));
    const float dotX=curve.getX()+juce::jlimit(0.0f,1.0f,(inputDb+60)/60)*curve.getWidth();
    const float dotY=yCurve(inputDb-arson::reduction(inputDb,p[arson::Threshold],
        p[arson::Ratio],p[arson::Knee],p[arson::Range]));
    g.setColour(gold.withAlpha(0.25f));g.fillEllipse(dotX-6,dotY-6,12,12);
    g.setColour(juce::Colours::white);g.fillEllipse(dotX-2.2f,dotY-2.2f,4.4f,4.4f);
    g.setColour(muted);g.setFont(juce::FontOptions(10.0f));
    g.drawText("-60 dB",24,static_cast<int>(h)-29,65,17,juce::Justification::left);
    g.drawText("0 dB",190,static_cast<int>(h)-29,65,17,juce::Justification::right);

    // GUI drains a fixed 400-frame telemetry history; no audio-thread graphics.
    const float left=290,right=w-132;
    const auto levelY=[&](float db) {
        return bottom-juce::jlimit(0.0f,1.0f,(db+60.0f)/60.0f)*(bottom-top);
    };
    juce::Path inPath,outPath,grPath,grFill;
    for(std::size_t i=0;i<history.size();++i) {
        const auto& f=history[(head+i)%history.size()];
        const float xx=left+(right-left)*static_cast<float>(i)/(history.size()-1);
        const float iy=levelY(arson::gainToDb(f.input));
        const float oy=levelY(arson::gainToDb(f.output));
        const float ry=top+juce::jlimit(0.0f,1.0f,f.reduction/36.0f)*(bottom-top);
        if(i==0) {
            inPath.startNewSubPath(xx,iy);outPath.startNewSubPath(xx,oy);
            grPath.startNewSubPath(xx,ry);
            grFill.startNewSubPath(xx,top);grFill.lineTo(xx,ry);
        } else {
            inPath.lineTo(xx,iy);outPath.lineTo(xx,oy);
            grPath.lineTo(xx,ry);grFill.lineTo(xx,ry);
        }
    }
    grFill.lineTo(right,top);grFill.closeSubPath();
    g.setGradientFill(juce::ColourGradient(gold.withAlpha(0.16f),left,top,
        gold.withAlpha(0.0f),left,bottom,false));
    g.fillPath(grFill);
    g.setColour(muted.withAlpha(0.48f));g.strokePath(inPath,juce::PathStrokeType(1.05f));
    g.setColour(cyan.withAlpha(0.93f));g.strokePath(outPath,juce::PathStrokeType(1.55f));
    g.setColour(gold);g.strokePath(grPath,juce::PathStrokeType(1.55f));
    g.setColour(gold.withAlpha(0.33f));
    g.drawHorizontalLine(static_cast<int>(levelY(p[arson::Threshold])),left,right);
    g.setFont(juce::FontOptions(10.5f));
    g.setColour(muted);g.drawText("IN",static_cast<int>(left),static_cast<int>(h)-28,36,17,juce::Justification::left);
    g.setColour(cyan);g.drawText("OUT",static_cast<int>(left)+38,static_cast<int>(h)-28,48,17,juce::Justification::left);
    g.setColour(gold);g.drawText("GR",static_cast<int>(left)+93,static_cast<int>(h)-28,38,17,juce::Justification::left);
    const float values[]{inputDb,gr,outputDb};const char* names[]{"IN","GR","OUT"};
    for(int i=0;i<3;++i) {
        const float x=w-116.0f+static_cast<float>(i)*37.0f;
        g.setColour(muted);g.drawText(names[i],static_cast<int>(x)-3,8,31,18,juce::Justification::centred);
        g.setColour(juce::Colour{0xff26313c});g.fillRoundedRectangle(x+7,top,10,bottom-top,3);
        const float fraction=i==1 ? juce::jlimit(0.0f,1.0f,values[i]/36.0f)
            : juce::jlimit(0.0f,1.0f,(values[i]+60.0f)/60.0f);
        const float filled=(bottom-top)*fraction;
        g.setColour(i==1 ? gold : cyan);
        if(filled>0)g.fillRoundedRectangle(x+7,(i==1 ? top : bottom-filled),10,filled,3.0f);
        g.setColour(text);g.setFont(juce::FontOptions(10.0f));
        g.drawText(values[i]<-99 ? "-inf" : juce::String(values[i],1),
            static_cast<int>(x)-6,static_cast<int>(h)-29,39,18,juce::Justification::centred);
    }
    if(p[arson::Listen]>0.5f || p[arson::Bypass]>0.5f) {
        g.setColour(gold);g.setFont(juce::FontOptions(11.0f));
        g.drawText(p[arson::Bypass]>0.5f ? "BYPASSED" : "SIDECHAIN MONITOR",
            static_cast<int>(left)+125,8,210,18,juce::Justification::left);
    }
}
    
// Dragging the gold transfer marker is a real host-automatable control.
// One host gesture covers the entire mouse interaction.
void Editor::mouseDown(const juce::MouseEvent& event) {
    if(compactMode || !display.contains(event.getPosition())) return;
    const auto local=event.getPosition()-display.getPosition();
    if(local.x<26 || local.x>255 || local.y<32 || local.y>display.getHeight()-35) return;
    const float markerX=26.0f+
        juce::jlimit(0.0f,1.0f,(processor.snapshot()[arson::Threshold]+60.0f)/60.0f)*229.0f;
    if(std::abs(static_cast<float>(local.x)-markerX)>14.0f) return;
    if(auto* p=processor.state.getParameter("threshold")) {
        p->beginChangeGesture();
        thresholdDragging=true;
        mouseDrag(event);
    }
}
void Editor::mouseDrag(const juce::MouseEvent& event) {
    if(!thresholdDragging) return;
    const float x=static_cast<float>(event.getPosition().x-display.getX());
    const float threshold=-60.0f+60.0f*juce::jlimit(0.0f,1.0f,(x-26.0f)/229.0f);
    if(auto* p=processor.state.getParameter("threshold"))
        p->setValueNotifyingHost(p->convertTo0to1(threshold));
    repaint(display);
}
void Editor::mouseUp(const juce::MouseEvent&) {
    if(!thresholdDragging) return;
    thresholdDragging=false;
    if(auto* p=processor.state.getParameter("threshold")) p->endChangeGesture();
    setMouseCursor(juce::MouseCursor::NormalCursor);
}
void Editor::mouseMove(const juce::MouseEvent& event) {
    if(compactMode || !display.contains(event.getPosition())) {
        setMouseCursor(juce::MouseCursor::NormalCursor);return;
    }
    const auto local=event.getPosition()-display.getPosition();
    const float markerX=26.0f+
        juce::jlimit(0.0f,1.0f,(processor.snapshot()[arson::Threshold]+60.0f)/60.0f)*229.0f;
    const bool near=local.y>=32 && local.y<=display.getHeight()-35 &&
        std::abs(static_cast<float>(local.x)-markerX)<=14.0f;
    setMouseCursor(near ? juce::MouseCursor::PointingHandCursor
                        : juce::MouseCursor::NormalCursor);
}
