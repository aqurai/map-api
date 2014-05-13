/**
 * A test table revealing some more internals than a typical table, such as
 * template, database session and cleanup.
 */
#include <map-api/cru-table-interface.h>
#include <glog/logging.h>

template <typename TableInterfaceType>
class TestTable : public TableInterfaceType {
 public:
  TestTable(map_api::Id owner) : TableInterfaceType(owner) {}
  ~TestTable() {}
  virtual bool init(){
    this->setup("test_table");
    return true;
  }
  std::shared_ptr<Poco::Data::Session> sessionForward(){
    return std::shared_ptr<Poco::Data::Session>(this->session_);
  }
  void cleanup(){
    *(sessionForward()) << "DROP TABLE IF EXISTS " << this->name(),
        Poco::Data::now;
    LOG(INFO) << "Table " << this->name() << " dropped";
  }
 protected:
  virtual bool define(){
    return true;
  }

 public:
  using TableInterfaceType::rawInsertQuery;
  using TableInterfaceType::rawGetRow;
};
