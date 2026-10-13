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
    const auto output=root_/"operation.log";
    if(fs::is_symlink(output))throw std::runtime_error("Linked mod diagnostic file");
    std::vector<std::string> args={python_,"-I","-B",helper_.u8string(),"--root",root_.u8string(),action};
    if(!value.empty())args.push_back(value);if(!error.empty()){args.push_back("--error");args.push_back(error);}
    const auto code=run_mod_process(args,output);std::ifstream input(output);std::string line;std::getline(input,line);if(!line.empty()&&line.back()=='\r')line.pop_back();
    if(code || line!="OK")throw std::runtime_error(line.empty()?"Mod package tool failed; check operation.log":line.substr(0,300));
    std::vector<Record> rows;
    while(std::getline(input,line)){
        if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty())continue;
        std::istringstream fields(line);std::vector<std::string> parts;std::string field;
        while(std::getline(fields,field,'\t'))parts.push_back(decode(field));if(line.back()=='\t')parts.emplace_back();
        if(parts.size()!=7 || rows.size()==32)throw std::runtime_error("Invalid package listing");
        rows.push_back({parts[0],parts[1],parts[2],parts[3],parts[4],parts[6],parts[5]=="1"});
    }return rows;
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
    try{request("disable",id,error);}catch(const std::exception& e){error_+="; persistence failed: "+std::string(e.what());}
}
Manager::Manager(fs::path root,std::string python,fs::path helper):root_(std::move(root)),helper_(std::move(helper)),python_(std::move(python)) {
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
    for(auto i=scripts_.begin();i!=scripts_.end();){
        const bool keep=std::any_of(records_.begin(),records_.end(),[&](const auto&r){return r.id==i->id&&r.enabled;});
        if(keep){++i;continue;}
        try{i->lua->unload();for(const auto& line:i->lua->take_logs())log(i->id,line);}catch(const std::exception& e){disable_failed(i->id,e.what());}
        i=scripts_.erase(i);
    }
    for(auto& r:records_)if(r.enabled && std::none_of(scripts_.begin(),scripts_.end(),[&](const auto&s){return s.id==r.id;})){
        try {
            auto path=root_/"packages"/r.id/"main.lua";if(fs::is_symlink(path)||fs::file_size(path)>128*1024)throw std::runtime_error("Invalid mod entry");
            std::ifstream in(path,std::ios::binary);std::string source((std::istreambuf_iterator<char>(in)),{});
            auto lua=std::make_unique<LuaMod>(source);lua->load(observation_);for(const auto& line:lua->take_logs())log(r.id,line);scripts_.push_back({r.id,std::move(lua),0});
        }catch(const std::exception& e){disable_failed(r.id,e.what());}
    }
    std::sort(scripts_.begin(),scripts_.end(),[](const auto&a,const auto&b){return a.id<b.id;});
}
void Manager::install(const fs::path& zip){error_.clear();reconcile(request("install",fs::absolute(zip).u8string()));}
void Manager::enable(const std::string& id,bool enabled){error_.clear();reconcile(request(enabled?"enable":"disable",id));}
void Manager::uninstall(const std::string& id){error_.clear();reconcile(request("uninstall",id));}
void Manager::update(const Observation& o) noexcept {
    observation_=o;
    for(auto i=scripts_.begin();i!=scripts_.end();){
        try {i->lua->update(o);
            // Only new messages; Lua retains a bounded ring. Logging is host-owned.
            for(const auto& line:i->lua->take_logs())log(i->id,line);++i;
        }catch(const std::exception& e){disable_failed(i->id,e.what());i=scripts_.erase(i);}
    }
}
}
