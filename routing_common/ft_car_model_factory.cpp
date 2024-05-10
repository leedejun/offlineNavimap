#include "ft_car_model_factory.hpp"

#include <fstream>
#include <map>
#include <memory>
#include <mutex>

#include "../services/json.hpp"
#include "../platform/Platform.hpp"


namespace
{
struct FtStrategyInitData
{
  using json = nlohmann::json;

  routing::VehicleModel::LimitsInitList limitsInitList[routing::FtStrategy::Count];
  routing::HighwayBasedFactors highwayBasedFactors[routing::FtStrategy::Count];
  routing::HighwayBasedSpeeds highwayBasedSpeeds[routing::FtStrategy::Count];
  std::string strStrategy[routing::FtStrategy::Count] = {"MotorwayFirst", "NoMotorway",
                                                         "ShortestWay"};

  FtStrategyInitData() { init(); }

  void init()
  {
    std::string jsonDir = GetPlatform().WritableDir();
//    std::string jsonDir="../services";
    std::ifstream jfile(jsonDir + "/strategy.cfg");
    json j;
    jfile >> j;

    std::map<std::string, json> mapJson;
    for (size_t i = 0; i < j.size(); i++)
    {
      auto it = mapJson.find(j[i]["strategy"]);
      if (it == mapJson.end())
      {
        mapJson[j[i]["strategy"]] = j[i];
      }
    }
    for (size_t i = 0; i < routing::FtStrategy::Count; i++)
    {
      initLimitsInitList(limitsInitList[i], mapJson[strStrategy[i]]);
      initHighwayBasedFactors(highwayBasedFactors[i], mapJson[strStrategy[i]]);
      initHighwayBasedSpeeds(highwayBasedSpeeds[i], mapJson[strStrategy[i]]);
    }
  }

  void initLimitsInitList(routing::VehicleModel::LimitsInitList & limitsInitList, const json & j)
  {
    auto jsonFeatureType = j["FeatureTypeLimitsList"];
    for (size_t i = 0; i < jsonFeatureType.size(); i++)
    {
      routing::VehicleModel::FeatureTypeLimits limit;
      for (auto item : jsonFeatureType[i]["type"])
      {
        limit.m_types.push_back(item);
      }
      limit.m_isPassThroughAllowed = jsonFeatureType[i]["isPassThroughAllowed"];
      limitsInitList.push_back(limit);
    }
  }
  void initHighwayBasedFactors(routing::HighwayBasedFactors & highwayBasedFactors, const json & j)
  {
    auto jsonFactors = j["HighwayBasedFactors"];
    for (size_t i = 0; i < jsonFactors.size(); i++)
    {
      routing::HighwayType type = (routing::HighwayType)jsonFactors[i]["code"];
      routing::InOutCityFactor factor;
      std::vector<double> tmpVecDouble;
      for (auto item : jsonFactors[i]["InOutCityFactor"])
      {
        tmpVecDouble.push_back(item);
      }
      switch (tmpVecDouble.size())
      {
      case 1:
      {
        routing::SpeedFactor speedFactor(tmpVecDouble[0]);
        factor.setInCity(speedFactor);
        factor.setOutCity(speedFactor);
      }
      break;
      case 2:
      {
        routing::SpeedFactor inFactor(tmpVecDouble[0]);
        routing::SpeedFactor outFactor(tmpVecDouble[1]);
        factor.setInCity(inFactor);
        factor.setOutCity(outFactor);
      }
      break;
      case 4:
      {
        routing::SpeedFactor inFactor(tmpVecDouble[0], tmpVecDouble[1]);
        routing::SpeedFactor outFactor(tmpVecDouble[2], tmpVecDouble[3]);
        factor.setInCity(inFactor);
        factor.setOutCity(outFactor);
      }
      break;
      default: CHECK(false, ("InOutCityFactor init error ", tmpVecDouble.size()));
      }
      highwayBasedFactors[type] = factor;
    }
  }
  void initHighwayBasedSpeeds(routing::HighwayBasedSpeeds & highwayBasedSpeeds, const json & j)
  {
    auto jsonSpeeds = j["HighwayBasedSpeeds"];
    for (size_t i = 0; i < jsonSpeeds.size(); i++)
    {
      routing::HighwayType type = (routing::HighwayType)jsonSpeeds[i]["code"];
      routing::InOutCitySpeedKMpH speed;
      std::vector<double> tmpVecDouble;
      for (auto item : jsonSpeeds[i]["InOutCitySpeedKMpH"])
      {
        tmpVecDouble.push_back(item);
      }
      switch (tmpVecDouble.size())
      {
      case 1:
      {
        routing::SpeedKMpH speedKMpH(tmpVecDouble[0]);
        speed.setInCity(speedKMpH);
        speed.setOutCity(speedKMpH);
      }
      break;
      case 2:
      {
        routing::SpeedKMpH inSpeedKMpH(tmpVecDouble[0]);
        routing::SpeedKMpH outSpeedKMpH(tmpVecDouble[1]);
        speed.setInCity(inSpeedKMpH);
        speed.setOutCity(outSpeedKMpH);
      }
      break;
      case 4:
      {
        routing::SpeedKMpH inSpeedKMpH(tmpVecDouble[0], tmpVecDouble[1]);
        routing::SpeedKMpH outSpeedKMpH(tmpVecDouble[2], tmpVecDouble[3]);
        speed.setInCity(inSpeedKMpH);
        speed.setOutCity(outSpeedKMpH);
      }
      break;
      default: CHECK(false, ("InOutCityFactor init error ", tmpVecDouble.size()));
      }
      highwayBasedSpeeds[type] = speed;
    }
  }
};

std::once_flag initFlag;
static FtStrategyInitData * pFtStrategyInitData;

void initFtStrategyInitData() { pFtStrategyInitData = new FtStrategyInitData(); }
}  // namespace

namespace routing
{
FtCarModelFactory::FtCarModelFactory(CountryParentNameGetterFn const & countryParentNameGetterF,
                                     const FtStrategy strategy)
  : VehicleModelFactory(countryParentNameGetterF)
{
  std::call_once(initFlag, initFtStrategyInitData);
  m_models[""] = std::make_shared<CarModel>(
      pFtStrategyInitData->limitsInitList[strategy],
      HighwayBasedInfo(pFtStrategyInitData->highwayBasedSpeeds[strategy],
                       pFtStrategyInitData->highwayBasedFactors[strategy]));
}

}  // namespace routing