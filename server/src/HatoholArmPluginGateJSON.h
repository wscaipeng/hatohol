/*
 * Copyright (C) 2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef HatoholArmPluginGateJSON_h
#define HatoholArmPluginGateJSON_h

#include "DataStore.h"

class HatoholArmPluginGateJSON : public DataStore {
public:
	HatoholArmPluginGateJSON(const MonitoringServerInfo &serverInfo,
				 const bool &autoStart = true);

	virtual const MonitoringServerInfo
	  &getMonitoringServerInfo(void) const override;
	virtual const ArmStatus &getArmStatus(void) const override;

protected:
	virtual ~HatoholArmPluginGateJSON();

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

typedef UsedCountablePtr<HatoholArmPluginGateJSON> HatoholArmPluginGateJSONPtr;

#endif // HatoholArmPluginGateJSON_h

