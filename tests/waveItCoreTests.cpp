#include "../src/waveItCore.h"
#include <cassert>
#include <iostream>
#include <limits>
using namespace waveItCore;
static bool near(double a, double b, double tolerance = 1e-6) { return std::abs(a-b) < tolerance; }
static void motionTests() {
    const std::vector<float> x{0,.25f,1}, y{0,0,0};
    Simulation sim;
    auto tick=[&](float height, const std::vector<float>& influence, double dt=1.0/60,
                  bool spring=true, const std::vector<float>& stiffness=std::vector<float>{40}) {
        return sim.update(x,{height,height,height},{.5f},{0},stiffness,spring,dt,influence);
    };
    auto r=tick(0,{1}); assert(r.y==y); // no startup kick
    r=tick(.1f,{1});
    assert(r.y[0]>0 && r.y[0]<.01); // moving body lags even with Amount zero
    bool rebounded=false;
    for(int i=0;i<600;++i) { r=tick(.1f,{1}); rebounded |= r.y[0]>.1f; }
    assert(rebounded && near(r.y[0],.1));
    sim.reset(); tick(0,{0,1}); r=tick(.1f,{0,1});
    const double quarterLag=.1-r.y[1], fullLag=.1-r.y[2];
    assert(near(r.y[0],.1) && near(quarterLag,fullLag*.25));
    sim.reset(); tick(0,{1}); r=tick(.1f,{0});
    assert(near(r.y[0],.1)); r=tick(.1f,{1}); assert(near(r.y[0],.1)); // no replay
    sim.reset(); tick(0,{1},1.0/60,false); r=tick(.5f,{1});
    assert(near(r.y[0],.5)); // entering Spring rebases motion
    sim.reset(); tick(0,{1}); r=tick(.5f,{1},1.0/60,false);
    assert(near(r.y[0],.5)); // Direct ignores influence
    sim.reset(); tick(0,{1}); r=tick(.1f,{1},1.0/60,true,{0});
    assert(near(r.y[0],.1)); // zero stiffness retains hold semantics
    sim.reset(); tick(0,{1});
    r=sim.update({.2f,.45f,1.2f},y,{.5f},{0},{40},true,1.0/60,{1});
    assert(r.y==y); // purely tangential movement gives no normal excitation
    for(double dt : {0.,-1.,1.,std::numeric_limits<double>::quiet_NaN()}) {
        sim.reset(); tick(0,{1}); r=tick(.5f,{1},dt); assert(near(r.y[0],.5));
        r=tick(.5f,{1}); assert(near(r.y[0],.5)); // rebase on invalid/gap time
    }
    sim.reset(); tick(0,{1});
    r=sim.update({0,1},{.5f,.5f},{.5f},{0},{40},true,1.0/60,{1});
    assert(near(r.y[0],.5)); // topology change rebases instead of teleport kick
    sim.update({0},{},{.5f},{0},{40},true,1.0/60,{1});
    r=tick(.7f,{1}); assert(near(r.y[0],.7)); // invalid input clears history
    sim.reset(); tick(0,{1}); sim.reset(); r=tick(.7f,{1}); assert(near(r.y[0],.7));
    sim.reset();
    auto paths=[&](float h) {
        return sim.update({0,.25f,1,-1,0,.5f,1}, {h,h,h,-1,h,h,h},
                          {.5f},{0},{40},true,1.0/60,{0,1});
    };
    paths(0); r=paths(.1f);
    assert(r.x[3]==-1 && r.y[3]==-1 && near(r.y[2],r.y[6]));
    assert(near(.1-r.y[5],2*(.1-r.y[1]))); // independent per-path mapping
    sim.reset();
    auto square=[&](float h) {
        return sim.update({0,1,1,0,0,0},{h,h,1+h,1+h,h,h},
                          {.5f},{0},{40},true,1.0/60,{0,1});
    };
    square(0); r=square(.1f);
    assert(r.x[0]==r.x[4] && r.x[4]==r.x[5] && r.y[0]==r.y[5]);
    assert(r.y[1]<.1f); // translation projected onto closed-path normals
    // Same constant-speed trajectory at two rates: exact moving-rest forcing.
    auto translate=[&](int hz) {
        Simulation motion;
        motion.update(x,y,{.5f},{0},{40},true,1.0/hz,{1});
        Result out;
        for(int i=1;i<=hz;++i) {
            const float h=.2f*i/hz;
            out=motion.update(x,{h,h,h},{.5f},{0},{40},true,1.0/hz,{1});
        }
        return out.y[0];
    };
    assert(near(translate(30),translate(120),2e-7));
    for(float k : {1e-30f,1e-5f,40.f,1000.f}) {
        sim.reset(); tick(0,{1}); r=tick(.1f,{1},1.0/60,true,{k});
        assert(std::isfinite(r.y[0]) && r.y[0]<.1f);
    }
    // Explicit zero influence preserves the original spring trajectory exactly.
    Simulation original, zero;
    for(int i=0;i<60;++i) {
        const float h=.1f*i/60;
        auto a=original.update(x,{h,h,h},{1},{.1f},{40},true,1.0/60);
        auto b=zero.update(x,{h,h,h},{1},{.1f},{40},true,1.0/60,{0});
        assert(a.x==b.x && a.y==b.y);
    }
    const float nan=std::numeric_limits<float>::quiet_NaN();
    sim.reset(); tick(0,{1}); r=tick(.1f,{nan}); assert(near(r.y[0],.1));
    sim.reset(); tick(0,{1}); r=tick(.1f,{}); assert(near(r.y[0],.1));
    std::cout << "Wave It motion: lag/rebound, distance masks, tangential motion, closure, resets, mode switches, timing gaps, soft springs and 30/120 fps agreement passed.\n";
}

int main() {
    motionTests();
    const std::vector<float> x{0,.1f,.7f,1}, y{0,0,0,0};
    auto r = process(x,y,{0,1},.2f);
    for(size_t i=0;i<x.size();++i) {
        assert(r.x[i] == x[i]); assert(near(r.y[i],(2*x[i]-1)*.2));
    }
    assert(process(x,y,{.5f},.2f).y == y);
    assert(process(x,y,{},.2f).x == x);
    assert(process(x,y,{1},0).y == y);
    r = process({0,0,0},{0,.3f,1},{1},.1f);
    for(float v:r.x) assert(near(v,-.1)); // perpendicular to a vertical body
    r = process({0,1,-1,0,.5f,1,-1},{0,0,-1,1,1,1,-1},{0,1},.2f);
    assert(r.x.size()==7 && r.x[2]==-1 && r.y[2]==-1 && r.y[6]==-1);
    assert(near(r.y[0],-.2) && near(r.y[1],.2));
    assert(near(r.y[3],.8) && near(r.y[4],1) && near(r.y[5],1.2));
    // Outward square normals, both windings; closure must be bit-identical.
    for(bool reverse:{false,true}) {
        std::vector<float> sx{0,1,1,0,0}, sy{0,0,1,1,0};
        if(reverse) { std::reverse(sx.begin(),sx.end()); std::reverse(sy.begin(),sy.end()); }
        r=process(sx,sy,{1},.1f);
        assert(r.x.front()==r.x.back() && r.y.front()==r.y.back());
        for(size_t i=0;i<4;++i) {
            assert((r.x[i]-.5)*(sx[i]-.5) > .25);
            assert((r.y[i]-.5)*(sy[i]-.5) > .25);
            assert(near(std::hypot(r.x[i]-sx[i],r.y[i]-sy[i]),.1));
        }
    }
    // User example: 1000 samples / 3 cycles mapped onto 100 circle vertices.
    const double pi=std::acos(-1.0);
    std::vector<float> wave(1000), cx,cy;
    for(size_t i=0;i<wave.size();++i) wave[i]=.5+.5*std::sin(6*pi*i/wave.size());
    for(int i=0;i<100;++i) { cx.push_back(.5+.3*std::cos(2*pi*i/100)); cy.push_back(.5+.3*std::sin(2*pi*i/100)); }
    cx.push_back(cx[0]); cy.push_back(cy[0]);
    r=process(cx,cy,wave,.04f);
    assert(r.x.size()==101 && r.x[0]==r.x[100] && r.y[0]==r.y[100]);
    for(int i=0;i<100;++i) {
        assert(near(std::hypot(r.x[i]-.5,r.y[i]-.5), .3+.04*std::sin(6*pi*i/100),2e-6));
        // Radial displacement is perpendicular to the circle's tangent.
        assert(near((r.x[i]-cx[i])*(-std::sin(2*pi*i/100))+(r.y[i]-cy[i])*std::cos(2*pi*i/100),0));
    }
    r=process({0,0,.5f,.5f,1},{0,0,0,0,0},{0,1},.1f);
    assert(r.y[0]==r.y[1] && r.y[2]==r.y[3] && near(r.y[2],0));
    r=process({0,1,1,0,0,0},{0,0,1,1,0,0},{0,1},.1f);
    assert(r.x[0]==r.x[4] && r.x[4]==r.x[5] && r.y[0]==r.y[5]);
    assert(process({.2f,.2f},{.3f,.3f},{1},.1f).x == std::vector<float>({.2f,.2f}));
    assert(process({.2f},{.3f},{1},.1f).y == std::vector<float>({.3f}));
    assert(process({0,1},{0},{1},.1f).x.empty());
    assert(process({0,-1},{0,0},{1},.1f).x.empty());
    const float nan=std::numeric_limits<float>::quiet_NaN();
    assert(process({nan},{0},{1},.1f).x.empty());
    assert(process(x,y,{nan},.1f).y==y);
    assert(process(x,y,{1},nan).y==y);
    assert(near(sample({0,1},.75,true),.5)); // interpolation across periodic seam
    r=process({0,1,0},{0,0,0},{1},.1f); // reversing/degenerate path
    for(float v:r.x) assert(std::isfinite(v));
    assert(process({-1,-1},{-1,-1},{1},.1f).x==std::vector<float>({-1,-1}));
    // Amount is interpolated independently of Wave In, by physical distance.
    r=process(x,y,{1},std::vector<float>{0,.2f});
    for(size_t i=0;i<x.size();++i) assert(near(r.y[i],.2*x[i]));
    r=process({0,1,-1,0,.5f,1},{0,0,-1,0,0,0},{1},std::vector<float>{0,.2f});
    assert(near(r.y[0],0) && near(r.y[1],.2) && near(r.y[4],.1) && near(r.y[5],.2));
    auto frame=prepare({0,.25f,1},{0,0,0},{1},{.1f},{0,100});
    assert(near(frame.stiffness[1],25));
    frame=prepare({0,1,1,0,0},{0,0,1,1,0},{1},{0,.2f},{0,100});
    assert(near(frame.targets[1],.1) && near(frame.targets[2],.2));
    assert(near(frame.targets[3],.1) && frame.targets[0]==frame.targets[4]);
    assert(near(frame.stiffness[3],50));

    Simulation sim;
    r=sim.update(x,y,{1},{.2f},{40},true,1.0/60);
    assert(r.y[0]>0 && r.y[0]<.2f);
    const double first=r.y[0];
    r=sim.update(x,y,{1},{.2f},{40},true,1.0/60);
    assert(r.y[0]>first); // continues with unchanged input
    double peak=0;
    for(int i=0;i<600;++i) {
        r=sim.update(x,y,{1},{.2f},{40},true,1.0/60);
        peak=std::max(peak,double(r.y[0]));
    }
    assert(peak>.2 && peak<.23 && near(r.y[0],.2));
    // Fixed-target analytical integration agrees across frame rates.
    Simulation low,high;
    Result a,b;
    for(int i=0;i<30;++i) a=low.update(x,y,{1},{.2f},{12},true,1.0/30);
    for(int i=0;i<120;++i) b=high.update(x,y,{1},{.2f},{12},true,1.0/120);
    assert(near(a.y[0],b.y[0],1e-7));
    sim.reset();
    r=sim.update(x,y,{1},{.2f},{0,100},true,.1);
    assert(r.y.front()==0 && r.y[1]>0 && r.y[1]<r.y[2] && r.y[2]<r.y[3]);
    auto held=r.y;
    r=sim.update(x,y,{0},{.2f},{0},true,.1);
    assert(r.y==held); // zero stiffness freezes displacement, clearing velocity
    r=sim.update(x,y,{1},{.2f},{40},false,0);
    for(float v:r.y) assert(near(v,.2));
    r=sim.update(x,y,{1},{.2f},{40},true,.1);
    for(float v:r.y) assert(near(v,.2)); // no jump on Direct -> Spring
    r=sim.update(x,y,{}, {.2f},{40},true,.1);
    assert(r.y[0]>0 && r.y[0]<.2f); // zero target relaxes instead of snapping
    r=sim.update({0,1},{0,0},{1},{.2f},{40},true,0);
    assert(r.y==std::vector<float>({0,0})); // topology reset
    r=sim.update({0,1},{0},{1},{.2f},{40},true,.1);
    assert(r.x.empty());
    r=sim.update({0,1},{0,0},{1},{.2f},{40},true,0);
    assert(r.y==std::vector<float>({0,0}));
    sim.reset();
    for(int i=0;i<100;++i) {
        r=sim.update({0,1,1,0,0,0},{0,0,1,1,0,0},{0,1},{.1f,.2f},{10,100},true,.02);
        assert(r.x[0]==r.x[4] && r.x[4]==r.x[5] && r.y[0]==r.y[5]);
    }
    Simulation stalled, capped;
    a=stalled.update(x,y,{1},{.2f},{1000},true,100);
    b=capped.update(x,y,{1},{.2f},{1000},true,.1);
    assert(a.y==b.y);
    r=stalled.update(x,y,{1},{.2f},{1000},true,nan);
    assert(r.y==a.y);
    std::cout << "Wave It spring: vector controls, overshoot/settling, frame-rate equivalence, zero stiffness, mode switches, closure, invalid input and topology resets passed.\n";
    std::cout << "Wave It geometry: resampling, normals, 3-cycle ripple, independent shapes, closure, duplicates and invalid inputs passed.\n";
}
