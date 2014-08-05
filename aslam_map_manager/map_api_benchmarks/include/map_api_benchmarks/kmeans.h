#ifndef MAP_API_BENCHMARKS_KMEANS_H_
#define MAP_API_BENCHMARKS_KMEANS_H_

#include <memory>
#include <vector>

#include <Eigen/Core>

#include <aslam_posegraph/common.h>
#include <aslam_posegraph/pose-graph.h>

#include "map_api_benchmarks/simple_kmeans.h"

#include "kmeans.pb.h"

namespace map_api {
namespace benchmarks {

typedef float Scalar;
typedef Eigen::Matrix<Scalar, Eigen::Dynamic, 1> DescriptorType;
static const int kDescriptorDimensionality = 10;
typedef Aligned<std::vector, DescriptorType>::type DescriptorVector;

typedef map_api::benchmarks::SimpleKmeans<DescriptorType,
    map_api::benchmarks::distance::L2<DescriptorType>,
    Eigen::aligned_allocator<DescriptorType> > KMeans2D;

class Kmeans : public KMeans2D {
 public:
  bool saveToDatabase() const;
  bool loadFromDatabase(
      const std::vector<DataPointId>& data_points,
      const std::vector<CenterId>& centers);
  bool loadEntireDatabase();

  bool addDataPoint(const DataPointId& id, const CenterId& center_id,
                    const Eigen::VectorXd& data);

  bool addCenter(const CenterId& from, const Eigen::VectorXd& data);

  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

 private:

};

}  // namespace benchmarks
}  // namespace map_api

#endif  // MAP_API_BENCHMARKS_KMEANS_H_
