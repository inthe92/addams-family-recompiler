#include "runtime/mod_manager.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
namespace nes::mods {
namespace fs=std::filesystem;
int run_mod_process(const std::vector<std::string>&,const fs::path&);
static std::string decode(const std::string& text) {
    if(text.size()%2 || text.size()>4096)throw std::runtime_error("Invalid mod package response");
    std::string result;for(size_t i=0;i<text.size();i+=2){const auto digit=[](char c)->unsigned{if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;throw std::runtime_error("Invalid package encoding");};result+=char(digit(text[i])*16+digit(text[i+1]));}return result;
}
std::vector<Record> Manager::request(const std::string& action,const std::string& value,const std::string& error) {
    unchanged_=false;
    const auto output=root_/"operation.log";
    if(fs::is_symlink(output))throw std::runtime_error("Linked mod diagnostic file");
    std::vector<std::string> args={python_,"-I","-B",helper_.u8string(),"--root",root_.u8string(),action};
    if(!target_.engine_version.empty())args.insert(args.end()-1,{"--engine-version",target_.engine_version});
    if(!target_.game_id.empty())args.insert(args.end()-1,{"--game-id",target_.game_id});
    if(!target_.rom_sha256.empty())args.insert(args.end()-1,{"--rom-sha256",target_.rom_sha256});
    if(!target_.resource_catalog.empty())args.insert(args.end()-1,{"--profiles",target_.resource_catalog});
    if(!value.empty())args.push_back(value);if(!error.empty()){args.push_back("--error");args.push_back(error);}
    const auto code=run_mod_process(args,output);std::ifstream input(output);std::string line;std::getline(input,line);if(!line.empty()&&line.back()=='\r')line.pop_back();
    if(code || line!="OK")throw std::runtime_error(line.empty()?"Mod package tool failed; check operation.log":line.substr(0,300));
    std::vector<Profile> profiles;
    std::vector<Record> rows;std::vector<nes::runtime::PrgTextResource> resources;
    while(std::getline(input,line)){
        if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty())continue;
        if(line=="UNCHANGED"){unchanged_=true;continue;}
        if(line.rfind("PREVIEW\t",0)==0){std::istringstream f(line.substr(8));std::array<std::string,3> v;for(auto&x:v){if(!std::getline(f,x,'\t'))throw std::runtime_error("Invalid profile preview");x=decode(x);}std::string extra;if(std::getline(f,extra,'\t')||v[0].size()>24||v[2].size()!=64)throw std::runtime_error("Invalid profile preview bounds");preview_name_=v[0];preview_summary_=v[1];preview_digest_=v[2];continue;}
        if(line.rfind("PROFILE\t",0)==0){
            std::istringstream f(line.substr(8));std::array<std::string,3> v;
            for(auto&x:v){if(!std::getline(f,x,'\t'))throw std::runtime_error("Invalid profile response");x=decode(x);}
            std::string extra;if(std::getline(f,extra,'\t')||profiles.size()==17||v[0].empty()||v[0].size()>24||(v[1]!="0"&&v[1]!="1"))throw std::runtime_error("Invalid profile listing");
            profiles.push_back({v[0],v[2],v[1]=="1"});continue;
        }
        std::istringstream fields(line);std::vector<std::string> parts;std::string field;
        while(std::getline(fields,field,'\t'))parts.push_back(action=="resources"&&parts.empty()?field:decode(field));if(line.back()=='\t')parts.emplace_back();
        if(action=="resources"){
            if(!((parts.size()==7&&parts[0]=="RESOURCE")||(parts.size()==11&&parts[0]=="COMPOSE"))||resources.size()==16)throw std::runtime_error("Invalid text resource response");
            const auto number=[](const std::string& v,uint64_t max){if(v.empty()||v.size()>10||v.find_first_not_of("0123456789")!=std::string::npos)throw std::runtime_error("Invalid resource number");auto n=std::stoull(v);if(n>max)throw std::runtime_error("Resource number exceeds bound");return n;};
            auto data=decode(parts[6]);if(data.empty()||data.size()>64)throw std::runtime_error("Invalid encoded text length");
            resources.push_back({parts[1],parts[2],unsigned(number(parts[3],15)),unsigned(number(parts[4],16383)),uint32_t(number(parts[5],0xffffffffu)),std::vector<uint8_t>(data.begin(),data.end())});
            if(parts[0]=="COMPOSE"){
                nes::runtime::TextComposition c;c.chr_bank=unsigned(number(parts[7],31));c.nametable=unsigned(number(parts[8],0x2fff));c.mirroring=unsigned(number(parts[9],3));
                auto glyphs=decode(parts[10]);if(glyphs.size()%17||glyphs.size()>8*17)throw std::runtime_error("Invalid own glyph payload");
                for(unsigned i=0;i<glyphs.size();i+=17){nes::runtime::VisualGlyph g;g.cell=uint8_t(glyphs[i]);for(unsigned j=0;j<16;++j)g.pixels[j]=uint8_t(glyphs[i+1+j]);c.overrides.push_back(g);}
                resources.back().composition=std::move(c);
            }
            continue;
        }
        if((parts.size()!=7&&parts.size()!=9&&parts.size()!=10&&parts.size()!=14) || rows.size()==32)throw std::runtime_error("Invalid package listing");
        unsigned api=1;uint8_t permissions=0;
        if(parts.size()>=9){if((parts[7]!="1"&&parts[7]!="2")||(parts[8].empty()||parts[8].size()>3||parts[8].find_first_not_of("0123456789")!=std::string::npos||std::stoul(parts[8])>255))throw std::runtime_error("Invalid API negotiation");api=unsigned(parts[7][0]-'0');permissions=uint8_t(std::stoul(parts[8]));if(api==1&&permissions)throw std::runtime_error("API v1 cannot receive permissions");}
        bool data_only=false;if(parts.size()>=10){if(parts[9]!="0"&&parts[9]!="1")throw std::runtime_error("Invalid data-only marker");data_only=parts[9]=="1";if(data_only&&(api!=2||permissions!=16))throw std::runtime_error("Invalid data-only permissions");}
        rows.push_back({parts[0],parts[1],parts[2],parts[3],parts[4],parts[6],parts[5]=="1",api,permissions,data_only});
        if(parts.size()==14){
            auto&r=rows.back();r.category=parts[10];r.dependencies=parts[11];r.conflicts=parts[12];
            std::istringstream settings(parts[13]);std::string row;
            while(std::getline(settings,row)){
                std::istringstream f(row);std::array<std::string,9> v;
                for(auto&x:v)if(!std::getline(f,x,'|'))throw std::runtime_error("Invalid settings response");
                std::string extra;if(std::getline(f,extra,'|')||r.settings.size()==8)throw std::runtime_error("Settings quota exceeded");
                Setting d;d.key=v[0];d.label=v[1];d.type=v[2];d.minimum=std::stod(v[3]);d.maximum=std::stod(v[4]);d.step=std::stod(v[5]);d.value=v[7];d.default_value=v[8];
                std::istringstream choices(v[6]);while(std::getline(choices,extra,','))d.choices.push_back(extra);r.settings.push_back(std::move(d));
            }
        }
    }if(action=="resources"){resources_=std::move(resources);++resource_revision_;}if(action!="resources")profiles_=std::move(profiles);return rows;
}
void Manager::log(const std::string& id,const std::string& message) noexcept {
    try {const auto file=root_/"mods.log";if(fs::is_symlink(file))return;
        if(fs::exists(file)&&fs::file_size(file)>1024*1024){std::ofstream clear(file);}
        std::ofstream out(file,std::ios::app);out<<id<<": "<<message.substr(0,300)<<'\n';
    }catch(...){}
}
void Manager::disable_failed(const std::string& id,const std::string& error) noexcept {
    log(id,error);error_="Mod disabled: "+id+": "+error.substr(0,200);
    for(auto& r:records_)if(r.id==id){r.enabled=false;r.error=error.substr(0,300);}
    try{auto rows=request("disable",id,error);for(auto&r:records_)for(const auto&row:rows)if(r.id==row.id){r.enabled=row.enabled;r.error=row.error;}}catch(const std::exception& e){error_+="; persistence failed: "+std::string(e.what());}
}
Manager::Manager(fs::path root,std::string python,fs::path helper,Target target):root_(std::move(root)),helper_(std::move(helper)),python_(std::move(python)),target_(std::move(target)) {
    root_=fs::absolute(root_);
    for(auto p=root_;!p.empty();){if(fs::is_symlink(p))throw std::runtime_error("Linked mod storage");auto next=p.parent_path();if(next==p)break;p=next;}
    fs::create_directories(root_);
    auto rows=request("list");const auto marker=root_/"session-active";
    if(fs::is_symlink(marker))throw std::runtime_error("Linked mod session marker");
    if(fs::exists(marker)){
        for(const auto& r:rows)if(r.enabled)request("disable",r.id,"Previous session did not close cleanly; re-enable manually");
        rows=request("list");error_="Mods disabled after an unclean session. Review before enabling.";
    }
    {std::ofstream out(marker);out<<"Private mod session; no game data\n";out.flush();
     if(!out)throw std::runtime_error("Cannot record mod session; scripts were not loaded");}
    reconcile(std::move(rows));
}
Manager::~Manager(){
    for(auto& s:scripts_)try{s.lua->unload();for(const auto& line:s.lua->take_logs())log(s.id,line);}catch(const std::exception& e){disable_failed(s.id,e.what());}
    std::error_code e;fs::remove(root_/"session-active",e);
}
void Manager::reconcile(std::vector<Record> rows) {
    records_=std::move(rows);
    if(std::count_if(records_.begin(),records_.end(),[](const auto&r){return r.enabled&&(r.permissions&gameplay_jump);})>1)
        for(auto&r:records_)if(r.enabled&&(r.permissions&gameplay_jump))disable_failed(r.id,"Conflicting jump_extension owners; disable competing gameplay mods");
    if(std::count_if(records_.begin(),records_.end(),[](const auto&r){return r.enabled&&(r.permissions&(ui_draw|graphics_layers));})>8)
        for(auto&r:records_)if(r.enabled&&(r.permissions&(ui_draw|graphics_layers)))disable_failed(r.id,"Visual mod quota exceeded; enable at most eight");
    for(auto i=scripts_.begin();i!=scripts_.end();){
        const bool keep=std::any_of(records_.begin(),records_.end(),[&](const auto&r){return r.id==i->id&&r.enabled;});
        if(keep){++i;continue;}
        try{i->lua->unload();for(const auto& line:i->lua->take_logs())log(i->id,line);}catch(const std::exception& e){disable_failed(i->id,e.what());}
        i=scripts_.erase(i);
    }
    for(auto& r:records_)if(r.enabled && std::none_of(scripts_.begin(),scripts_.end(),[&](const auto&s){return s.id==r.id;})){
        try {
            if(r.data_only){(void)request("check",r.id);continue;}
            auto path=root_/"packages"/r.id/"main.lua";if(fs::is_symlink(path)||fs::file_size(path)>128*1024)throw std::runtime_error("Invalid mod entry");
            std::ifstream in(path,std::ios::binary);std::string source((std::istreambuf_iterator<char>(in)),{});
            if(r.api_version==2)(void)request("check",r.id); // Recheck requirements at startup without executing Lua.
            auto lua=std::make_unique<LuaMod>(source,2*1024*1024,r.api_version,r.permissions,(r.permissions&graphics_layers)?load_assets(r.id):std::vector<ImageAsset>{},r.settings);lua->load(r.api_version==2?completed_:observation_);for(const auto& line:lua->take_logs())log(r.id,line);scripts_.push_back({r.id,std::move(lua),0});
        }catch(const std::exception& e){disable_failed(r.id,e.what());}
    }
    std::sort(scripts_.begin(),scripts_.end(),[](const auto&a,const auto&b){return a.id<b.id;});
    try{(void)request("resources");}catch(const std::exception&e){resources_.clear();++resource_revision_;error_="Translations unavailable: "+std::string(e.what());}
    refresh_overlay();
}
void Manager::install(const fs::path& zip){error_.clear();reconcile(request("install",fs::absolute(zip).u8string()));}
void Manager::enable(const std::string& id,bool enabled){error_.clear();reconcile(request(enabled?"enable":"disable",id));}
void Manager::uninstall(const std::string& id){error_.clear();reconcile(request("uninstall",id));}
static std::string json_string(const std::string&s){std::string result="\"";for(char c:s){if(c=='\\'||c=='\"')result+='\\';if(static_cast<unsigned char>(c)<32)throw std::runtime_error("Invalid configuration text");result+=c;}return result+'\"';}
bool Manager::configure(const std::string&id,const std::vector<Setting>&settings){
    std::string value="{\"id\":"+json_string(id)+",\"values\":{";bool first=true;
    for(const auto&s:settings){if(!first)value+=',';first=false;value+=json_string(s.key)+":"+(s.type=="enum"?json_string(s.value):s.value);}value+="}}";
    error_.clear();auto rows=request("configure",value);if(unchanged_)return false;bool unload_failed=false;
    // Configuration is an explicit lifecycle restart, never a live Lua mutation.
    for(auto i=scripts_.begin();i!=scripts_.end();)if(i->id==id){try{i->lua->unload();for(const auto&line:i->lua->take_logs())log(i->id,line);}catch(const std::exception&e){unload_failed=true;disable_failed(i->id,e.what());}i=scripts_.erase(i);}else ++i;
    if(unload_failed)rows=request("list");reconcile(std::move(rows));return true;
}
void Manager::profile(const std::string&action,const std::string&value){
    error_.clear();auto rows=request("profile-"+action,value);bool unload_failed=false;
    if(action=="apply"&&!unchanged_){for(auto&s:scripts_)try{s.lua->unload();for(const auto&line:s.lua->take_logs())log(s.id,line);}catch(const std::exception&e){unload_failed=true;disable_failed(s.id,e.what());}scripts_.clear();}
    if(unload_failed)rows=request("list");reconcile(std::move(rows));
}
void Manager::export_profile(const std::string&name,const fs::path&file){error_.clear();(void)request("profile-export","{\"name\":"+json_string(name)+",\"file\":"+json_string(fs::absolute(file).u8string())+"}");}
void Manager::inspect_profile(const fs::path&file){preview_name_.clear();preview_digest_.clear();error_.clear();(void)request("profile-inspect",fs::absolute(file).u8string());if(preview_digest_.empty())throw std::runtime_error("Missing profile review result");}
void Manager::import_profile(const fs::path&file){if(preview_digest_.empty())throw std::runtime_error("Review the profile before importing");error_.clear();reconcile(request("profile-import","{\"file\":"+json_string(fs::absolute(file).u8string())+",\"sha256\":"+json_string(preview_digest_)+"}"));preview_digest_.clear();}
Manager::MemoryUsage Manager::memory_usage()const{MemoryUsage n;for(const auto&s:scripts_){n.lua+=s.lua->memory_used();for(const auto&a:s.lua->assets())n.images+=a.bitmap.rgba.size();}n.planes=overlay_.rgba.size();for(const auto&b:visuals_)n.planes+=b.rgba.size();for(const auto&r:resources_)n.resources+=r.bytes.size();return n;}
void Manager::rename_profile(const std::string&old,const std::string&name){profile("rename","{\"old\":"+json_string(old)+",\"name\":"+json_string(name)+"}");}
void Manager::reject_resources(const std::string& message) noexcept{
    for(auto& r:records_)if(r.enabled&&r.data_only)disable_failed(r.id,message);resources_.clear();++resource_revision_;
}
unsigned Manager::jump_extension()const{for(const auto&s:scripts_)if(s.lua->jump_extension())return s.lua->jump_extension();return 0;}
bool Manager::needs_memory()const{for(const auto& s:scripts_)if(s.lua->needs_memory())return true;return false;}
std::vector<ImageAsset> Manager::load_assets(const std::string&id){
    const auto output=root_/"operation.log";if(fs::is_symlink(output))throw std::runtime_error("Linked asset diagnostic");
    const auto code=run_mod_process({python_,"-I","-B",helper_.u8string(),"--root",root_.u8string(),"assets",id},output);
    if(code||fs::file_size(output)>600000)throw std::runtime_error("Image validation failed; check operation.log");
    std::ifstream in(output);std::string line;std::getline(in,line);if(!line.empty()&&line.back()=='\r')line.pop_back();if(line!="OK")throw std::runtime_error("Invalid image response");
    std::vector<ImageAsset> result;
    while(std::getline(in,line)){
        if(!line.empty()&&line.back()=='\r')line.pop_back();std::istringstream row(line);std::array<std::string,5> fields;
        for(auto&field:fields)if(!std::getline(row,field,'\t'))throw std::runtime_error("Invalid image row");std::string extra;if(std::getline(row,extra,'\t')||fields[0]!="IMAGE"||result.size()==2)throw std::runtime_error("Invalid image count");
        ImageAsset a;a.id=decode(fields[1]);const auto number=[](const std::string&v){if(v.empty()||v.size()>3||v.find_first_not_of("0123456789")!=std::string::npos)throw std::runtime_error("Invalid image size");unsigned n=unsigned(std::stoul(v));if(!n||n>128)throw std::runtime_error("Image size exceeds quota");return n;};
        a.bitmap.width=number(decode(fields[2]));a.bitmap.height=number(decode(fields[3]));const size_t bytes=a.bitmap.width*a.bitmap.height*4;
        if(fields[4].size()!=bytes*4)throw std::runtime_error("Invalid image payload size");
        const auto digit=[](char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;throw std::runtime_error("Invalid image hex");};
        a.bitmap.rgba.reserve(bytes);for(size_t i=0;i<fields[4].size();i+=4){char h=char(digit(fields[4][i])*16+digit(fields[4][i+1])),l=char(digit(fields[4][i+2])*16+digit(fields[4][i+3]));a.bitmap.rgba.push_back(uint8_t(digit(h)*16+digit(l)));}
        result.push_back(std::move(a));
    }
    return result;
}
void Manager::refresh_overlay(){
    std::vector<nes::ui::TextLine> lines;
    for(const auto& s:scripts_)for(const auto& line:s.lua->overlay())lines.push_back(line);
    overlay_=nes::ui::text_overlay_bitmap(hud_width,hud_height,lines);++overlay_revision_;
    struct Item {const VisualCommand* c;const LuaMod* lua;unsigned order;};std::array<Item,8*visual_quota> items{};unsigned count=0;
    for(const auto&s:scripts_)for(const auto&c:s.lua->visual()){if(count==items.size())throw std::runtime_error("Global visual command quota exceeded");items[count]={&c,s.lua.get(),count};++count;}
    std::sort(items.begin(),items.begin()+count,[](auto&a,auto&b){return a.c->z==b.c->z?a.order<b.order:a.c->z<b.c->z;});
    for(unsigned space=0;space<2;++space){auto&b=visuals_[space];const bool used=std::any_of(items.begin(),items.begin()+count,[&](const auto&i){return i.c->space==space;});
        if(!used){b={};continue;}b.width=visual_width;b.height=visual_height;b.rgba.resize(visual_width*visual_height*4);std::fill(b.rgba.begin(),b.rgba.end(),0);
        for(unsigned j=0;j<count;++j){auto&i=items[j];if(i.c->space==space)paint_visual(b,*i.c,i.lua->assets());}
    }++visual_revision_;
}
void Manager::update(const Observation& o) noexcept {observation_=o;tick(o,1);}
void Manager::complete(const Observation& o) noexcept {completed_=o;tick(o,2);}
void Manager::tick(const Observation& o,unsigned api) noexcept {
    for(auto i=scripts_.begin();i!=scripts_.end();){
        if(std::none_of(records_.begin(),records_.end(),[&](const auto&r){return r.id==i->id&&r.enabled;})){try{i->lua->unload();}catch(...){}i=scripts_.erase(i);continue;}
        if(i->lua->api_version()!=api){++i;continue;}
        try {i->lua->update(o);
            // Only new messages; Lua retains a bounded ring. Logging is host-owned.
            for(const auto& line:i->lua->take_logs())log(i->id,line);++i;
        }catch(const std::exception& e){disable_failed(i->id,e.what());i=scripts_.erase(i);}
    }
    for(auto i=scripts_.begin();i!=scripts_.end();)if(std::none_of(records_.begin(),records_.end(),[&](const auto&r){return r.id==i->id&&r.enabled;})){try{i->lua->unload();}catch(...){}i=scripts_.erase(i);}else ++i;
    if(api==2)try{refresh_overlay();}catch(...){overlay_={};visuals_={};++visual_revision_;error_="Unable to publish mod HUD";}
}
}
