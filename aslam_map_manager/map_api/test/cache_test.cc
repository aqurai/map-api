#include <set>

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <multiagent_mapping_common/test/testing_entrypoint.h>

#include "map-api/cache.h"
#include "map-api/chunk-manager.h"
#include "map-api/ipc.h"

#include "net_table_test_fixture.cc"

namespace map_api {

UNIQUE_ID_DEFINE_ID(IntId);
MAP_API_REVISION_UNIQUE_ID(IntId);

void objectFromRevision(const Revision& revision, int* object) {
  revision.get(NetTableTest::kFieldName, CHECK_NOTNULL(object));
}
void objectToRevision(const int& object, Revision* revision) {
  CHECK_NOTNULL(revision)->set(NetTableTest::kFieldName, object);
}
bool requiresUpdate(const int& object, const Revision& revision) {
  return !revision.verifyEqual(NetTableTest::kFieldName, object);
}

}  // namespace map_api

UNIQUE_ID_DEFINE_ID_HASH(map_api::IntId);

namespace map_api {

TEST_P(NetTableTest, Cache) {
  enum SubProcesses {
    ROOT,
    A
  };
  enum Barriers {
    INIT,
    ROOT_INSERTED,
    A_DONE
  };
  constexpr int kKb = 1024;
  std::shared_ptr<Transaction> transaction;
  std::shared_ptr<ChunkManagerChunkSize> manager;
  std::shared_ptr<Cache<IntId, int>> cache;
  IntId kId[3];
  std::shared_ptr<int> kVal[3];
  for (int i = 0; i < 3; ++i) {
    generateIdFromInt(i + 1, &kId[i]);
    kVal[i].reset(new int(i));
  }
  std::unordered_set<IntId> id_result;
  if (getSubprocessId() == ROOT) {
    launchSubprocess(A);
    transaction.reset(new Transaction);
    manager.reset(new ChunkManagerChunkSize(kKb, table_));
    cache.reset(new Cache<IntId, int>(transaction, table_, manager));
    cache->getAllAvailableIds(&id_result);
    EXPECT_TRUE(id_result.empty());
    for (int i = 0; i < 3; ++i) {
      EXPECT_FALSE(cache->has(kId[i]));
    }
    EXPECT_TRUE(cache->insert(kId[0], kVal[0]));
    EXPECT_TRUE(transaction->commit());
    IPC::barrier(INIT, 1);
    manager->requestParticipationAllChunks();
    IPC::barrier(ROOT_INSERTED, 1);
    IPC::barrier(A_DONE, 1);

    transaction.reset(new Transaction);
    manager.reset(new ChunkManagerChunkSize(kKb, table_));
    cache.reset(new Cache<IntId, int>(transaction, table_, manager));
    cache->getAllAvailableIds(&id_result);
    EXPECT_EQ(2u, id_result.size());
    ASSERT_TRUE(cache->has(kId[0]));
    ASSERT_TRUE(cache->has(kId[1]));
    EXPECT_FALSE(cache->has(kId[2]));
    EXPECT_EQ((GetParam() ? (*kVal[2]) : (*kVal[0])), cache->get(kId[0]));
    EXPECT_EQ(*kVal[1], cache->get(kId[1]));
  }
  if (getSubprocessId() == A) {
    IPC::barrier(INIT, 1);
    IPC::barrier(ROOT_INSERTED, 1);
    transaction.reset(new Transaction);
    manager.reset(new ChunkManagerChunkSize(kKb, table_));
    cache.reset(new Cache<IntId, int>(transaction, table_, manager));
    if (GetParam()) {
      CHECK(cache->has(kId[0]));
      cache->get(kId[0]) = *kVal[2];
    }
    CHECK(cache->insert(kId[1], kVal[1]));
    CHECK(transaction->commit());
    manager->requestParticipationAllChunks();
    IPC::barrier(A_DONE, 1);
  }
}

}  // namespace map_api

MULTIAGENT_MAPPING_UNITTEST_ENTRYPOINT