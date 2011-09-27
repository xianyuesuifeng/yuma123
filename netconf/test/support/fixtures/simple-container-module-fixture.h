#ifndef __YUMA_SIMPLE_CONTAINER_MODULE_TEST_FIXTURE__H
#define __YUMA_SIMPLE_CONTAINER_MODULE_TEST_FIXTURE__H

// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/base-suite-fixture.h"
#include "test/support/msg-util/NCMessageBuilder.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <vector>
#include <map>
#include <string>
#include <memory>

// ---------------------------------------------------------------------------|
namespace YumaTest 
{
class AbstractNCSession;

// ---------------------------------------------------------------------------|
/**
 * This class is used to perform simple test case initialisation.
 * It can be used on a per test case basis or on a per test suite
 * basis.
 */
struct SimpleContainerModuleFixture : public BaseSuiteFixture
{
public:
    /** Convenience typedef */
    typedef std::map< std::string, std::string > EntryMap_T;

    /** Convenience typedef */
    typedef std::shared_ptr< EntryMap_T > SharedPtrEntryMap_T;

public:
    /** 
     * Constructor. 
     */
    SimpleContainerModuleFixture();

    /**
     * Destructor. Shutdown the test.
     */
    ~SimpleContainerModuleFixture();

    /**
     * Run an edit query.
     *
     * \param session the session running the query
     * \param query the query to run
     */
    void runEditQuery( std::shared_ptr<AbstractNCSession> session,
                       const std::string& query );

    /** 
     * Create the top level container.
     *
     * \param session the session running the query
     */
    void createMainContainer( std::shared_ptr<AbstractNCSession> session );

    /** 
     * Delete the top level container.
     *
     * \param session the session running the query
     */
    void deleteMainContainer( std::shared_ptr<AbstractNCSession> session );

    /** 
     * Add an entry.
     *
     * \param session the session running the query
     * \param entryKeyStr the name of the entry key to add.
     */
    void addEntry( std::shared_ptr<AbstractNCSession> session,
                   const std::string& entryKeyStr );

    /** 
     * Add an entry value.
     *
     * \param session the session running the query
     * \param entryKeyStr the name of the entry key.
     * \param entryValStr the value of the entry.
     */
    void addEntryValue( std::shared_ptr<AbstractNCSession> session,
                        const std::string& entryKeyStr,
                        const std::string& entryValStr );

    /** 
     * Add an entry key and value.
     *
     * \param session the session running the query
     * \param entryKeyStr the name of the entry key.
     * \param entryValStr the value of the entry.
     */
    void addEntryValuePair( std::shared_ptr<AbstractNCSession> session,
                            const std::string& entryKeyStr,
                            const std::string& entryValStr );

    /**
     * Populate the database with the entries. This function creates
     * numEntries elements in the database, each having a key matching
     * the following format "entryKey#". This function uses the
     * primarySession_ for all operations.
     *
     * \param numEntries the number of entries to add.
     */
    void populateDatabase( const uint16_t numEntries );

    /** 
     * Edit an entry key and value.
     *
     * \param session the session running the query
     * \param entryKeyStr the name of the entry key.
     * \param entryValStr the value of the entry.
     */
    void editEntryValue( std::shared_ptr<AbstractNCSession> session,
                         const std::string& entryKeyStr,
                         const std::string& entryValStr );

    /**
     * Verify the entries in the specified database.
     * This function checks that all expected entries are present in
     * the specified database. It also checks that both databases
     * differ as expected. 
     *
     * e.g.:
     * If the candidate database is being checked it makes sure that
     * any values in the candidate database that should be different
     * are not present in the running database.
     *
     * TODO: This checking is still very crude - it currently does not
     *       support databases with multiple values that are the same.
     * 
     * \param session the session running the query
     * \param targetDbName the name of the database to check
     * \param refMap the map that corresponds to the expected values
     *               in the database being checked.
     * \param otherMap the map that corresponds to the entries in the
     *                 database not being checked.
     */
    void checkEntriesImpl(
        std::shared_ptr<AbstractNCSession> session,
        const std::string& targetDbName,
        const SharedPtrEntryMap_T refMap,
        const SharedPtrEntryMap_T otherMap );

    /**
     * Check the status of both databases.
     *
     * \param session the session running the query
     */
    void checkEntries( 
        std::shared_ptr<AbstractNCSession> session );

    /**
     * Commit the changes.
     *
     * \param session  the session requesting the locks
     */
    virtual void commitChanges( std::shared_ptr<AbstractNCSession> session );

    /**
     * Let the test harness know that changes shoudl be discarded
     * (e.g. due to unlocking the database without a commit.
     */
    void discardChanges();

    /** The NCMessage builder for the writeable database */
    NCMessageBuilder wrBuilder_;

    const std::string moduleName_;      ///< The module name
    const std::string moduleNs_;        ///< the module namespace
    const std::string containerName_;   ///< the container name 

    SharedPtrEntryMap_T runningEntries_; /// Running Db Entries 
    SharedPtrEntryMap_T candidateEntries_; /// CandidateTarget Db Entries 
};

} // namespace YumaTest

#endif // __YUMA_SIMPLE_CONTAINER_MODULE_TEST_FIXTURE__H
