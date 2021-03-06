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

#ifndef ArmFake_h
#define ArmFake_h

#include "ArmBase.h"

class ArmFake : public ArmBase {
public:
	ArmFake(const MonitoringServerInfo &serverInfo);
	virtual ~ArmFake();

protected:
	virtual ArmPollingResult mainThreadOneProc(void) override;
};

#endif // ArmFake_h

