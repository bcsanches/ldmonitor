// Copyright (C) 2019 - Bruno Sanches. See the COPYRIGHT
// file at the top-level directory of this distribution.
// 
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 
// This Source Code Form is "Incompatible With Secondary Licenses", as
// defined by the Mozilla Public License, v. 2.0.

#include <gtest/gtest.h>

#include <thread>
#include <fstream>

#include "DirectoryMonitor.h"

using namespace std::chrono_literals;


static void NullFileCallback(const DirectoryMonitor::fs::path &path, std::string fileName, uint32_t flags)
{

}



TEST(DirectoryMonitor, CancelWatcherTest)
{	
#if 1
	auto tmpPath = DirectoryMonitor::fs::temp_directory_path();

	tmpPath.append("testDir");

	DirectoryMonitor::fs::create_directories(tmpPath);
#endif

	auto filePath = tmpPath;
	filePath.append("f1_cancel.txt");	

	DirectoryMonitor::Watch(tmpPath, NullFileCallback, DirectoryMonitor::MONITOR_ACTION_FILE_CREATE);

	DirectoryMonitor::Unwatch(tmpPath);
}

TEST(DirectoryMonitor, CancelWaitingWatcherTest)
{	
#if 1
	auto tmpPath = DirectoryMonitor::fs::temp_directory_path();

	tmpPath.append("testDir");

	DirectoryMonitor::fs::create_directories(tmpPath);
#endif

	auto filePath = tmpPath;
	filePath.append("f1_cancel.txt");

	DirectoryMonitor::Watch(tmpPath, NullFileCallback, DirectoryMonitor::MONITOR_ACTION_FILE_CREATE);

	//make almost sure thread is waiting...
	for (;;)
	{
		auto v = DirectoryMonitor::detail::IsThreadWaiting(tmpPath);
		if (v.has_value() && v.value())
		{
			std::this_thread::sleep_for(5ms);
			break;
		}

		std::this_thread::sleep_for(1ms);
	}

	DirectoryMonitor::Unwatch(tmpPath);
}


//
//
//
//
//

static bool g_fThrowFileCallbackCalled = false;

static void ThrowFileCallback(const DirectoryMonitor::fs::path &path, std::string fileName, uint32_t flags)
{	
	ASSERT_THROW(DirectoryMonitor::Unwatch(path), std::logic_error);

	g_fThrowFileCallbackCalled = true;
}

TEST(DirectoryMonitor, ThrowWatcherTest)
{	
#if 1
	auto tmpPath = DirectoryMonitor::fs::temp_directory_path();

	tmpPath.append("testDir");

	DirectoryMonitor::fs::create_directories(tmpPath);
#endif

	auto filePath = tmpPath;
	filePath.append("f1_cancel.txt");

	DirectoryMonitor::fs::remove(filePath);

	DirectoryMonitor::Watch(tmpPath, ThrowFileCallback, DirectoryMonitor::MONITOR_ACTION_FILE_CREATE);

	ASSERT_FALSE(g_fThrowFileCallbackCalled);

	std::ofstream ofs(filePath);
	ofs << "this is some text in the new file\n";
	ofs.close();

	//wait fot the callback
	while (!g_fThrowFileCallbackCalled)
		std::this_thread::sleep_for(1ms);

	DirectoryMonitor::Unwatch(tmpPath);	
}

//
//
//
//
//

static bool g_fCreateCalled = false;
static bool g_fDeleteCalled = false;
static bool g_fModifyCalled = false;
static bool g_fRenameOldCalled = false;
static bool g_fRenameNewCalled = false;

static void FileCallback(const DirectoryMonitor::fs::path &path, std::string fileName, uint32_t flags)
{
	if(flags & DirectoryMonitor::MONITOR_ACTION_FILE_CREATE)
		g_fCreateCalled = true;

	if(flags & DirectoryMonitor::MONITOR_ACTION_FILE_DELETE)
		g_fDeleteCalled = true;

	if(flags & DirectoryMonitor::MONITOR_ACTION_FILE_MODIFY)
		g_fModifyCalled = true;

	if(flags & DirectoryMonitor::MONITOR_ACTION_FILE_RENAME_OLD_NAME)
		g_fRenameOldCalled = true;

	if(flags & DirectoryMonitor::MONITOR_ACTION_FILE_RENAME_NEW_NAME)
		g_fRenameNewCalled = true;

}

static void ClearFlags()
{
	g_fCreateCalled = false;
	g_fDeleteCalled = false;
	g_fModifyCalled = false;
	g_fRenameOldCalled = false;
	g_fRenameNewCalled = false;
}

static void AssertFlagsClear()
{
	ASSERT_FALSE(g_fCreateCalled);
	ASSERT_FALSE(g_fDeleteCalled);
	ASSERT_FALSE(g_fModifyCalled);
	ASSERT_FALSE(g_fRenameOldCalled);
	ASSERT_FALSE(g_fRenameNewCalled);
}

TEST(DirectoryMonitor, BasicTest)
{		
#if 1
	auto tmpPath = DirectoryMonitor::fs::temp_directory_path();

	tmpPath.append("testDir");	

	DirectoryMonitor::fs::create_directories(tmpPath);
#endif

	auto filePath = tmpPath;
	filePath.append("f1.txt");

	auto newName = tmpPath;
	newName.append("f2.txt");

	DirectoryMonitor::fs::remove(newName);
	DirectoryMonitor::fs::remove(filePath);

	ClearFlags();
	
	DirectoryMonitor::Watch(
		tmpPath, 
		FileCallback, 
		DirectoryMonitor::MONITOR_ACTION_FILE_CREATE | 
		DirectoryMonitor::MONITOR_ACTION_FILE_DELETE | 
		DirectoryMonitor::MONITOR_ACTION_FILE_MODIFY |
		DirectoryMonitor::MONITOR_ACTION_FILE_RENAME_OLD_NAME | 
		DirectoryMonitor::MONITOR_ACTION_FILE_RENAME_NEW_NAME
	);	

	AssertFlagsClear();

	{
		std::ofstream ofs(filePath);
	}
	
	for (;;)
	{			
		if (g_fCreateCalled)
		{
			g_fCreateCalled = false;

			AssertFlagsClear();
			break;
		}

		std::this_thread::sleep_for(1ms);
	}	

	{
		std::ofstream ofs(filePath);
		ofs << "this is some text in the new file\n";
	}

	for (;;)
	{
		if (g_fModifyCalled)
		{
			g_fModifyCalled = false;

			AssertFlagsClear();
			break;
		}

		std::this_thread::sleep_for(1ms);
	}	

	DirectoryMonitor::fs::rename(filePath, newName);

	for (;;)
	{
		if (g_fRenameNewCalled && g_fRenameOldCalled)
		{
			g_fRenameOldCalled = false;
			g_fRenameNewCalled = false;

			AssertFlagsClear();
			break;
		}

		std::this_thread::sleep_for(1ms);
	}	

	DirectoryMonitor::fs::remove(newName);

	for (;;)
	{
		if (g_fDeleteCalled)
		{
			g_fDeleteCalled = false;

			AssertFlagsClear();
			break;
		}
		std::this_thread::sleep_for(1ms);
	}
		
	DirectoryMonitor::Unwatch(tmpPath);
}