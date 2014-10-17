#ifndef MAP_API_CACHE_INL_H_
#define MAP_API_CACHE_INL_H_

#include <utility>

#include <glog/logging.h>
#include <timing/timer.h>

namespace map_api {

template <typename IdType, typename Value, typename DerivedValue>
Cache<IdType, Value, DerivedValue>::Cache(
    const std::shared_ptr<Transaction>& transaction, NetTable* const table,
    const std::shared_ptr<ChunkManagerBase>& chunk_manager)
    : ids_fetched_(false),
      underlying_table_(CHECK_NOTNULL(table)),
      chunk_manager_(chunk_manager),
      staged_(false),
      transaction_(transaction) {
  CHECK_NOTNULL(transaction.get());
  CHECK_NOTNULL(chunk_manager.get());

  transaction_.get()->attachCache(underlying_table_, this);
}

template <typename IdType, typename Value, typename DerivedValue>
Cache<IdType, Value, DerivedValue>::~Cache() {}

template <typename IdType, typename Value, typename DerivedValue>
Value& Cache<IdType, Value, DerivedValue>::get(const IdType& id) {
  LockGuard lock(mutex_);
  typename CacheMap::iterator found = this->cache_.find(id);
  if (found == this->cache_.end()) {
    std::shared_ptr<const Revision> revision = getRevisionLocked(id);
    CHECK(revision);
    std::pair<typename CacheMap::iterator, bool> cache_insertion;

    cache_insertion = cache_.emplace(id, Factory::getNewInstance());
    CHECK(cache_insertion.second);
    objectFromRevision(
        *revision, Factory::getPointerToDerived(cache_insertion.first->second));
    found = cache_insertion.first;
  }
  return found->second;
}

template <typename IdType, typename Value, typename DerivedValue>
const Value& Cache<IdType, Value, DerivedValue>::get(const IdType& id) const {
  LockGuard lock(mutex_);
  typename CacheMap::iterator found = this->cache_.find(id);
  if (found == this->cache_.end()) {
    std::shared_ptr<const Revision> revision = getRevisionLocked(id);
    CHECK(revision);
    std::pair<typename CacheMap::iterator, bool> cache_insertion;
    cache_insertion = cache_.emplace(id, Factory::getNewInstance());
    CHECK(cache_insertion.second);
    objectFromRevision(
        *revision, Factory::getPointerToDerived(cache_insertion.first->second));
    found = cache_insertion.first;
  }
  return found->second;
}

template <typename IdType, typename Value, typename DerivedValue>
bool Cache<IdType, Value, DerivedValue>::insert(const IdType& id,
                                                const Value& value) {
  LockGuard lock(mutex_);
  typename IdSet::iterator found = getAvailableIdsLocked().find(id);
  if (found != getAvailableIdsLocked().end()) {
    return false;
  }
  CHECK(cache_.emplace(id, value).second);
  CHECK(getAvailableIdsLocked().emplace(id).second);
  return true;
}

template <typename IdType, typename Value, typename DerivedValue>
void Cache<IdType, Value, DerivedValue>::erase(const IdType& id) {
  CHECK(underlying_table_->type() == CRTable::Type::CRU);
  LockGuard lock(mutex_);
  cache_.erase(id);
  getAvailableIdsLocked().erase(id);
  removals_.emplace(id);
}

template <typename IdType, typename Value, typename DerivedValue>
bool Cache<IdType, Value, DerivedValue>::has(const IdType& id) const {
  LockGuard lock(mutex_);
  typename IdSet::const_iterator found = getAvailableIdsLocked().find(id);
  return found != getAvailableIdsLocked().end();
}

template <typename IdType, typename Value, typename DerivedValue>
void Cache<IdType, Value, DerivedValue>::getAllAvailableIds(
    std::unordered_set<IdType>* available_ids) const {
  LockGuard lock(mutex_);
  CHECK_NOTNULL(available_ids);
  available_ids->clear();
  available_ids->insert(getAvailableIdsLocked().begin(),
                        getAvailableIdsLocked().end());
}

template <typename IdType, typename Value, typename DerivedValue>
size_t Cache<IdType, Value, DerivedValue>::size() const {
  LockGuard lock(mutex_);
  return getAvailableIdsLocked().size();
}

template <typename IdType, typename Value, typename DerivedValue>
bool Cache<IdType, Value, DerivedValue>::empty() const {
  LockGuard lock(mutex_);
  return getAvailableIdsLocked().empty();
}

template <typename IdType, typename Value, typename DerivedValue>
std::shared_ptr<const Revision>
Cache<IdType, Value, DerivedValue>::getRevisionLocked(
    const IdType& id) const {
  typedef CRTable::RevisionMap::iterator RevisionIterator;
  RevisionIterator found = revisions_.find(id);
  if (found == revisions_.end()) {
    std::shared_ptr<const Revision> revision =
        transaction_.get()->getById(id, underlying_table_);
    CHECK(revision);
    std::pair<RevisionIterator, bool> insertion =
        revisions_.insert(id, revision);
    CHECK(insertion.second);
    found = insertion.first;
  }
  return found->second;
}

template <typename IdType, typename Value, typename DerivedValue>
void Cache<IdType, Value, DerivedValue>::prepareForCommit() {
  LockGuard lock(mutex_);
  CHECK(!staged_);
  for (const typename CacheMap::value_type& cached_pair : cache_) {
    CRTable::RevisionMap::iterator corresponding_revision =
        revisions_.find(cached_pair.first);
    if (corresponding_revision == revisions_.end()) {
      // All items that were in the db before must have been gotten through
      // the revision cache, so an item not present in the revision cache must
      // have been inserted newly.
      std::shared_ptr<Revision> insertion = underlying_table_->getTemplate();
      objectToRevision(cached_pair.first,
                       Factory::getReferenceToDerived(cached_pair.second),
                       insertion.get());
      transaction_.get()->insert(chunk_manager_.get(), insertion);
    } else {
      // Convert the object to the revision and then compare if it has changed.
      std::shared_ptr<map_api::Revision> update_revision =
          std::make_shared<map_api::Revision>(
              *corresponding_revision->second);
      objectToRevision(cached_pair.first,
                       Factory::getReferenceToDerived(cached_pair.second),
                       update_revision.get());
      if (*update_revision != *corresponding_revision->second) {
        transaction_.get()->update(underlying_table_, update_revision);
      }
    }
  }
  for (const IdType& id : removals_) {
    std::shared_ptr<Revision> to_remove =
        std::make_shared<Revision>(*getRevisionLocked(id));
    transaction_.get()->remove(underlying_table_, to_remove);
  }
  staged_ = true;
}

template <typename IdType, typename Value, typename DerivedValue>
typename Cache<IdType, Value, DerivedValue>::IdSet&
Cache<IdType, Value, DerivedValue>::getAvailableIdsLocked() {
  if (!ids_fetched_) {
    timing::Timer timer("getAvailableIds");
    transaction_.get()->getAvailableIds(underlying_table_, &available_ids_);
    timer.Stop();
    ids_fetched_ = true;
  }
  return available_ids_;
}

template <typename IdType, typename Value, typename DerivedValue>
const typename Cache<IdType, Value, DerivedValue>::IdSet&
Cache<IdType, Value, DerivedValue>::getAvailableIdsLocked() const {
  if (!ids_fetched_) {
    timing::Timer timer("getAvailableIds");
    transaction_.get()->getAvailableIds(underlying_table_, &available_ids_);
    timer.Stop();
    ids_fetched_ = true;
  }
  return available_ids_;
}

}  // namespace map_api

#endif  // MAP_API_CACHE_INL_H_
