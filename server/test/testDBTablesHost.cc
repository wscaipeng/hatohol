/*
 * Copyright (C) 2014-2015 Project Hatohol
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

#include <cppcutter.h>
#include <gcutter.h>
#include "DBTablesTest.h"
#include "DBHatohol.h"
#include "DBTablesHost.h"
#include "Hatohol.h"
#include "Helpers.h"
#include "testDBTablesMonitoring.h"
using namespace std;
using namespace mlpl;

namespace testDBTablesHost {

#define DECLARE_DBTABLES_HOST(VAR_NAME) \
	DBHatohol _dbHatohol; \
	DBTablesHost &VAR_NAME = _dbHatohol.getDBTablesHost();

static const char *testHostName = "FOO FOO FOO";

struct AssertGetHostsArg
  : public AssertGetHostResourceArg<ServerHostDef, HostsQueryOption>
{
	AssertGetHostsArg(gconstpointer ddtParam)
	{
		fixtures = testServerHostDef;
		numberOfFixtures = NumTestServerHostDef;
		setDataDrivenTestParam(ddtParam);
	}

	// This should be moved to AssertGetHostResourceArg later
	virtual bool isAuthorized(const ServerHostDef &svHostDef) override
	{
		return ::isAuthorized(userId, svHostDef.hostId);
	}

	virtual LocalHostIdType
	  getHostId(const ServerHostDef &svHostDef) const override
	{
		return svHostDef.hostIdInServer;
	}

	virtual string makeOutputText(const ServerHostDef &svHostDef)
	{
		string expectedOut =
		  StringUtils::sprintf(
		    "%" PRIu32 "|%s|%s\n",
		    svHostDef.serverId, svHostDef.hostIdInServer.c_str(),
		    svHostDef.name.c_str());
		return expectedOut;
	}

};


static void assertUpsertHost(const HostIdType &hostId)
{
	DBHatohol dbHatohol;
	DBTablesHost &dbHost = dbHatohol.getDBTablesHost();
	ServerHostDef serverHostDef;
	serverHostDef.id = AUTO_INCREMENT_VALUE;
	serverHostDef.hostId = hostId;
	serverHostDef.serverId = 10;
	serverHostDef.hostIdInServer= "123456";
	serverHostDef.name = testHostName;
	serverHostDef.status = HOST_STAT_NORMAL;
	HostIdType retHostId = dbHost.upsertHost(serverHostDef);

	string statement = "SELECT * FROM host_list";
	HostIdType expectHostId =
	  (hostId == AUTO_ASSIGNED_ID) ? retHostId : hostId;
	string expect = StringUtils::sprintf(
	  "%" FMT_HOST_ID "|%s",
	  expectHostId, serverHostDef.name.c_str());
	assertDBContent(&dbHost.getDBAgent(), statement, expect);

	statement = "SELECT * FROM server_host_def";
	expect = StringUtils::sprintf(
	  "1|%" FMT_HOST_ID "|%" FMT_SERVER_ID "|%s|%s|%d",
	  expectHostId, serverHostDef.serverId,
	  serverHostDef.hostIdInServer.c_str(),
	  serverHostDef.name.c_str(), serverHostDef.status);
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
}

static void _assertGetHosts(AssertGetHostsArg &arg)
{
	loadTestDBUser();
	loadTestDBAccessList();
	loadTestDBServerHostDef();
	loadTestDBHostgroupMember();

	DECLARE_DBTABLES_HOST(dbHost);
	arg.fixup();
	ServerHostDefVect svHostDefs;
	dbHost.getServerHostDefs(svHostDefs, arg.option);

	ServerHostDefVectConstIterator svHostItr = svHostDefs.begin();
	for (; svHostItr != svHostDefs.end(); ++svHostItr)
		arg.actualRecordList.push_back(*svHostItr);
	arg.assert();
}
#define assertGetHosts(A) cut_trace(_assertGetHosts(A))

static string makeHostgroupsOutput(const Hostgroup &hostgrp, const size_t &id)
{
	string expectedOut = StringUtils::sprintf(
	  "%zd|%" FMT_SERVER_ID "|%s|%s\n",
	  id + 1, hostgrp.serverId,
	  hostgrp.idInServer.c_str(), hostgrp.name.c_str());
	return expectedOut;
}

static string makeMapHostsHostgroupsOutput(
  const HostgroupMember &hostgrpMember, const size_t &id)
{
	string expectedOut = StringUtils::sprintf(
	  "%zd|%" FMT_SERVER_ID "|%s|%s|%" FMT_HOST_ID "\n",
	  id + 1,
	  hostgrpMember.serverId,
	  hostgrpMember.hostIdInServer.c_str(),
	  hostgrpMember.hostgroupIdInServer.c_str(),
	  hostgrpMember.hostId);

	return expectedOut;
}

static string makeHostsOutput(const ServerHostDef &svHostDef, const size_t &id)
{
	string expectedOut = StringUtils::sprintf(
	  "%zd|" DBCONTENT_MAGIC_ANY "|%" FMT_SERVER_ID "|%s|%s|%d\n",
	  id + 1, svHostDef.serverId,
	  svHostDef.hostIdInServer.c_str(), svHostDef.name.c_str(),
	  HOST_STAT_NORMAL);

	return expectedOut;
}

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_tablesVersion(void)
{
	DBHatohol dbHatohol;
	DBTablesHost &dbHost = dbHatohol.getDBTablesHost();
	cppcut_assert_not_null(&dbHost);
	assertDBTablesVersion(dbHatohol.getDBAgent(),
	                      DB_TABLES_ID_HOST, DBTablesHost::TABLES_VERSION);
}

void test_addHost(void)
{
	DECLARE_DBTABLES_HOST(dbHost);
	const string name = "FOO";
	HostIdType hostId = dbHost.addHost(name);
	const string statement = "SELECT * FROM host_list";
	const string expect = StringUtils::sprintf("%" FMT_HOST_ID "|%s",
	                                           hostId, name.c_str());
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
}

void test_addHostWithDuplicatedName(void)
{
	DECLARE_DBTABLES_HOST(dbHost);
	const string name = "DOG";
	HostIdType hostId0 = dbHost.addHost(name);
	HostIdType hostId1 = dbHost.addHost(name);
	const string statement = "SELECT * FROM host_list";
	const string expect = StringUtils::sprintf(
	  "%" FMT_HOST_ID "|%s\n%" FMT_HOST_ID "|%s",
	  hostId0, name.c_str(), hostId1, name.c_str());
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
}

void test_upsertServerHostDef(void)
{
	ServerHostDef svHostDef;
	svHostDef.id = AUTO_INCREMENT_VALUE;
	svHostDef.hostId = 12345;
	svHostDef.serverId = 15;
	svHostDef.hostIdInServer = "8023455";
	svHostDef.name = "test host name";
	svHostDef.status = HOST_STAT_NORMAL;

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id = dbHost.upsertServerHostDef(svHostDef);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|12345|15|8023455|test host name|%d",
	  id, HOST_STAT_NORMAL);
	const string statement = "SELECT * FROM server_host_def";
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id);
}

void test_upsertServerHostDefAutoIncrement(void)
{
	ServerHostDef svHostDef;
	svHostDef.id = AUTO_INCREMENT_VALUE;
	svHostDef.hostId = 12345;
	svHostDef.serverId = 15;
	svHostDef.hostIdInServer = "8023455";
	svHostDef.name = "test host name";
	svHostDef.status = HOST_STAT_NORMAL;

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id0 = dbHost.upsertServerHostDef(svHostDef);
	GenericIdType id1 = dbHost.upsertServerHostDef(svHostDef);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|12345|15|8023455|test host name|%d",
	  id0, HOST_STAT_NORMAL);
	const string statement = "SELECT * FROM server_host_def ORDER BY id";
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_equal(id0, id1);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id1);
}

void test_upsertServerHostDefUpdate(void)
{
	ServerHostDef svHostDef;
	svHostDef.id = AUTO_INCREMENT_VALUE;
	svHostDef.hostId = 12345;
	svHostDef.serverId = 15;
	svHostDef.hostIdInServer = "8023455";
	svHostDef.name = "test host name";
	svHostDef.status = HOST_STAT_NORMAL;

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id0 = dbHost.upsertServerHostDef(svHostDef);
	svHostDef.id = id0;
	svHostDef.name = "dog-dog-dog";
	GenericIdType id1 = dbHost.upsertServerHostDef(svHostDef);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|12345|15|8023455|dog-dog-dog|%d",
	  id1, HOST_STAT_NORMAL);
	const string statement = "SELECT * FROM server_host_def";
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id0);
	cppcut_assert_equal(id0, id1);
}

void test_upsertServerHostDefUpdateByServerAndHost(void)
{
	ServerHostDef svHostDef;
	svHostDef.id = AUTO_INCREMENT_VALUE;
	svHostDef.hostId = 12345;
	svHostDef.serverId = 15;
	svHostDef.hostIdInServer = "8023455";
	svHostDef.name = "test host name";
	svHostDef.status = HOST_STAT_NORMAL;

	DECLARE_DBTABLES_HOST(dbHost);
	svHostDef.name = "dog-dog-dog";
	GenericIdType id = dbHost.upsertServerHostDef(svHostDef);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|12345|15|8023455|dog-dog-dog|%d",
	  id, HOST_STAT_NORMAL);
	const string statement = "SELECT * FROM server_host_def";
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id);
}

void test_upsertHostAccess(void)
{
	HostAccess hostAccess;
	hostAccess.id = AUTO_INCREMENT_VALUE;
	hostAccess.hostId = 12345;
	hostAccess.ipAddrOrFQDN = "192.168.100.5";
	hostAccess.priority = 1000;

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id = dbHost.upsertHostAccess(hostAccess);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|12345|192.168.100.5|1000", id);
	const string statement = "SELECT * FROM host_access";
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id);
}

void test_upsertHostAccessAutoIncrement(void)
{
	HostAccess hostAccess;
	hostAccess.id = AUTO_INCREMENT_VALUE;
	hostAccess.hostId = 12345;
	hostAccess.ipAddrOrFQDN = "192.168.100.5";
	hostAccess.priority = 1000;

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id0 = dbHost.upsertHostAccess(hostAccess);
	GenericIdType id1 = dbHost.upsertHostAccess(hostAccess);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|12345|192.168.100.5|1000\n"
	  "%" FMT_GEN_ID "|12345|192.168.100.5|1000", id0, id1);
	const string statement = "SELECT * FROM host_access ORDER BY id";
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id0);
	cppcut_assert_not_equal(id0, id1);
}

void test_upsertHostAccessUpdate(void)
{
	HostAccess hostAccess;
	hostAccess.id = AUTO_INCREMENT_VALUE;
	hostAccess.hostId = 12345;
	hostAccess.ipAddrOrFQDN = "192.168.100.5";
	hostAccess.priority = 1000;

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id0 = dbHost.upsertHostAccess(hostAccess);
	hostAccess.id = id0;
	GenericIdType id1 = dbHost.upsertHostAccess(hostAccess);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|12345|192.168.100.5|1000", id1);
	const string statement = "SELECT * FROM host_access ORDER BY id";
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id0);
	cppcut_assert_equal(id0, id1);
}

void test_upsertVMInfo(void)
{
	VMInfo vmInfo;
	vmInfo.id = AUTO_INCREMENT_VALUE;
	vmInfo.hostId = 12345;
	vmInfo.hypervisorHostId = 7766554433;

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id = dbHost.upsertVMInfo(vmInfo);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|12345|7766554433", id);
	const string statement = "SELECT * FROM vm_list";
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id);
}

void test_upsertVMInfoAutoIncrement(void)
{
	VMInfo vmInfo;
	vmInfo.id = AUTO_INCREMENT_VALUE;
	vmInfo.hostId = 12345;
	vmInfo.hypervisorHostId = 7766554433;

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id0 = dbHost.upsertVMInfo(vmInfo);
	GenericIdType id1 = dbHost.upsertVMInfo(vmInfo);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|12345|7766554433\n"
	  "%" FMT_GEN_ID "|12345|7766554433", id0, id1);
	const string statement = "SELECT * FROM vm_list ORDER BY id";
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id0);
	cppcut_assert_not_equal(id0, id1);
}

void test_upsertVMInfoUpdate(void)
{
	VMInfo vmInfo;
	vmInfo.id = AUTO_INCREMENT_VALUE;
	vmInfo.hostId = 12345;
	vmInfo.hypervisorHostId = 7766554433;

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id0 = dbHost.upsertVMInfo(vmInfo);
	vmInfo.id = id0;
	GenericIdType id1 = dbHost.upsertVMInfo(vmInfo);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|12345|7766554433", id1);
	const string statement = "SELECT * FROM vm_list ORDER BY id";
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id0);
	cppcut_assert_equal(id0, id1);
}

void test_upsertHostgroupList(void)
{
	Hostgroup hostgroup;
	hostgroup.id = AUTO_INCREMENT_VALUE;
	hostgroup.serverId = 52;
	hostgroup.idInServer = "88664422";
	hostgroup.name = "foo-group";

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id = dbHost.upsertHostgroup(hostgroup);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|52|88664422|foo-group", id);
	const string statement = "SELECT * FROM hostgroup_list";
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id);
}

void test_addHostgroupList(void)
{
	DECLARE_DBTABLES_HOST(dbHost);
	HostgroupVect hostgroups;
	DBAgent &dbAgent = dbHost.getDBAgent();
	string statement = "SELECT * FROM ";
	statement += tableProfileHostgroupList.name;
	string expect;

	for (size_t i = 0; i < NumTestHostgroup; i++) {
		const Hostgroup &hostgrp = testHostgroup[i];
		hostgroups.push_back(hostgrp);
		expect += makeHostgroupsOutput(hostgrp, i);
	}
	dbHost.upsertHostgroups(hostgroups);
	assertDBContent(&dbAgent, statement, expect);
}

void test_upsertHostgroupListUpdate(void)
{
	Hostgroup hostgroup;
	hostgroup.id = AUTO_INCREMENT_VALUE;
	hostgroup.serverId = 52;
	hostgroup.idInServer = "88664422";
	hostgroup.name = "new group";

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id0 = dbHost.upsertHostgroup(hostgroup);
	hostgroup.id = id0;
	GenericIdType id1 = dbHost.upsertHostgroup(hostgroup);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|52|88664422|new group", id1);
	const string statement = "SELECT * FROM hostgroup_list";
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id0);
	cppcut_assert_equal(id0, id1);
}

void test_upsertHostgroupMember(void)
{
	HostgroupMember hostgroupMember;
	hostgroupMember.id = AUTO_INCREMENT_VALUE;
	hostgroupMember.serverId = 52;
	hostgroupMember.hostIdInServer = "88664422";
	hostgroupMember.hostgroupIdInServer = "1133";
	hostgroupMember.hostId = 10;

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id = dbHost.upsertHostgroupMember(hostgroupMember);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|52|88664422|1133|10", id);
	const string statement = StringUtils::sprintf(
	  "SELECT * FROM %s", tableProfileHostgroupMember.name);
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id);
}

void test_addHostgroupMember(void)
{
	DECLARE_DBTABLES_HOST(dbHost);
	HostgroupMemberVect hostgrpMembers;
	DBAgent &dbAgent = dbHost.getDBAgent();
	string statement = "SELECT * FROM ";
	statement += tableProfileHostgroupMember.name;
	statement += " ORDER BY id";
	string expect;

	for (size_t i = 0; i < NumTestHostgroupMember; i++) {
		const HostgroupMember hostgrpMember = testHostgroupMember[i];
		hostgrpMembers.push_back(hostgrpMember);
		expect += makeMapHostsHostgroupsOutput(hostgrpMember, i);
	}
	dbHost.upsertHostgroupMembers(hostgrpMembers);
	assertDBContent(&dbAgent, statement, expect);
}

void test_upsertHostgroupMemberAutoIncrement(void)
{
	HostgroupMember hostgroupMember;
	hostgroupMember.id = AUTO_INCREMENT_VALUE;
	hostgroupMember.serverId = 52;
	hostgroupMember.hostIdInServer = "88664422";
	hostgroupMember.hostgroupIdInServer = "1133";
	hostgroupMember.hostId = 0xfedcba9876543210;

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id0 = dbHost.upsertHostgroupMember(hostgroupMember);
	hostgroupMember.hostgroupIdInServer = "Lion";
	GenericIdType id1 = dbHost.upsertHostgroupMember(hostgroupMember);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|52|88664422|1133|18364758544493064720\n"
	  "%" FMT_GEN_ID "|52|88664422|Lion|18364758544493064720", id0, id1);
	const string statement = StringUtils::sprintf(
	  "SELECT * FROM %s ORDER BY id", tableProfileHostgroupMember.name);
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id0);
	cppcut_assert_not_equal(id0, id1);
}

void test_upsertHostgroupMemberUpdate(void)
{
	HostgroupMember hostgroupMember;
	hostgroupMember.id = AUTO_INCREMENT_VALUE;
	hostgroupMember.serverId = 52;
	hostgroupMember.hostIdInServer = "88664422";
	hostgroupMember.hostgroupIdInServer = "1133";
	hostgroupMember.hostId = 123456789;

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id0 = dbHost.upsertHostgroupMember(hostgroupMember);
	hostgroupMember.id = id0;
	GenericIdType id1 = dbHost.upsertHostgroupMember(hostgroupMember);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|52|88664422|1133|123456789", id1);
	const string statement = StringUtils::sprintf(
	  "SELECT * FROM %s", tableProfileHostgroupMember.name);
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id0);
	cppcut_assert_equal(id0, id1);
}

void test_upsertHostgroupMemberUpdateUsingIndex(void)
{
	HostgroupMember hostgroupMember;
	hostgroupMember.id = AUTO_INCREMENT_VALUE;
	hostgroupMember.serverId = 52;
	hostgroupMember.hostIdInServer = "88664422";
	hostgroupMember.hostgroupIdInServer = "1133";
	hostgroupMember.hostId = 11223344;

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id0 = dbHost.upsertHostgroupMember(hostgroupMember);
	GenericIdType id1 = dbHost.upsertHostgroupMember(hostgroupMember);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|52|88664422|1133|11223344", id1);
	const string statement = StringUtils::sprintf(
	  "SELECT * FROM %s", tableProfileHostgroupMember.name);
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id0);
	cppcut_assert_equal(id0, id1);
}

void test_getHostgroupMembers(void)
{
	loadTestDBServer();

	HostgroupMember hostgroupMember;
	hostgroupMember.id = AUTO_INCREMENT_VALUE;
	hostgroupMember.serverId = testServerInfo[0].id;
	hostgroupMember.hostIdInServer = "88664422";
	hostgroupMember.hostgroupIdInServer = "1133";
	hostgroupMember.hostId = 11223344;

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id0 = dbHost.upsertHostgroupMember(hostgroupMember);
	const string expected0 =
	  makeMapHostsHostgroupsOutput(hostgroupMember, id0);

	hostgroupMember.hostIdInServer = "0817";
	hostgroupMember.hostgroupIdInServer = "1155";
	hostgroupMember.hostId = 1976;
	GenericIdType id1 = dbHost.upsertHostgroupMember(hostgroupMember);
	const string expected1 =
	  makeMapHostsHostgroupsOutput(hostgroupMember, id1);

	// Read the data out and check them
	HostgroupMemberVect actualMembers;
	HostgroupMembersQueryOption option(USER_ID_SYSTEM);
	HatoholError err = dbHost.getHostgroupMembers(actualMembers, option);
	assertHatoholError(HTERR_OK, err);
	cppcut_assert_equal((size_t)2, actualMembers.size());
	cppcut_assert_equal(expected0,
	  makeMapHostsHostgroupsOutput(actualMembers[0], id0));
	cppcut_assert_equal(expected1,
	  makeMapHostsHostgroupsOutput(actualMembers[1], id1));
}

void test_getHypervisor(void)
{
	loadTestDBVMInfo();

	DECLARE_DBTABLES_HOST(dbHost);
	const int targetIdx = 0;
	const HostIdType hostId = testVMInfo[targetIdx].hostId;
	HostIdType hypervisorId = INVALID_HOST_ID;
	HostsQueryOption option(USER_ID_SYSTEM);
	HatoholError err = dbHost.getHypervisor(hypervisorId, hostId, option);
	assertHatoholError(HTERR_OK, err);
	cppcut_assert_equal(testVMInfo[targetIdx].hypervisorHostId,
	                    hypervisorId);
}

void test_getHypervisorWithInvalidUser(void)
{
	DECLARE_DBTABLES_HOST(dbHost);
	HostIdType hypervisorId = INVALID_HOST_ID;
	HostsQueryOption option;
	HatoholError err = dbHost.getHypervisor(hypervisorId, 0, option);
	assertHatoholError(HTERR_INVALID_USER, err);
}

void data_getHypervisorWithUserWhoCanAccessAllHostgroup(void)
{
	gcut_add_datum("Allowed user",
	               "allowed", G_TYPE_BOOLEAN, TRUE, NULL);
	gcut_add_datum("Not allowed user",
	               "allowed", G_TYPE_BOOLEAN, FALSE, NULL);
}

void test_getHypervisorWithUserWhoCanAccessAllHostgroup(gconstpointer data)
{
	loadTestDBUser();
	loadTestDBAccessList();
	loadTestDBServerHostDef();
	loadTestDBVMInfo();
	loadTestDBHostgroupMember();

	const gboolean allowedUser = gcut_data_get_boolean(data, "allowed");
	// TODO: This raw index is too unreadable !!
	const ServerHostDef &targetServerHostDef = testServerHostDef[11];
	DECLARE_DBTABLES_HOST(dbHost);
	HostIdType hypervisorId = INVALID_HOST_ID;

	// Select the suitable test data
	AccessInfo *targetAccessInfo = NULL;
	for (size_t i = 0; i < NumTestAccessInfo; i++) {
		AccessInfo *ai = &testAccessInfo[i];
		if (ai->hostgroupId == ALL_HOST_GROUPS &&
		    ai->serverId == targetServerHostDef.serverId) {
			targetAccessInfo = ai;
			break;
		}
	}
	cppcut_assert_not_null(targetAccessInfo);

	// Find the expected test data
	const VMInfo *expectedVMInfo = NULL;
	for (size_t i = 0; i < NumTestVMInfo; i++) {
		const VMInfo *vminfo = &testVMInfo[i];
		if (vminfo->hypervisorHostId == targetServerHostDef.hostId) {
			expectedVMInfo = vminfo;
			break;
		}
	}
	cppcut_assert_not_null(expectedVMInfo);

	UserIdType userId = targetAccessInfo->userId;
	if (!allowedUser) {
		userId = 3;
		cppcut_assert_not_equal(userId, targetAccessInfo->userId);
	}

	// Try to find the hypervisor
	HostsQueryOption option(userId);
	HatoholError err = dbHost.getHypervisor(hypervisorId,
	                                        expectedVMInfo->hostId, option);
	if (allowedUser) {
		assertHatoholError(HTERR_OK, err);
		cppcut_assert_equal(expectedVMInfo->hypervisorHostId, hypervisorId);
	} else {
		assertHatoholError(HTERR_NOT_FOUND_HYPERVISOR, err);
	}
}

static void prepareForAllUserIds(void)
{
	UserIdSet userIdSet;
	userIdSet.insert(USER_ID_SYSTEM);
	for (size_t i = 0; i < NumTestAccessInfo; ++i)
		userIdSet.insert(testAccessInfo[i].userId);

	UserIdSetConstIterator userIdItr = userIdSet.begin();
	for (; userIdItr != userIdSet.end(); ++userIdItr) {
		string uidStr = StringUtils::sprintf("uid:%d", *userIdItr);
		gcut_add_datum(uidStr.c_str(),
		               "userId", G_TYPE_INT, *userIdItr, NULL);
	}
}

void data_getVirtualMachines(void)
{
	prepareForAllUserIds();
}

void test_getVirtualMachines(gconstpointer data)
{
	typedef map<HostIdType, HostIdSet> HypervisorVMMap;
	typedef HypervisorVMMap::iterator  HypervisorVMMapIterator;

	loadTestDBUser();
	loadTestDBAccessList();
	loadTestDBServerHostDef();
	loadTestDBVMInfo();
	loadTestDBHostgroupMember();
	DECLARE_DBTABLES_HOST(dbHost);

	const UserIdType userId = gcut_data_get_int(data, "userId");

	// Collect the Hypervisors
	HypervisorVMMap hypervisorVMMap;
	size_t hostCount = 0;
	for (size_t i = 0; i < NumTestVMInfo; i++) {
		const VMInfo &vminf = testVMInfo[i];
		if (!isAuthorized(userId, vminf.hostId))
			continue;
		hypervisorVMMap[vminf.hypervisorHostId].insert(vminf.hostId);
		hostCount++;
	}
	if (isVerboseMode())
		cut_notify("<<Number of Hosts>>: %zd", hostCount);

	// Call the test method
	HostsQueryOption option(userId);
	HypervisorVMMapIterator mapItr = hypervisorVMMap.begin();
	HypervisorVMMap actualHypervisorVMMap;
	for (; mapItr != hypervisorVMMap.end(); ++mapItr) {
		HostIdVector virtualMachines;
		const HostIdType &hypervisorHostId = mapItr->first;
		HatoholError err = dbHost.getVirtualMachines(
		                     virtualMachines, hypervisorHostId, option);
		assertHatoholError(HTERR_OK, err);
		for (size_t i = 0; i < virtualMachines.size(); i++) {
			const HostIdType &hid = virtualMachines[i];
			actualHypervisorVMMap[hypervisorHostId].insert(hid);
		}
	}

	// Check it
	mapItr = hypervisorVMMap.begin();
	for (; mapItr != hypervisorVMMap.end(); ++mapItr) {
		const HostIdType &expectHypervisorId = mapItr->first;
		HypervisorVMMapIterator actMapItr =
		  actualHypervisorVMMap.find(expectHypervisorId);
		cppcut_assert_equal(
		  true, actMapItr != actualHypervisorVMMap.end());
		HostIdSet &actualHostIds = actMapItr->second;

		const HostIdSet &expectHostIds = mapItr->second;
		HostIdSetConstIterator expHostIdItr = expectHostIds.begin();
		for (; expHostIdItr != expectHostIds.end(); ++expHostIdItr) {
			HostIdSetIterator actHostItr =
			  actualHostIds.find(*expHostIdItr);
			cppcut_assert_equal(
			  true, actHostItr != actualHostIds.end());
			actualHostIds.erase(actHostItr);
		}
		cppcut_assert_equal(true, actualHostIds.empty());
		actualHypervisorVMMap.erase(actMapItr);
	}
	cppcut_assert_equal(true, actualHypervisorVMMap.empty());
}

void data_upsertHost(void)
{
	gcut_add_datum("UNKNOWN_HOST_ID",
	               "hostId", G_TYPE_UINT64, AUTO_ASSIGNED_ID, NULL);
	gcut_add_datum("Specific Host ID",
	               "hostId", G_TYPE_UINT64, 224466, NULL);
}

void test_upsertHost(gconstpointer data)
{
	const HostIdType hostId = gcut_data_get_boolean(data, "hostId");
	assertUpsertHost(hostId);
}

void test_upsertHostUpdate(gconstpointer data)
{
	assertUpsertHost(AUTO_ASSIGNED_ID);

	DBHatohol dbHatohol;
	DBTablesHost &dbHost = dbHatohol.getDBTablesHost();
	ServerHostDef serverHostDef;
	serverHostDef.id = AUTO_INCREMENT_VALUE;
	serverHostDef.hostId = AUTO_ASSIGNED_ID;
	serverHostDef.serverId = 10;
	serverHostDef.hostIdInServer= "123456";
	serverHostDef.name = "GO GO GO GO";
	serverHostDef.status = HOST_STAT_REMOVED;
	HostIdType hostId = dbHost.upsertHost(serverHostDef);

	string statement = "SELECT * FROM host_list";
	string expect = StringUtils::sprintf(
	  "%" FMT_HOST_ID "|%s", hostId, testHostName);
	assertDBContent(&dbHost.getDBAgent(), statement, expect);

	statement = "SELECT * FROM server_host_def";
	expect = StringUtils::sprintf(
	  "1|%" FMT_HOST_ID "|%" FMT_SERVER_ID "|%s|%s|%d",
	  hostId, serverHostDef.serverId,
	  serverHostDef.hostIdInServer.c_str(),
	  serverHostDef.name.c_str(), serverHostDef.status);
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
}

void test_upsertHosts(void)
{
	ServerHostDefVect svHostDefs;
	for (size_t i = 0; i < NumTestServerHostDef; i++)
		svHostDefs.push_back(testServerHostDef[i]);
	DECLARE_DBTABLES_HOST(dbHost);
	dbHost.upsertHosts(svHostDefs);

	// check

	const char *idColumnName =
	  tableProfileHostList.columnDefs[IDX_HOST_LIST_ID].columnName;
	string statement;
	statement = StringUtils::sprintf("SELECT * FROM %s ORDER BY %s ASC",
	              tableProfileHostList.name, idColumnName);

	string expect;
	for (size_t i = 0; i < NumTestServerHostDef; i++) {
		expect += StringUtils::sprintf(
		  "%" FMT_HOST_ID "|%s",
		  testServerHostDef[i].hostId,
		  testServerHostDef[i].name.c_str());
		if (i <  NumTestServerHostDef - 1)
			expect += "\n";
	}
	assertDBContent(&dbHost.getDBAgent(), statement, expect);

	const ColumnDef *colDefs = tableProfileServerHostDef.columnDefs;
	statement = StringUtils::sprintf("SELECT * FROM %s ORDER BY %s ASC",
	              tableProfileServerHostDef.name,
	              colDefs[IDX_HOST_SERVER_HOST_DEF_ID].columnName);
	expect.clear();
	for (size_t i = 0; i < NumTestServerHostDef; i++) {
		const ServerHostDef &serverHostDef = testServerHostDef[i];
		expect += StringUtils::sprintf(
		  "%zd|%" FMT_HOST_ID "|%" FMT_SERVER_ID "|%s|%s|%d",
		  i + 1,
		  serverHostDef.hostId, serverHostDef.serverId,
		  serverHostDef.hostIdInServer.c_str(),
		  serverHostDef.name.c_str(), serverHostDef.status);
		if (i <  NumTestServerHostDef - 1)
			expect += "\n";
	}
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
}

void test_upsertHostsWithHostIdMap(void)
{
	ServerHostDefVect serverHostDefs;
	ServerHostDef serverHostDef;
	serverHostDef.id = AUTO_INCREMENT_VALUE;
	serverHostDef.hostId = AUTO_ASSIGNED_ID;
	serverHostDef.serverId = 10;
	serverHostDef.hostIdInServer= "123456";
	serverHostDef.name = "GO GO GO GO";
	serverHostDef.status = HOST_STAT_NORMAL;

	serverHostDefs.push_back(serverHostDef);
	serverHostDef.hostIdInServer= "887766";
	serverHostDefs.push_back(serverHostDef);

	HostHostIdMap hostHostIdMap;
	DBHatohol dbHatohol;
	DBTablesHost &dbHost = dbHatohol.getDBTablesHost();
	dbHost.upsertHosts(serverHostDefs, &hostHostIdMap);
	cppcut_assert_equal((size_t)2, hostHostIdMap.size());
	for (size_t i = 0; i < serverHostDefs.size(); i++) {
		const ServerHostDef &svHostDef = serverHostDefs[i];
		HostHostIdMapConstIterator it =
		  hostHostIdMap.find(svHostDef.hostIdInServer);
		cppcut_assert_equal(true, it != hostHostIdMap.end());
		const HostIdType expectedHostId = i + 1;
		cppcut_assert_equal(expectedHostId, it->second);
	}
}

void data_getServerHostDefs(void)
{
	prepareForAllUserIds();
}

void test_getServerHostDefs(gconstpointer data)
{
	const UserIdType userId = gcut_data_get_int(data, "userId");
	loadTestDBUser();
	loadTestDBServer();
	loadTestDBAccessList();
	loadTestDBServerHostDef();
	loadTestDBHostgroupMember();
	DECLARE_DBTABLES_HOST(dbHost);

	ServerHostDefVect svHostDefVect;
	HostsQueryOption option(userId);
	assertHatoholError(HTERR_OK,
	                   dbHost.getServerHostDefs(svHostDefVect, option));

	// make the expected hosts
	set<size_t> expectIds;
	for (size_t i = 0; i < NumTestServerHostDef; i++) {
		if (isDefunctTestServer(testServerHostDef[i].serverId))
			continue;
		if (option.has(OPPRVLG_GET_ALL_SERVER) ||
		    isAuthorized(userId, testServerHostDef[i].hostId))
			expectIds.insert(i + 1);
	}

	// Check it
	set<ServerIdType> actIds;
	for (size_t i = 0; i < svHostDefVect.size(); i++)
		actIds.insert(svHostDefVect[i].id);
	assertEqualSize(expectIds, actIds);

	for (size_t i = 0; i < svHostDefVect.size(); i++) {
		const ServerHostDef &act = svHostDefVect[i];
		set<size_t>::const_iterator it = expectIds.find(act.id);
		if (it == expectIds.end()) {
			string errMsg
			  = StringUtils::sprintf("act.id: %" FMT_GEN_ID "\n",
						 act.id);
			errMsg +=
			  makeElementsComparisonString(expectIds, actIds);
			cut_fail("%s", errMsg.c_str());
		}
		const size_t expectedIdx = *it - 1;
		expectIds.erase(it);
		const ServerHostDef &exp = testServerHostDef[expectedIdx];
		cppcut_assert_equal(exp.hostId, act.hostId);
		cppcut_assert_equal(exp.serverId, act.serverId);
		cppcut_assert_equal(exp.hostIdInServer, act.hostIdInServer);
		cppcut_assert_equal(exp.name, act.name);
	}
	cppcut_assert_equal(true, expectIds.empty());
}

void data_getHostInfoList(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getHostInfoList(gconstpointer data)
{
	AssertGetHostsArg arg(data);
	assertGetHosts(arg);
}

void data_getHostInfoListForOneServer(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getHostInfoListForOneServer(gconstpointer data)
{
	AssertGetHostsArg arg(data);
	arg.targetServerId = 1;
	assertGetHosts(arg);
}

void data_getHostInfoListWithNoAuthorizedServer(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getHostInfoListWithNoAuthorizedServer(gconstpointer data)
{
	AssertGetHostsArg arg(data);
	arg.userId = 4;
	assertGetHosts(arg);
}

void data_getHostInfoListWithOneAuthorizedServer(gconstpointer data)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getHostInfoListWithOneAuthorizedServer(gconstpointer data)
{
	AssertGetHostsArg arg(data);
	arg.userId = 5;
	assertGetHosts(arg);
}

void data_getHostInfoListWithUserWhoCanAccessSomeHostgroups(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getHostInfoListWithUserWhoCanAccessSomeHostgroups(gpointer data)
{
	AssertGetHostsArg arg(data);
	arg.userId = 3;
	assertGetHosts(arg);
}

void test_syncHostsMarkInvalid(void)
{
	loadTestDBServerHostDef();
	DECLARE_DBTABLES_HOST(dbHost);
	const ServerIdType targetServerId = 1;

	// We keep the valid status for the hosts whose id is even.
	ServerHostDefVect svHostDefs;
	map<HostIdType, const ServerHostDef *> hostMap;

	for (size_t i = 0; i < NumTestServerHostDef; i++) {
		const ServerHostDef &svHostDef = testServerHostDef[i];
		if (svHostDef.serverId != targetServerId)
			continue;
		hostMap[svHostDef.hostId] = &svHostDef;
		if (svHostDef.hostId % 2)
			continue;
		svHostDefs.push_back(svHostDef);
	}
	// sanity check if we use the proper data
	cppcut_assert_equal(false, svHostDefs.empty());

	// Prepare for the expected result.
	string expect;
	map<HostIdType, const ServerHostDef *>::const_iterator
	  hostMapItr = hostMap.begin();
	for (; hostMapItr != hostMap.end(); ++hostMapItr) {
		const ServerHostDef &svHostDef = *hostMapItr->second;
		expect += StringUtils::sprintf(
		  "%" FMT_HOST_ID "|%d\n", svHostDef.hostId, svHostDef.status);
	}

	// Call the method to be tested and check the result
	dbHost.syncHosts(svHostDefs, targetServerId);
	DBAgent &dbAgent = dbHost.getDBAgent();
	const ColumnDef *coldef = tableProfileServerHostDef.columnDefs;
	string statement = StringUtils::sprintf(
	  "select %s,%s from %s where %s=%" FMT_SERVER_ID " order by %s asc;",
	  coldef[IDX_HOST_SERVER_HOST_DEF_HOST_ID].columnName,
	  coldef[IDX_HOST_SERVER_HOST_DEF_HOST_STATUS].columnName,
	  tableProfileServerHostDef.name,
	  coldef[IDX_HOST_SERVER_HOST_DEF_SERVER_ID].columnName,
	  targetServerId,
	  coldef[IDX_HOST_SERVER_HOST_DEF_HOST_ID].columnName);
	assertDBContent(&dbAgent, statement, expect);
}

void test_syncHostsAddNewHost(void)
{
	loadTestDBServerHostDef();
	DECLARE_DBTABLES_HOST(dbHost);
	const ServerIdType targetServerId = 1;
	ServerHostDef newHost;
	newHost.id = AUTO_INCREMENT_VALUE;
	newHost.hostId = AUTO_ASSIGNED_ID;
	newHost.serverId = targetServerId;
	newHost.hostIdInServer = "123231";
	newHost.name = "new test host";
	newHost.status = HOST_STAT_NORMAL;

	// Sanity check the ID of the new host is not duplicated.
	for (size_t i = 0; i < NumTestServerHostDef; i++) {
		const ServerHostDef &svHostDef = testServerHostDef[i];
		if (svHostDef.serverId != targetServerId)
			continue;
		if (svHostDef.hostIdInServer == newHost.hostIdInServer)
			cut_fail("We use the worng test data");
	}

	// Prepare for the test data and the expected result.
	ServerHostDefVect svHostDefs;
	string expect;
	size_t i = 0;
	for (; i < NumTestServerHostDef; i++) {
		const ServerHostDef &svHostDef = testServerHostDef[i];
		if (svHostDef.serverId != targetServerId)
			continue;
		svHostDefs.push_back(svHostDef);
		expect += makeHostsOutput(svHostDef, i);
	}
	// sanity check if we use the proper data
	cppcut_assert_equal(false, svHostDefs.empty());

	svHostDefs.push_back(newHost);
	expect += makeHostsOutput(newHost, i);

	// Call the method to be tested and check the result
	dbHost.syncHosts(svHostDefs, targetServerId);
	DBAgent &dbAgent = dbHost.getDBAgent();
	const ColumnDef *coldef = tableProfileServerHostDef.columnDefs;
	string statement = StringUtils::sprintf(
	  "select * from %s where %s=%" FMT_SERVER_ID " order by %s asc;",
	  tableProfileServerHostDef.name,
	  coldef[IDX_HOST_SERVER_HOST_DEF_SERVER_ID].columnName,
	  targetServerId,
	  coldef[IDX_HOST_SERVER_HOST_DEF_ID].columnName);
	assertDBContent(&dbAgent, statement, expect);
}

} // namespace testDBTablesHost
