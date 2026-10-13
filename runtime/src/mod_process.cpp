// Host-only argv process transport, derived from the existing importer transport.
// No shell and no mod code is evaluated by this process.
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include <chrono>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#include <codecvt>
#include <locale>
#else
#include <spawn.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
extern char** environ;
#endif
namespace nes::mods { namespace fs=std::filesystem;
int run_mod_process(const std::vector<std::string>& arguments,const fs::path& log) {
#ifdef _WIN32
    // CreateProcess takes a command line, not argv. Quote each argument using
    // the Windows CRT rules, including backslashes before quotes and at the end.
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring command;
    for(const auto& item:arguments) {
        if(!command.empty())command+=L' ';
        command+=L'"';size_t slashes=0;
        for(wchar_t ch:converter.from_bytes(item)) {
            if(ch==L'\\'){++slashes;continue;}
            command.append(ch==L'"'?slashes*2+1:slashes,L'\\');
            command+=ch;slashes=0;
        }
        command.append(slashes*2,L'\\');command+=L'"';
    }
    SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES),nullptr,TRUE};
    HANDLE output=CreateFileW(log.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,
        &security,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(output==INVALID_HANDLE_VALUE)throw std::runtime_error("Unable to create diagnostic log: "+log.u8string());
    const auto failed=[&](DWORD error) {
        const std::string message="Python launch failed (Windows error "+std::to_string(error)+").\n";
        DWORD written=0;WriteFile(output,message.data(),static_cast<DWORD>(message.size()),&written,nullptr);
        CloseHandle(output);return -1;
    };
    HANDLE input=CreateFileW(L"NUL",GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,
        &security,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(input==INVALID_HANDLE_VALUE)return failed(GetLastError());
    STARTUPINFOW startup{};startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESTDHANDLES;
    startup.hStdInput=input;startup.hStdOutput=output;startup.hStdError=output;
    PROCESS_INFORMATION process{};
    // No shell is involved. A null application name permits PATH lookup for
    // developer builds; the packaged executable path is quoted in argv[0].
    const BOOL launched=CreateProcessW(nullptr,command.data(),nullptr,nullptr,TRUE,
        CREATE_NO_WINDOW,nullptr,nullptr,&startup,&process);
    const DWORD error=launched?ERROR_SUCCESS:GetLastError();CloseHandle(input);
    if(!launched)return failed(error);
    DWORD exit_code=1;
    const DWORD waited=WaitForSingleObject(process.hProcess,30000);
    if(waited==WAIT_TIMEOUT){TerminateProcess(process.hProcess,1);WaitForSingleObject(process.hProcess,5000);}
    const BOOL exited=waited==WAIT_OBJECT_0&&GetExitCodeProcess(process.hProcess,&exit_code);
    const DWORD wait_error=exited?ERROR_SUCCESS:GetLastError();
    CloseHandle(process.hThread);CloseHandle(process.hProcess);
    if(!exited)return failed(wait_error);
    CloseHandle(output);return static_cast<int>(exit_code);
#else
    std::vector<const char*> args;
    for(const auto& item:arguments)args.push_back(item.c_str());args.push_back(nullptr);
    // Capture output before Python loads its script, so startup/argument errors
    // are also visible. The child owns the log; Python must not reopen it.
    posix_spawn_file_actions_t actions;
    int error=posix_spawn_file_actions_init(&actions);
    if(!error) {
        error=posix_spawn_file_actions_addopen(&actions,STDOUT_FILENO,log.c_str(),O_WRONLY|O_CREAT|O_TRUNC,0600);
        if(!error)error=posix_spawn_file_actions_adddup2(&actions,STDOUT_FILENO,STDERR_FILENO);
        pid_t child=0;
        if(!error)error=posix_spawnp(&child,args[0],&actions,nullptr,const_cast<char**>(args.data()),environ);
        posix_spawn_file_actions_destroy(&actions);
        if(!error) {
            int status=0;pid_t waited;
            const auto limit=std::chrono::steady_clock::now()+std::chrono::seconds(30);
            do{waited=waitpid(child,&status,WNOHANG);if(waited==0){if(std::chrono::steady_clock::now()>limit){kill(child,SIGKILL);waitpid(child,&status,0);return -1;}std::this_thread::sleep_for(std::chrono::milliseconds(10));}}while(waited==0||(waited<0&&errno==EINTR));
            if(waited>=0)return WIFEXITED(status)?WEXITSTATUS(status):-1;
            error=errno;
        }
    }
    std::ofstream(log)<<"Python launch failed: "<<std::strerror(error)<<'\n';return -1;
#endif
}
}
