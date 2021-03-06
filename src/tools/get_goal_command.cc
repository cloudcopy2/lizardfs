/*
   Copyright 2005-2010 Jakub Kruszona-Zawadzki, Gemius SA, 2013-2014 EditShare,
   2013-2016 Skytechnology sp. z o.o..

   This file was part of MooseFS and is part of LizardFS.

   LizardFS is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, version 3.

   LizardFS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with LizardFS. If not, see <http://www.gnu.org/licenses/>.
 */

#include "common/platform.h"

#include <stdio.h>

#include "tools/tools_commands.h"

int get_goal(const char *fname, uint8_t mode) {
	uint32_t inode;
	int fd = open_master_conn(fname, &inode, NULL, 0, 0);
	if (fd < 0) {
		return -1;
	}
	try {
		uint32_t messageId = 0;
		MessageBuffer request;
		cltoma::fuseGetGoal::serialize(request, messageId, inode, mode);
		MessageBuffer response =
		    ServerConnection::sendAndReceive(fd, request, LIZ_MATOCL_FUSE_GETGOAL);
		PacketVersion version;
		std::vector<FuseGetGoalStats> goalsStats;
		deserializePacketVersionNoHeader(response, version);
		if (version == matocl::fuseGetGoal::kStatusPacketVersion) {
			uint8_t status;
			matocl::fuseGetGoal::deserialize(response, messageId, status);
			throw Exception(std::string(fname) + ": failed", status);
		}
		matocl::fuseGetGoal::deserialize(response, messageId, goalsStats);

		if (mode == GMODE_NORMAL) {
			if (goalsStats.size() != 1) {
				throw Exception(std::string(fname) +
				                ": master query: wrong answer (goalsStats.size != 1)");
			}
			printf("%s: %s\n", fname, goalsStats[0].goalName.c_str());
		} else {
			for (FuseGetGoalStats goalStats : goalsStats) {
				if (goalStats.files > 0) {
					printf(" files with goal        %s :", goalStats.goalName.c_str());
					print_number(" ", "\n", goalStats.files, 1, 0, 1);
				}
			}
			for (FuseGetGoalStats goalStats : goalsStats) {
				if (goalStats.directories > 0) {
					printf(" directories with goal  %s :", goalStats.goalName.c_str());
					print_number(" ", "\n", goalStats.directories, 1, 0, 1);
				}
			}
		}
	} catch (Exception &e) {
		fprintf(stderr, "%s\n", e.what());
		close_master_conn(1);
		return -1;
	}
	close_master_conn(0);
	return 0;
}
