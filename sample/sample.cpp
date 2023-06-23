#include <fstream>
#include <iostream>
#include <thread>

#include <ldmonitor/DirectoryMonitor.h>

static bool g_fCallbackCalled = false;

static void Callback(const ldmonitor::fs::path &path, std::string fileName, uint32_t flags)
{
    using namespace std;

    cout << "Callback called, file " << path << fileName << ' ' << ldmonitor::ActionName(flags) << '\n';

    g_fCallbackCalled = true;
}

void main2()
{
    ldmonitor::Watch(
        "/mypath/", 
        [](const ldmonitor::fs::path &path, std::string fileName, uint32_t flags)
        {
            std::cout << "Callback called, file " << path << fileName << ' ' << ldmonitor::ActionName(flags) << '\n';
        }, 
        ldmonitor::MONITOR_ACTION_FILE_CREATE | ldmonitor::MONITOR_ACTION_FILE_MODIFY
    );

    //
    //Do something... callback will be called sometime later by another thread...

    //remove it when not necessary anymore...
    ldmonitor::Unwatch("/mypath/");
}

int main()
{
    auto tmpPath = ldmonitor::fs::temp_directory_path();

    ldmonitor::Watch(tmpPath, Callback, ldmonitor::MONITOR_ACTION_FILE_CREATE | ldmonitor::MONITOR_ACTION_FILE_MODIFY);

    //
    // create a file to trigger the callback
    auto filePath = tmpPath;
    filePath.append("f1_cancel.txt");

    {
        std::ofstream ofs(filePath);

        ofs << "sample content";
    }

    using namespace std::chrono_literals;

    while (!g_fCallbackCalled)
    {
        std::this_thread::sleep_for(10ms);
    }
    
    //
    //got the event, just cleanup
    ldmonitor::Unwatch(tmpPath);

    return 0;
}
