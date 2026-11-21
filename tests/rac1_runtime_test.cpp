// SPDX-License-Identifier: MIT
// Exercise the real adapter callbacks with guest RAM and an SDK host stub.
#include "openrac/mod.hpp"
#include "rac1_vendor.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
void load_rac1(openrac::Mod&);
namespace {
std::vector<std::uint8_t> ram(0x02000000u);
openrac_frame_fn frame_fn;
openrac_pad_fn pad_fn;
void* frame_user;
void* pad_user;
void (*draw_fn)();
int balance=100, plays=0, last_sound=-1, originals=0, prints=0;
bool visible=false;
std::vector<std::string> lines;
const vendor_buy_all::rac1::Disc* disc;
void check(bool ok, const char* why) {
    if (!ok) { std::cerr << "FAIL: " << why << '\n'; std::exit(EXIT_FAILURE); }
}
openrac::Memory memory() { return openrac::Memory(ram.data()); }
void print(int x, int y, std::uint64_t, openrac::gaddr text, int) {
    check(x==6 && (y==86 || y==110), "native prompt fits the lower-left details monitor");
    lines.emplace_back(reinterpret_cast<const char*>(ram.data()+text));
    ++prints;
}
int sound(int id, int mode, openrac::gaddr owner) {
    check(mode==0 && owner==0x6000u, "RAC1 native sound mode and owner");
    ++plays; last_sound=id; return 1;
}
void original() { ++originals; }
void* function(const char* name) {
    if (std::string(name)==disc->print) return reinterpret_cast<void*>(&print);
    if (std::string(name)==disc->sound) return reinterpret_cast<void*>(&sound);
    return nullptr;
}
int hook(openrac_mod*, const char* id, const char* signature, void* replacement, void** next) {
    check(std::string(id)=="rac1.vendor.draw_details" && std::string(signature)=="void()",
          "RAC1 details capture hook installed for native vendor prompt");
    draw_fn=reinterpret_cast<void(*)()>(replacement); *next=reinterpret_cast<void*>(&original); return 0;
}
void pad(bool down) {
    std::uint8_t report[2]={0xFF,static_cast<std::uint8_t>(down?0x7F:0xFF)};
    pad_fn(pad_user,0,report,2);
    check(report[0]==0xFF && report[1]==(down?0x7F:0xFF), "normal vendor input is preserved");
}
void tick() { frame_fn(frame_user); }
}
int main(int argc, char** argv) {
    const std::string version=argc>1?argv[1]:"pal";
    for (const auto& d : vendor_buy_all::rac1::discs) if (version==d.version) disc=&d;
    check(disc!=nullptr,"known disc fixture");
    openrac_api api{};
    api.size=sizeof api; api.api_major=1; api.api_minor=20;
    api.game="rac1"; api.version=disc->version; api.memory=ram.data();
    api.log=[](openrac_mod*,int,const char*){};
    api.setting_bool=[](openrac_mod*,const char*){return 1;};
    api.function=&function; api.hook=&hook;
    api.on_frame=[](openrac_mod*,openrac_frame_fn fn,void* user){frame_fn=fn;frame_user=user;return 0;};
    api.on_pad=[](openrac_mod*,openrac_pad_fn fn,void* user){pad_fn=fn;pad_user=user;return 0;};
    api.loaded_level=[](){return 4;};
    api.relocate_data=[](openrac_gaddr a){return a;};
    api.relocate_level_data=[](int level,openrac_gaddr a){check(level==0,"level-0 relocation");return a;};
    api.bolts=[](int add){balance+=add;return balance;};
    api.touch_hint=[](openrac_mod*,const char*,int on){visible=on!=0;};
    api.stack_push=[](std::uint32_t,openrac_gaddr* saved){*saved=0;return 0x10000u;};
    api.stack_pop=[](openrac_gaddr){};
    auto m=memory();
    m.write<std::int32_t>(disc->mode,5);
    m.write<std::int32_t>(disc->vendor,1);
    m.write<openrac::gaddr>(disc->vendor+0x1C,0x6000u);
    m.write<std::int32_t>(disc->vendor+0x210,1);
    m.write<std::int32_t>(disc->vendor+0xD0,2);
    m.write<std::int32_t>(disc->vendor+0xD4,1);
    m.write<std::uint8_t>(disc->owned+2,1);
    m.write<std::int32_t>(disc->ammo+8,3);
    m.write<std::uint16_t>(disc->table+2*0x18+8,5);
    m.write<std::uint16_t>(disc->table+2*0x18+0xA,10);
    m.write<std::uint16_t>(disc->table+2*0x18+0xE,10);
    openrac::Mod mod(&api,reinterpret_cast<openrac_mod*>(1));
    load_rac1(mod); tick(); draw_fn();
    check(originals==1 && prints==2 && lines[0]=="\x13 Buy all" && lines[1]=="35 bolts",
          "native Square glyph and quote are drawn in the vendor details capture");
    draw_fn();
    check(originals==2 && prints==4 && lines[2]=="\x13 Buy all",
          "original vendor details are preserved; prompt is rendered every draw");
    check(visible && plays==0,"idle quote is silent and Android action available");
    pad(true); tick();
    check(balance==65 && m.read<std::int32_t>(disc->ammo+8)==10 && plays==1 && last_sound==7,
          "one paid refill and native purchase sound");
    tick(); check(plays==1 && balance==65,"held button does not buy repeatedly");
    pad(false); tick(); pad(true); tick();
    check(balance==65 && plays==2 && last_sound==2,"full ammo rejects once without charging");
    pad(false); tick(); balance=1; m.write<std::int32_t>(disc->ammo+8,3);
    pad(true); tick();
    check(balance==1 && plays==3 && last_sound==2 && m.read<std::int32_t>(disc->ammo+8)==3,
          "unaffordable purchase rejects without modifying ammo");
    pad(false); tick(); balance=55; m.write<std::int32_t>(disc->vendor+0x40,1);
    pad(true); tick();
    check(balance==5 && m.read<std::int32_t>(disc->ammo+8)==8 && last_sound==7,
          "partial PDA refill uses alternate price");
    pad(false); tick(); m.write<std::int32_t>(disc->vendor+0x5C,2);
    const int before=plays; pad(true); tick();
    check(plays==before && balance==5 && !visible,"manual dialog blocks transactions and touch action");
    prints=0; draw_fn();
    check(originals==3 && prints==0,"manual dialog retains native details but hides custom prompt");
    m.write<std::int32_t>(disc->vendor+0x5C,0);
    m.write<std::int32_t>(disc->mode,0); pad(false); tick(); pad(true); tick();
    check(plays==before && balance==5,"gameplay outside vendor blocks transactions");
    std::cout << "RAC1 " << version << " runtime purchase, sound, input and native prompt checks passed\n";
}
