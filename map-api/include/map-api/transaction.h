#ifndef MAP_API_TRANSACTION_H_
#define MAP_API_TRANSACTION_H_
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <glog/logging.h>

#include <map-api/cache-base.h>
#include <map-api/logical-time.h>
#include <map-api/net-table.h>
#include <map-api/net-table-transaction.h>
#include <map-api/unique-id.h>

namespace map_api {
class Chunk;
class ChunkManagerBase;
class Revision;

class Transaction {
  friend class CacheBase;
  template <typename IdType, typename Value, typename DerivedValue>
  friend class Cache;

 public:
  Transaction();
  explicit Transaction(const LogicalTime& begin_time);

  // READ
  inline LogicalTime getBeginTime() const { return begin_time_; }
  /**
   * By Id or chunk:
   * Use the overload with chunk specification to increase performance. Use
   * dumpChunk() for best performance if reading out most of a chunk.
   */
  template <typename IdType>
  std::shared_ptr<const Revision> getById(const IdType& id,
                                          NetTable* table) const;
  template <typename IdType>
  std::shared_ptr<const Revision> getById(const IdType& id, NetTable* table,
                                          Chunk* chunk) const;
  CRTable::RevisionMap dumpChunk(NetTable* table, Chunk* chunk);
  CRTable::RevisionMap dumpActiveChunks(NetTable* table);
  template <typename IdType>
  void getAvailableIds(NetTable* table, std::vector<IdType>* ids);
  /**
   * By some other field: Searches in ALL active chunks of a table, thus
   * fundamentally differing from getById or dumpChunk.
   */
  template <typename ValueType>
  CRTable::RevisionMap find(int key, const ValueType& value, NetTable* table);

  // WRITE
  void insert(
      NetTable* table, Chunk* chunk, std::shared_ptr<Revision> revision);
  /**
   * Uses ChunkManager to auto-size chunks.
   */
  void insert(ChunkManagerBase* chunk_manager,
              std::shared_ptr<Revision> revision);
  void update(NetTable* table, std::shared_ptr<Revision> revision);
  // fast
  void remove(NetTable* table, std::shared_ptr<Revision> revision);
  // slow
  template <typename IdType>
  void remove(NetTable* table, const UniqueId<IdType>& id);

  // TRANSACTION OPERATIONS
  bool commit();
  inline LogicalTime getCommitTime() const { return commit_time_; }
  using Conflict = ChunkTransaction::Conflict;
  using Conflicts = ChunkTransaction::Conflicts;
  class ConflictMap
      : public std::unordered_map<NetTable*, ChunkTransaction::Conflicts> {
   public:
    inline std::string debugString() const {
      std::ostringstream ss;
      for (const value_type& pair : *this) {
        ss << pair.first->name() << ": " << pair.second.size() << " conflicts"
           << std::endl;
      }
      return ss.str();
    }
  };

  /**
   * Merge_transaction will be filled with all insertions and non-conflicting
   * updates from this transaction, while the conflicting updates will be
   * represented in a ConflictMap.
   */
  void merge(const std::shared_ptr<Transaction>& merge_transaction,
             ConflictMap* conflicts);
  size_t numChangedItems() const;

 private:
  void attachCache(NetTable* table, CacheBase* cache);
  void enableDirectAccessForCache();
  void disableDirectAccessForCache();

  NetTableTransaction* transactionOf(NetTable* table) const;

  void ensureAccessIsCache(NetTable* table) const;
  void ensureAccessIsDirect(NetTable* table) const;

  /**
   * A global ordering of tables prevents deadlocks (resource hierarchy
   * solution)
   */
  struct NetTableOrdering {
    inline bool operator()(const NetTable* a, const NetTable* b) const {
      return CHECK_NOTNULL(a)->name() < CHECK_NOTNULL(b)->name();
    }
  };
  typedef std::map<NetTable*, std::shared_ptr<NetTableTransaction>,
      NetTableOrdering> TransactionMap;
  typedef TransactionMap::value_type TransactionPair;
  mutable TransactionMap net_table_transactions_;
  LogicalTime begin_time_, commit_time_;

  // direct access vs. caching
  enum class TableAccessMode {
    kDirect,
    kCache
  };
  typedef std::unordered_map<NetTable*, TableAccessMode> TableAccessModeMap;
  /**
   * A table may only be accessed directly through a transaction or through a
   * cache, but not both. Otherwise, getting uncommitted entries becomes rather
   * complicated.
   */
  mutable TableAccessModeMap access_mode_;
  typedef std::unordered_map<NetTable*, CacheBase*> CacheMap;
  CacheMap attached_caches_;
  /**
   * Cache must be able to access transaction directly, even though table
   * is in cache access mode. This on a per-thread basis.
   */
  std::unordered_set<std::thread::id> cache_access_override_;
};

}  // namespace map_api

#include "./transaction-inl.h"

#endif  // MAP_API_TRANSACTION_H_