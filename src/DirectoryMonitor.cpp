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

std::string ldmonitor::ActionName(const uint32_t action)
{
	std::string name;

	if (action & MONITOR_ACTION_FILE_CREATE)
		name.append("FILE_CREATE ");

	if (action & MONITOR_ACTION_FILE_DELETE)
		name.append("FILE_DELETE ");

	if (action & MONITOR_ACTION_FILE_MODIFY)
		name.append("FILE_MODIFY");

	if (action & MONITOR_ACTION_FILE_RENAME_OLD_NAME)
		name.append("FILE_RENAME_OLD_NAME");

	if (action & MONITOR_ACTION_FILE_RENAME_NEW_NAME)
		name.append("FILE_RENAME_NEW_NAME");

	return name.empty() ? "NULL" : name;
}
