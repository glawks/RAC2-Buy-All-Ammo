// SPDX-License-Identifier: MIT
// Simulate RAC1 vendor transactions, sound events and SDK 1.31 menu overlays.
#include "openrac/mod.hpp"
#include "rac1_vendor.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
void load_rac1(openrac::Mod&);

namespace {
std::vector<std::uint8_t> ram(0x02000000u);
openrac_frame_fn frame_fn = nullptr;
openrac_pad_fn pad_fn = nullptr;
openrac_overlay_fn overlay_fn = nullptr;
void *frame_user = nullptr, *pad_user = nullptr, *overlay_user = nullptr;
int balance=100, plays=0, last_sound=-1, draws=0, rects=0, texts=0;
bool visible=false;
std::string rendered;
const vendor_buy_all::rac1::Disc* disc=nullptr;
void check(bool ok,const char* reason) {
    if(!ok) {std::cerr<<"FAIL: "<<reason<<'\n';std::exit(EXIT_FAILURE);}
}
openrac::Memory memory() {return openrac::Memory(ram.data());}
int sound(int id,int mode,openrac::gaddr owner) {
    check(mode==0&&owner==0x6000u,"native RAC1 sound mode/owner");
    ++plays;last_sound=id;return 1;
}
void* function(const char* name) {
    return std::string(name)==disc->sound?reinterpret_cast<void*>(&sound):nullptr;
}
void pad(bool down) {
    std::uint8_t report[2]={0xFF,static_cast<std::uint8_t>(down?0x7F:0xFF)};
    pad_fn(pad_user,0,report,2);
    check(report[0]==0xFF&&report[1]==(down?0x7F:0xFF),"unchanged native pad state");
}
void tick(){frame_fn(frame_user);}
void rect(void*,float x,float y,float w,float h,std::uint32_t rgba) {
    ++rects;
    check(x==176.0f&&y==274.0f&&w==160.0f&&h>0,
          "RAC1 panel between vendor controls, above weapon carousel");
    check((rgba&0xFFu)!=0,"panel is visibly translucent");
}
void overlay_text(void*,float x,float y,float height,std::uint32_t rgba,const char* str) {
    ++texts;rendered=str;
    check(x>=176.0f&&x<=336.0f&&y>=274.0f&&y<302.0f,
          "prompt remains within vendor panel");
    check(height>=10.0f&&height<=15.0f&&rgba==0xF4F6FFFFu,
          "one readable overlay text line");
    check(!rendered.empty(),"nonempty menu overlay message");
    if (rendered.find("Ammo bought for ") == std::string::npos)
        check(rendered[0]==OPENRAC_UI_SQUARE[0],
              "interactive prompts use the native Square button token");
    else
        check(rendered[0]!=OPENRAC_UI_SQUARE[0],
              "purchase confirmation has no button icon");
}
float text_width(void*,float height,const char* str) {
    return static_cast<float>(std::strlen(str))*height*0.55f;
}
void draw(unsigned phase=OPENRAC_OVERLAY_MENU) {
    openrac_overlay o{};
    o.size=sizeof o;o.phase=phase;
    o.rect=&rect;o.text=&overlay_text;o.text_width=&text_width;
    const int before=texts;
    overlay_fn(overlay_user,&o);
    if(texts>before)++draws;
}
void expect(const std::string& prompt) {
    check(rendered==prompt,"native token and quote/status current");
    check(rects==2*draws&&texts==draws,
          "one text line over two background rectangles");
}
}
int main(int argc,char** argv) {
    const std::string version=argc>1?argv[1]:"pal";
    for(const auto& d:vendor_buy_all::rac1::discs)if(version==d.version)disc=&d;
    check(disc!=nullptr,"supported disc fixture");
    openrac_api api{};
    api.size=sizeof api;api.api_major=1;api.api_minor=31;
    api.game="rac1";api.version=disc->version;api.memory=ram.data();
    api.log=[](openrac_mod*,int,const char*){};
    api.setting_bool=[](openrac_mod*,const char*){return 1;};
    api.function=&function;
    api.hook=[](openrac_mod*,const char*,const char*,void*,void**) -> int {
        check(false,"no native vendor hook should be taken in RAC1");return -1;
    };
    api.hud_text_set=[](openrac_mod*,const openrac_hud_text*,int) -> int {
        check(false,"legacy HUD should no longer be used");return -1;
    };
    api.on_overlay_phases=[](openrac_mod*,openrac_overlay_fn fn,void* user,std::uint32_t phases)->int {
        check(phases==OPENRAC_OVERLAY_MENU,"only vendor/menu frames requested");
        overlay_fn=fn;overlay_user=user;return 0;
    };
    api.on_frame=[](openrac_mod*,openrac_frame_fn fn,void* user) {
        frame_fn=fn;frame_user=user;return 0;
    };
    api.on_pad=[](openrac_mod*,openrac_pad_fn fn,void* user) {
        pad_fn=fn;pad_user=user;return 0;
    };
    api.loaded_level=[](){return 4;};
    api.relocate_data=[](openrac_gaddr a){return a;};
    api.relocate_level_data=[](int level,openrac_gaddr a) {
        check(level==0,"level-0 relocation");return a;
    };
    api.bolts=[](int add){balance+=add;return balance;};
    api.touch_hint=[](openrac_mod*,const char*,int on){visible=on!=0;};
    auto m=memory();
    m.write<std::int32_t>(disc->mode,5);
    m.write<std::int32_t>(disc->vendor,1);
    m.write<openrac::gaddr>(disc->vendor+0x1Cu,0x6000u);
    m.write<std::int32_t>(disc->vendor+0x210u,1);
    m.write<std::int32_t>(disc->vendor+0xD0u,2);
    m.write<std::int32_t>(disc->vendor+0xD4u,1);
    m.write<std::uint8_t>(disc->owned+2u,1);
    m.write<std::int32_t>(disc->ammo+8u,3);
    m.write<std::uint16_t>(disc->table+2u*0x18u+8u,5);
    m.write<std::uint16_t>(disc->table+2u*0x18u+0xAu,10);
    m.write<std::uint16_t>(disc->table+2u*0x18u+0xEu,10);
    openrac::Mod mod(&api,reinterpret_cast<openrac_mod*>(1));
    load_rac1(mod);tick();
    check(overlay_fn!=nullptr,"SDK menu overlay hook installed");
    draw();check(draws==1,"vendor draws a panel");
    expect(std::string(OPENRAC_UI_SQUARE)+" Buy all - 35 bolts");
    draw(OPENRAC_OVERLAY_PLAYING);
    check(draws==1,"no HUD outside menu phase");
    check(visible&&plays==0,"idle quote silent and touch enabled");
    pad(true);tick();draw();
    check(balance==65&&m.read<std::int32_t>(disc->ammo+8u)==10&&plays==1&&last_sound==7,
          "one purchase at normal price with native purchase sound");
    expect("Ammo bought for 35 bolts");
    tick();check(plays==1&&balance==65,"no purchase repeat while held");
    pad(false);tick();pad(true);tick();draw();
    check(balance==65&&plays==2&&last_sound==2,
          "full ammo rejects without charging");
    pad(false);tick();balance=1;m.write<std::int32_t>(disc->ammo+8u,3);
    pad(true);tick();
    check(balance==1&&plays==3&&last_sound==2&&
          m.read<std::int32_t>(disc->ammo+8u)==3,
          "unaffordable purchase does not modify ammo");
    pad(false);tick();balance=55;m.write<std::int32_t>(disc->vendor+0x40u,1);
    pad(true);tick();
    check(balance==5&&m.read<std::int32_t>(disc->ammo+8u)==8&&last_sound==7,
          "partial PDA purchase uses alternate price");
    pad(false);tick();m.write<std::int32_t>(disc->vendor+0x5Cu,2);
    const int before=plays;pad(true);tick();const int old_draws=draws;draw();
    check(plays==before&&balance==5&&!visible,"purchase dialog blocks Buy All");
    check(draws==old_draws,"prompt hidden in purchase dialog");
    m.write<std::int32_t>(disc->vendor+0x5Cu,0);
    m.write<std::int32_t>(disc->mode,0);pad(false);tick();pad(true);tick();draw();
    check(plays==before&&balance==5&&draws==old_draws,
          "outside vendor Buy All remains hidden");
    std::cout<<"RAC1 "<<version
             <<" purchases, native sounds, and SDK 1.31 menu panel passed\n";
}
