# Lite Directory Monitor

This is a simple library for monitoring changes to a directory in the local file system.

It was designed to provide a simple way to monitor changes to files inside a directory. 

For usage, just call "Watch" function, provide the path, flags to specify which changes should be monitored and a callback function to be called when such changes are detected.

This is a multithreaded library, so the callback will not be called from the same thread that registered it. If you do not want to deal with multi threading issues, I sugest using a polling library like [lfwatch][1].

It can monitor as many directories as necessary, but it uses one thread per directory, so it was not designed to be fast, just simple to use. If you need to monitor several directories or/and expect lot of changes per second, probably this will have a bad performance.

# Example

To monitor a directory for modifications and detect it, simple register it and provide a callback:

```c++
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
```

## License

All code is licensed under the [MPLv2 License][2].

[1]: https://github.com/Twinklebear/lfwatch
[2]: https://choosealicense.com/licenses/mpl-2.0/
