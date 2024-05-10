#pragma once

#include "routing_common/car_model.hpp"

namespace routing
{
// 策略
enum FtStrategy
{
  MotorwayFirst = 0,  // 高速优先
  NoMotorway = 1,     // 不走高速
  ShortestWay = 2,    // 最短距离
  Count = 3
};

class FtCarModelFactory : public VehicleModelFactory
{
public:
  FtCarModelFactory(CountryParentNameGetterFn const & countryParentNameGetterF,
                    const FtStrategy strategy);
};

}  // namespace routing