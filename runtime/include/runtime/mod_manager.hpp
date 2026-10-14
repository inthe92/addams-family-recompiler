#pragma once
#include "runtime/mod_lua.hpp"
#include "runtime/text_resources.hpp"
#include <filesystem>
#include <memory>
#include <vector>
namespace nes::mods {
struct Record {std::string id,name,version,author,description,error;bool enabled{};unsigned api_version=1;uint8_t permissions{};bool data_only{};std::string category="Uncategorized",dependencies="None",conflicts="None";std::vector<Setting> settings;};
class Manager {
public:
    Manager(std::filesystem::path root,std::string python,std::filesystem::path helper,Target target={});
    ~Manager();
    void install(const std::filesystem::path& zip);
    void enable(const std::string& id,bool enabled);
    bool configure(const std::string& id,const std::vector<Setting>&);
    void profile(const std::string& action,const std::string& value);
    void export_profile(const std::string& name,const std::filesystem::path& file);
    void inspect_profile(const std::filesystem::path& file);
    void import_profile(const std::filesystem::path& file);
    const std::string& preview_name()const{return preview_name_;}
    const std::string& preview_summary()const{return preview_summary_;}
    struct MemoryUsage {size_t lua{},images{},planes{},resources{};};
    MemoryUsage memory_usage()const;
    void rename_profile(const std::string& old,const std::string& name);
    const std::vector<Profile>& profiles()const{return profiles_;}
    void uninstall(const std::string& id);
    void update(const Observation&) noexcept;
    void complete(const Observation&) noexcept;
    const nes::ui::Bitmap& overlay()const{return overlay_;}
    uint64_t overlay_revision()const{return overlay_revision_;}
    const std::vector<Record>& records()const{return records_;}
    const std::string& error()const{return error_;}
    unsigned jump_extension()const;
    const nes::ui::Bitmap& visual(unsigned space)const{return visuals_[space<2?space:0];}
    uint64_t visual_revision()const{return visual_revision_;}
    bool needs_memory()const;
    const std::vector<nes::runtime::PrgTextResource>& resources()const{return resources_;}
    uint64_t resource_revision()const{return resource_revision_;}
    void reject_resources(const std::string&) noexcept;
    bool active()const{return !scripts_.empty()||!resources_.empty();}
    const std::filesystem::path& root()const{return root_;}
private:
    struct Running {std::string id;std::unique_ptr<LuaMod> lua;std::size_t flushed{};};
    std::filesystem::path root_,helper_;
    std::string python_,error_,preview_name_,preview_summary_,preview_digest_;bool unchanged_{};
    Target target_;
    std::vector<nes::runtime::PrgTextResource> resources_;uint64_t resource_revision_{};
    Observation observation_,completed_;
    nes::ui::Bitmap overlay_;uint64_t overlay_revision_{};
    void refresh_overlay();
    std::array<nes::ui::Bitmap,2> visuals_;uint64_t visual_revision_{};
    std::vector<ImageAsset> load_assets(const std::string&);
    void tick(const Observation&,unsigned api) noexcept;
    std::vector<Profile> profiles_;
    std::vector<Record> records_;
    std::vector<Running> scripts_;
    std::vector<Record> request(const std::string&,const std::string& value="",const std::string& error="");
    void reconcile(std::vector<Record>);
    void log(const std::string&,const std::string&) noexcept;
    void disable_failed(const std::string&,const std::string&) noexcept;
};
}
