// Copyright (C) 2023 - Bruno Sanches. See the COPYRIGHT
// file at the top-level directory of this distribution.
// 
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 
// This Source Code Form is "Incompatible With Secondary Licenses", as
// defined by the Mozilla Public License, v. 2.0.

#include "DirectoryMonitor.h"

#include <assert.h>
#include <array>
#include <mutex>
#include <map>
#include <sstream>
#include <thread>

#include <errno.h>
#include <fcntl.h> 
#include <poll.h>
#include <unistd.h>
#include <sys/inotify.h>

//https://qualapps.blogspot.com/2010/05/understanding-readdirectorychangesw.html

namespace ldmonitor
{	
	struct DirectoryMonitor
	{								
		std::array<uint8_t, 1024 * 4>	m_arBuffer;

		fs::path						m_pthPath;

		Callback_t						m_pfnCallback;

		uint32_t						m_u32Flags = 0;				

		DirectoryMonitor() = default;
		DirectoryMonitor(fs::path path, Callback_t callback, uint32_t flags) :
			m_pthPath{ std::move(path) },
			m_pfnCallback{ callback },
			m_u32Flags{ flags }
		{
			//empty
		}
	};	

	typedef std::map<int, DirectoryMonitor> MapWatchers_t;
	
	static void CheckThreadConflict();
	static void RemoveWatcher(MapWatchers_t::iterator it, std::unique_lock<std::mutex> lock);

	struct State
	{
		std::mutex m_clLock;
		MapWatchers_t m_mapWatchers;

		int g_iNotifyFD = -1;

		int	m_arPipefd[2] = { -1, -1 };

		std::thread	m_thMonitorThread;		

		MapWatchers_t::iterator TryFindDirectory(const fs::path &path)
		{
			for (MapWatchers_t::iterator it = m_mapWatchers.begin(); it != m_mapWatchers.end(); ++it)
			{
				if (it->second.m_pthPath == path)
					return it;
			}

			return m_mapWatchers.end();
		}

		State() = default;
		State(const State &rhs) = delete;
		State(State &&rhs) = delete;

		~State()
		{
			CheckThreadConflict();

			while (!m_mapWatchers.empty())
			{
				std::unique_lock l{m_clLock};

				auto it = m_mapWatchers.begin();

				RemoveWatcher(it, std::move(l));
			}
		}		
	};

	static State g_State;

	static inline uint32_t Flags2Filter(const uint32_t flags) noexcept
	{			
		uint32_t filter = (flags & MONITOR_ACTION_FILE_CREATE) ? (IN_CREATE) : 0;

		filter = filter | ((flags & MONITOR_ACTION_FILE_DELETE) ? IN_DELETE | IN_DELETE_SELF : 0);

		filter = filter | ((flags & MONITOR_ACTION_FILE_MODIFY) ? IN_MODIFY : 0);

		filter = filter | ((flags & MONITOR_ACTION_FILE_RENAME_OLD_NAME) ? IN_MOVED_FROM : 0);
		filter = filter | ((flags & MONITOR_ACTION_FILE_RENAME_NEW_NAME) ? IN_MOVED_TO : 0);

		return filter;
	}	
	
	uint32_t ReadActions2Flags(uint32_t action) 
	{
		switch(action)
		{
			case IN_CREATE:
				return MONITOR_ACTION_FILE_CREATE;

			case IN_DELETE:
			case IN_DELETE_SELF:
				return MONITOR_ACTION_FILE_DELETE;

			case IN_MOVED_FROM:
				return MONITOR_ACTION_FILE_RENAME_OLD_NAME;

			case IN_MOVED_TO:
				return MONITOR_ACTION_FILE_RENAME_NEW_NAME;

			case IN_MODIFY:
				return MONITOR_ACTION_FILE_MODIFY;

			default:
				{
					std::stringstream stream;

					stream << "[DirectoryMonitor::ReadActions2Flags] Unexpected action: " << action;

					throw std::runtime_error(stream.str());
				}				
				return 0;
		}
	}

	static void CloseINotify()
	{		
		assert(g_State.g_iNotifyFD != -1);

		close(g_State.g_iNotifyFD);
		g_State.g_iNotifyFD = -1;
	}

	static void ThreadProc()
	{					
		//See https://man7.org/linux/man-pages/man7/inotify.7.html
		/* Some systems cannot read integer variables if they are not
			  properly aligned. On other systems, incorrect alignment may
			  decrease performance. Hence, the buffer used for reading from
			  the inotify file descriptor should have the same alignment as
			  struct inotify_event. */

		char buf[4096] __attribute__((aligned(__alignof__(struct inotify_event))));
		const struct inotify_event *event;
		ssize_t len;				

		pollfd pollfd[2];

		pollfd[0].events = POLLIN;		
		pollfd[0].fd = g_State.g_iNotifyFD;

		pollfd[1].events = POLLIN;		
		pollfd[1].fd = g_State.m_arPipefd[0];

		for (;;)
		{			
			pollfd[0].revents = 0;
			pollfd[1].revents = 0;

			auto retval = poll(pollfd, 2, -1);			
			if (retval == -1)
			{
				//acording to man it may happen...
				if (errno == EAGAIN)
					continue;

				std::stringstream stream;

				stream << "[FileMonitor::ThreadProc] select failed, ec: " << errno << ' ' << std::system_category().message(errno);

				throw std::runtime_error(stream.str());
			}			

			if (pollfd[1].revents)
			{
				//data on the pipe... game over... finish it
				break;
			}

			assert(pollfd[0].revents);

			//
			//no data on pipe, got something....
			len = read(g_State.g_iNotifyFD, buf, sizeof(buf));
			if (len == -1)
			{
				std::stringstream stream;

				stream << "[FileMonitor::ThreadProc] Read failed, ec: " << errno << ' ' << std::system_category().message(errno);

				throw std::runtime_error(stream.str());				
			}								

			/* Loop over all events in the buffer. */
			for (char *ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len) 
			{
				event = (const struct inotify_event *)ptr;

				std::lock_guard lock{g_State.m_clLock};
				//may it was removed?
				auto it = g_State.m_mapWatchers.find(event->wd);
				if (it == g_State.m_mapWatchers.end())
					continue;

				auto action = ReadActions2Flags(event->mask);

				auto &dirInfo = it->second;
				if (dirInfo.m_u32Flags & action)
				{
					auto now = std::chrono::steady_clock::now();
					dirInfo.m_pfnCallback(dirInfo.m_pthPath, event->name, action, std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()));
				}					
			}
		}	
	}

	void Watch(const fs::path &path, Callback_t callback, uint32_t flags)
	{		
		std::lock_guard lock{g_State.m_clLock};	
		
		if(g_State.TryFindDirectory(path) != g_State.m_mapWatchers.end())
		{
			std::stringstream stream;
			stream << "[WatchFile] Directory already has a watcher: " << path;

			throw std::invalid_argument(stream.str());
		}
		else
		{
			if (g_State.g_iNotifyFD == -1)
			{
				assert(g_State.m_mapWatchers.empty());

				g_State.g_iNotifyFD = inotify_init();
				if (g_State.g_iNotifyFD == -1)
				{
					std::stringstream stream;
					stream << "[WatchFile] Cannot create inotify instance: " << std::system_category().message(errno);

					throw std::runtime_error(stream.str());
				}
			}

			auto pathStr = path.string();

			auto wd = inotify_add_watch(g_State.g_iNotifyFD, pathStr.c_str(), Flags2Filter(flags));
			if (wd == -1)
			{
				std::stringstream stream;
				stream << "[WatchFile] Cannot add watch: " << pathStr << ", error " << std::system_category().message(errno);

				if (g_State.m_mapWatchers.empty())
				{
					CloseINotify();					
				}

				throw std::invalid_argument(stream.str());
			}

			g_State.m_mapWatchers.insert(std::make_pair(wd, DirectoryMonitor{ path, callback, flags }));

			//
			//start work thread?
			if (!g_State.m_thMonitorThread.joinable())
			{
				if (pipe2(g_State.m_arPipefd, O_DIRECT) == -1)
				{
					std::stringstream stream;
					stream << "[WatchFile] Cannot create pipe: " << pathStr << ", error " << std::system_category().message(errno);					

					throw std::runtime_error(stream.str());
				}

				g_State.m_thMonitorThread = std::thread{ ThreadProc };
			}			
		}
	}

	static void CheckThreadConflict()
	{
		if (std::this_thread::get_id() == g_State.m_thMonitorThread.get_id())
		{
			//called from the callback? - not supported
			throw std::logic_error("[[FileMonitor::UnwatchFile] Cannot remove watcher from the thread!");
		}
	}

	static void RemoveWatcher(MapWatchers_t::iterator it, std::unique_lock<std::mutex> lock)
	{						
		inotify_rm_watch(g_State.g_iNotifyFD, it->first);			
		
		g_State.m_mapWatchers.erase(it);

		if (g_State.m_mapWatchers.empty())
		{
			//notify the thread...
			write(g_State.m_arPipefd[1], "c", 1);

			lock.unlock();
						
			g_State.m_thMonitorThread.join();

			CloseINotify();

			for (int i = 0; i < 2; ++i)
			{
				close(g_State.m_arPipefd[i]);
				g_State.m_arPipefd[i] = -1;
			}			
		}
	}

	bool Unwatch(const fs::path &path)
	{
		CheckThreadConflict();

		std::unique_lock lock{g_State.m_clLock};

		auto it = g_State.TryFindDirectory(path);

		if (it == g_State.m_mapWatchers.end())
			return false;

		RemoveWatcher(it, std::move(lock));

		return true;
	}

	namespace detail
	{
		std::optional<bool> IsThreadWaiting(const fs::path &path)
		{
			std::unique_lock lock{g_State.m_clLock};

			return !g_State.m_mapWatchers.empty();			
		}
	}
}
