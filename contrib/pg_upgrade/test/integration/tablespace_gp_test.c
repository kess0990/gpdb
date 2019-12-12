/*
 * External dependencies
 */
#include "cmockery_gp.h"

/*
 * Production dependencies
 */
#include "pg_upgrade.h"
#include "greenplum/pg_upgrade_greenplum.h"
#include "greenplum/old_tablespace_file_gp.h"
#include "greenplum/old_tablespace_file_contents.h"
#include "greenplum/old_tablespace_file_parser_observer.h"

/*
 * Test dependencies
 */
#include "utilities/gpdb5-cluster.h"
#include "utilities/gpdb6-cluster.h"
#include "utilities/test-helpers.h"
#include "utilities/query-helpers.h"

ClusterInfo old_cluster;
ClusterInfo new_cluster;
OSInfo os_info;
UserOpts user_opts;

void 
OldTablespaceFileParser_invalid_access_error_for_field(int row_index, int field_index)
{
	
}

void 
OldTablespaceFileParser_invalid_access_error_for_row(int row_index)
{
	
}

static void
test_populates_old_tablespace_file_contents_to_have_default_tablespace_records_for_gpdb6_cluster(void **state)
{
	ClusterInfo cluster;
	cluster.port = GPDB_FIVE_PORT;
	cluster.major_version = 80300; /* a GPDB 5 cluster */
	os_info.user = getenv("USER");
	cluster.sockdir = NULL;

	generate_old_tablespaces_file(&cluster);

	assert_false(get_old_tablespace_file_contents() == NULL);

	assert_int_equal(
		OldTablespaceFileContents_TotalNumberOfTablespaces(get_old_tablespace_file_contents()),
		2);
}

static void
test_filespaces_on_a_gpdb_five_cluster_are_loaded_as_old_tablespace_file_contents(void **state)
{
	system("rm -rf /tmp/tablespace-gp-test");
	system("mkdir /tmp/tablespace-gp-test");

	PGconn *connection = connectToFive();

	PGresult *result5 = executeQuery(connection, "CREATE FILESPACE my_fast_locations ( \n"
	                                    "1: '/tmp/tablespace-gp-test/fsseg-1/', \n"
	                                    "2: '/tmp/tablespace-gp-test/fsseg0/', \n"
	                                    "3: '/tmp/tablespace-gp-test/fsseg1/', \n"
	                                    "4: '/tmp/tablespace-gp-test/fsseg2/', \n"
	                                    "5: '/tmp/tablespace-gp-test/fsdummy1/', \n"
	                                    "6: '/tmp/tablespace-gp-test/fsdummy2/', \n"
	                                    "7: '/tmp/tablespace-gp-test/fsdummy3/', \n"
	                                    "8: '/tmp/tablespace-gp-test/fsdummy4/' );");
	PQclear(result5);

	PQclear(executeQuery(connection, "CREATE TABLESPACE my_fast_tablespace FILESPACE my_fast_locations;"));

	PQfinish(connection);
	
	ClusterInfo cluster;
	cluster.port = GPDB_FIVE_PORT;
	cluster.major_version = 10000; /* less than gpdb 6 */
	cluster.gp_dbid = 2;
	os_info.user = getenv("USER");
	cluster.sockdir = NULL;

	generate_old_tablespaces_file(&cluster);

	assert_false(get_old_tablespace_file_contents() == NULL);

	assert_int_equal(
		OldTablespaceFileContents_TotalNumberOfTablespaces(get_old_tablespace_file_contents()),
		3);

	char **results = OldTablespaceFileContents_GetArrayOfTablespacePaths(
		get_old_tablespace_file_contents());

	assert_string_equal(
		results[2],
		"/tmp/tablespace-gp-test/fsseg0");

	OldTablespaceRecord **records = OldTablespaceFileContents_GetTablespaceRecords(
		get_old_tablespace_file_contents()
		);

	assert_false(
		OldTablespaceRecord_GetIsUserDefinedTablespace(records[0]));
	assert_false(
		OldTablespaceRecord_GetIsUserDefinedTablespace(records[1]));
	assert_true(
		OldTablespaceRecord_GetIsUserDefinedTablespace(records[2]));
}

static void
setup_gpdb5(void **state)
{
	system("rm old_tablespaces.txt");

	stopGpdbFiveCluster();
	resetGpdbFiveDataDirectories();
	startGpdbFiveCluster();
}

static void
teardown_gpdb5(void **state)
{
	stopGpdbFiveCluster();
	resetGpdbFiveDataDirectories();
}

int
main(int argc, char *argv[])
{
	cmockery_parse_arguments(argc, argv);

	const		UnitTest tests[] = {
		unit_test_setup_teardown(test_filespaces_on_a_gpdb_five_cluster_are_loaded_as_old_tablespace_file_contents, setup_gpdb5, teardown_gpdb5),
		unit_test_setup_teardown(test_populates_old_tablespace_file_contents_to_have_default_tablespace_records_for_gpdb6_cluster, setup_gpdb5, teardown_gpdb5),
	};

	return run_tests(tests);
}
