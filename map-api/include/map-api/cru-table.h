#ifndef MAP_API_CRU_TABLE_H_
#define MAP_API_CRU_TABLE_H_

#include <vector>
#include <list>
#include <memory>
#include <map>
#include <string>

#include <Poco/Data/Common.h>
#include <gflags/gflags.h>

#include "./core.pb.h"
#include <map-api/cr-table.h>
#include <map-api/logical-time.h>
#include <map-api/revision.h>

namespace map_api {

/**
 * Provides interface to map api tables.
 */
class CRUTable : public CRTable {
  friend class Chunk;

 public:
  // Latest at front
  class History : public std::list<std::shared_ptr<const Revision> > {
   public:
    inline const_iterator latestAt(const LogicalTime& time) const;
  };
  typedef std::unordered_map<Id, History> HistoryMap;

  virtual ~CRUTable();
  /**
   * Field ID in revision must correspond to an already present item, revision
   * structure needs to match. Query may be modified according to the default
   * field policy.
   * Calls insertUpdatedCRUDerived and updateCurrentReferToUpdatedCRUDerived.
   * It is possible to specify update time for singular times of transactions.
   * TODO(tcies) make it the only possible way of setting time
   */
  void update(const std::shared_ptr<Revision>& query);
  void update(const std::shared_ptr<Revision>& query, const LogicalTime& time);
  bool getLatestUpdateTime(const Id& id, LogicalTime* time);

  void remove(const LogicalTime& time, const std::shared_ptr<Revision>& query);
  // avoid if possible - this is slower
  template <typename IdType>
  void remove(const LogicalTime& time, const IdType& id);

  template <typename ValueType>
  void findHistory(int key, const ValueType& value, const LogicalTime& time,
                   HistoryMap* dest) const;
  template <typename IdType>
  void itemHistory(
      const IdType& id, const LogicalTime& time, History* dest) const;

  virtual void findHistoryByRevision(int key, const Revision& valueHolder,
                                     const LogicalTime& time,
                                     HistoryMap* dest) const final;

  virtual Type type() const final override;

  /**
   * Default fields for internal management,
   */
  static const std::string kUpdateTimeField;
  static const std::string kRemovedField;

 private:
  virtual bool insertCRDerived(const LogicalTime& time,
                               const std::shared_ptr<Revision>& query)
      final override;
  virtual bool bulkInsertCRDerived(const NonConstRevisionMap& query,
                                   const LogicalTime& time) final override;
  /**
   * ================================================
   * FUNCTIONS TO BE IMPLEMENTED BY THE DERIVED CLASS
   * ================================================
   * The CRTable class contains most documentation on these functions.
   */
  virtual bool insertCRUDerived(const std::shared_ptr<Revision>& query) = 0;
  virtual bool bulkInsertCRUDerived(const NonConstRevisionMap& query) = 0;
  virtual bool patchCRDerived(const std::shared_ptr<Revision>& query)
      override = 0;

  /**
   * Implement insertion of the updated revision
   */
  virtual bool insertUpdatedCRUDerived(
      const std::shared_ptr<Revision>& query) = 0;
  virtual void findHistoryByRevisionCRUDerived(int key,
                                               const Revision& valueHolder,
                                               const LogicalTime& time,
                                               HistoryMap* dest) const = 0;
  virtual void chunkHistory(const Id& chunk_id, const LogicalTime& time,
                            HistoryMap* dest) const = 0;
  virtual void itemHistoryCRUDerived(const Id& id, const LogicalTime& time,
                                     History* dest) const = 0;
};

}  // namespace map_api

#include "./cru-table-inl.h"

#endif  // MAP_API_CRU_TABLE_H_
