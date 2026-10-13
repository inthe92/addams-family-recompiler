#pragma once
#include "runtime/mod_lua.hpp"
#include <filesystem>
#include <memory>
#include <vector>
namespace nes::mods {
struct Record {std::string id,name,version,author,description,error;bool enabled{};};
class Manager {
public:
    Manager(std::filesystem::path root,std::string python,std::filesystem::path helper);
    ~Manager();
    void install(const std::filesystem::path& zip);
    void enable(const std::string& id,bool enabled);
    void uninstall(const std::string& id);
    void update(const Observation&) noexcept;
    const std::vector<Record>& records()const{return records_;}
    const std::string& error()const{return error_;}
    bool active()const{return !scripts_.empty();}
    const std::filesystem::path& root()const{return root_;}
private:
    struct Running {std::string id;std::unique_ptr<LuaMod> lua;std::size_t flushed{};};
    std::filesystem::path root_,helper_;
    std::string python_,error_;
    Observation observation_;
    std::vector<Record> records_;
    std::vector<Running> scripts_;
    std::vector<Record> request(const std::string&,const std::string& value="",const std::string& error="");
    void reconcile(std::vector<Record>);
    void log(const std::string&,const std::string&) noexcept;
    void disable_failed(const std::string&,const std::string&) noexcept;
};
}
