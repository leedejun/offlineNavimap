#include "routing_wrapper.hpp"
#include "httplib.h"
#include "jsoner.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>


#include "search/search_tests_support/test_search_request.hpp"
#include "search/search_tests_support/test_search_engine.hpp"
#include "indexer/data_source_helpers.hpp"
#include "indexer/feature_algo.hpp"

using namespace std;
using namespace httplib;
using namespace search::tests_support;
using namespace conf;

Server svr;
string ConfigFileName= "config.cfg"; 
Config config(ConfigFileName);

GuidesTracks GetTestGuides()
  {
    // Guide with single track.
    GuidesTracks guides;
    guides[10] = {{{mercator::FromLatLon(48.13999, 11.56873), 10},
                 {mercator::FromLatLon(48.14096, 11.57246), 10},
                 {mercator::FromLatLon(48.14487, 11.57259), 10}}};
    return guides;
  }

int main(int argc, char * argv[])
{
  string ipAddress;
  string port;  
  config.readConfigFile("ipAddress",ipAddress);
  config.readConfigFile("port",port);
  
  svr.Get("/route", [](const Request& req, Response& res) {

    vector<string> sPoints;
    vector<string> ePoints;
    //type:(lon,lat;lon,lat;lon,lat)
    vector<string> viaPoints;
    vector<string> strPolygons;
    string vehicleType;
    string searchKey;
    if (req.has_param("sPoint")) {
      string sPoint = req.get_param_value("sPoint");
      boost::split(sPoints, sPoint,boost::is_any_of(","), boost::token_compress_on );
    }
    if (req.has_param("ePoint")) {
      string ePoint = req.get_param_value("ePoint");
      boost::split(ePoints, ePoint,boost::is_any_of(","), boost::token_compress_on );
    }
    if (req.has_param("viaPoints")) {
      string viaPoint = req.get_param_value("viaPoints");
      boost::split(viaPoints, viaPoint,boost::is_any_of(";"), boost::token_compress_on );
    }
    if(req.has_param("avoidArea")){
      string avoidArea = req.get_param_value("avoidArea");
      boost::split(strPolygons, avoidArea,boost::is_any_of("w"), boost::token_compress_on );
    }
    if (req.has_param("vehicleType")) {
      vehicleType = req.get_param_value("vehicleType");
    }
    if (req.has_param("searchKey")) {
      searchKey = req.get_param_value("searchKey");
    }
    
    res.set_header("Access-Control-Allow-Origin", "*");

    if(sPoints.size() == 0||ePoints.size() == 0){
      string error = "not find sPoint or ePoint!!";
      string str = jsoner<string>::to_json("500",error,"data", "null");
      res.set_content(str, "application/json");
      return;
    }

    routing::VehicleType type;
    if(vehicleType == "3"){
      type = VehicleType::Transit;
    }else{
      type = VehicleType::Car;
    }

    integration::IRouterComponents const & routerComponents = integration::GetVehicleComponents(type,config);

    vector<m2::PointD>  points;
    points.push_back(mercator::FromLatLon(atof(sPoints[1].c_str()), atof(sPoints[0].c_str())));
    
    if(viaPoints.size()>0){
      vector<string> viaPoint;
      vector<string>::const_iterator itor, last = viaPoints.end();
      for (itor = viaPoints.begin(); itor != last; ++itor) {
        boost::split(viaPoint, *itor,boost::is_any_of(","), boost::token_compress_on );
        if(viaPoint.size()!=2){
          continue;
        }
          points.push_back(mercator::FromLatLon(atof(viaPoint[1].c_str()), atof(viaPoint[0].c_str())));
      }
    }
    points.push_back(mercator::FromLatLon(atof(ePoints[1].c_str()), atof(ePoints[0].c_str())));
    Checkpoints checkpoints(points);

    vector<Polygon> polygons;
    Polygon polygon;
    if(strPolygons.size()>0){
      for(string p:strPolygons){
        vector<string> sp;
        boost::split(sp, p,boost::is_any_of(";"), boost::token_compress_on );
        for(string point:sp){
          vector<string> spoint;
          boost::split(spoint, point,boost::is_any_of(","), boost::token_compress_on );

          m2::PointD const & mercator = mercator::FromLatLon(atof(spoint[1].c_str()), atof(spoint[0].c_str()));
          boost::geometry::append ( polygon, Point(mercator.x,mercator.y) );
        }
      }
      polygons.emplace_back(polygon);
    }

    TRouteResult const routeResult =integration::CalculateRoute(routerComponents,
                                  checkpoints,
                                  GetTestGuides(),polygons);
    Route route = *routeResult.first;

    if(searchKey != ""){
      std::vector<m2::ParametrizedSegment<m2::PointD>> point = route.GetFollowedPolyline().GetSegProjMeters();
      std::vector<m2::ParametrizedSegment<m2::PointD>>::const_iterator pItor, pLast = point.end();
      Linestring line;
      for(pItor = point.begin(); pItor != pLast; ++pItor){
        boost::geometry::append ( line, Point(pItor->GetP0().x, pItor->GetP0().y) );
      }

      search::SearchParams params;
      params.m_query = searchKey;
      params.m_inputLocale = "zh";
      params.m_viewport = m2::RectD(m2::PointD(0.5, 0.5), m2::PointD(1.5, 1.5));
      //params.m_mode = search::Mode::Everywhere;
      //params.m_suggestsEnabled = false;
      params.m_line = line;

      search::Engine::Params engineParams;
      //engineParams.m_locale = "zh";
      //engineParams.m_numThreads = 1;
      TestSearchEngine m_engine(routerComponents.GetFeaturesFetcher()->GetDataSource(),engineParams);
      TestSearchRequest request(m_engine, params);
      request.Run();
    
      route.SetOnTheWayPOI(request.Results());
    }
    
    string str = jsoner<Route>::to_json("200","null","data", route);
    
    res.set_content(str, "application/json");
  });

  svr.Get("/searchPOI", [](const Request& req, Response& res) {

    string searchKey;
    //type:(lon,lat)
    string centralPoint;
    if (req.has_param("searchKey")) {
      searchKey = req.get_param_value("searchKey");
    }
    if (req.has_param("centralPoint")) {
      centralPoint = req.get_param_value("centralPoint");
    }
    
    res.set_header("Access-Control-Allow-Origin", "*");
    
    if(searchKey == ""){
      string error = "not find searchKey!!";
      string str = jsoner<string>::to_json("500",error,"data", "null");
      res.set_content(str, "application/json");
      return;
    }
    
    // if(!config.IncludeChinese(searchKey.c_str())){
    //   string error = "Please input Chinese!!";
    //   string str = jsoner<string>::to_json("500",error,"data", "null");
    //   res.set_content(str, "application/json");
    //   return;
    // }
    
    integration::IRouterComponents const & routerComponents = integration::GetVehicleComponents(VehicleType::Car,config);

    search::SearchParams params;
    params.m_query = searchKey;
    params.m_inputLocale = "zh";
    if(centralPoint != ""){
      vector<string> centPoints;
      boost::split(centPoints, centralPoint,boost::is_any_of(","), boost::token_compress_on );
      if(centPoints.size()!=2){
        string error = "centPoints type error!!";
        string str = jsoner<string>::to_json("500",error,"data", "null");
        res.set_content(str, "application/json");
        return;
      }
      params.m_position = m2::PointD (atof(centPoints[0].c_str()),atof(centPoints[1].c_str()));
    }
    params.m_viewport = m2::RectD(m2::PointD(0.5, 0.5), m2::PointD(1.5, 1.5));
    //params.m_mode = search::Mode::Everywhere;
    //params.m_suggestsEnabled = false;
  
    search::Engine::Params engineParams;
    //engineParams.m_locale = "zh";
    //engineParams.m_numThreads = 1;
    TestSearchEngine m_engine(routerComponents.GetFeaturesFetcher()->GetDataSource(),engineParams);
    TestSearchRequest request(m_engine, params);
    request.Run();
    vector<search::Result> result = request.Results();
    if(centralPoint != ""){
      sort(result.begin(),result.end(),[](const search::Result &result1, const search::Result &result2){
      return result1.GetRankingInfo().m_distanceToPivot<result2.GetRankingInfo().m_distanceToPivot;
    });
    }
    string str = jsoner<vector<search::Result>>::to_json("200","null","data", result);
    
    res.set_content(str, "application/json");
  });

  svr.Get("/searchPoint", [](const Request& req, Response& res) {

    vector<string> points;
    if (req.has_param("point")) {
      string point = req.get_param_value("point");
      boost::split(points, point,boost::is_any_of(","), boost::token_compress_on );
    }
    
    res.set_header("Access-Control-Allow-Origin", "*");
    
    if(points.size() != 2){
      string error = "point type error!!";
      string str = jsoner<string>::to_json("500",error,"data", "null");
      res.set_content(str, "application/json");
      return;
    }
    
    integration::IRouterComponents const & routerComponents = integration::GetVehicleComponents(VehicleType::Car,config);

    ms::LatLon latLon(atof(points[1].c_str()), atof(points[0].c_str()));
    m2::PointD const & mercator = mercator::FromLatLon(latLon);

    FeatureID featureId,fullMatch, poi, line, area;
    auto haveBuilding = false;
    auto closestDistanceToCenter = numeric_limits<double>::max();
    auto currentDistance = numeric_limits<double>::max();
    indexer::ForEachFeatureAtPoint(routerComponents.GetFeaturesFetcher()->GetDataSource(), [&](FeatureType & ft)
    {
      if (fullMatch.IsValid())
        return;

      switch (ft.GetGeomType())
      {
      case feature::GeomType::Point:
        poi = ft.GetID();
        break;
      case feature::GeomType::Line:
        // Skip/ignore isolines.
        if (ftypes::IsIsolineChecker::Instance()(ft))
          return;
        line = ft.GetID();
        break;
      case feature::GeomType::Area:
        // Buildings have higher priority over other types.
        if (haveBuilding)
          return;
        // Skip/ignore coastlines.
        if (feature::TypesHolder(ft).Has(classif().GetCoastType()))
          return;
        haveBuilding = ftypes::IsBuildingChecker::Instance()(ft);
        currentDistance = mercator::DistanceOnEarth(mercator, feature::GetCenter(ft));
        // Choose the first matching building or, if no buildings are matched,
        // the first among the closest matching non-buildings.
        if (!haveBuilding && currentDistance >= closestDistanceToCenter)
          return;
        area = ft.GetID();
        closestDistanceToCenter = currentDistance;
        break;
      case feature::GeomType::Undefined:
        ASSERT(false, ("case feature::Undefined"));
        break;
      }
    }, mercator);

    featureId = fullMatch.IsValid() ? fullMatch : (poi.IsValid() ? poi : (area.IsValid() ? area : line));

    osm::MapObject mapObject;
    //ASSERT(featureId.IsValid(), ());
    if(!featureId.IsValid()){
      string error = "data is not find!!";
      string str = jsoner<string>::to_json("500",error,"data", "null");
      res.set_content(str, "application/json");
      return;
    }
    FeaturesLoaderGuard guard(routerComponents.GetFeaturesFetcher()->GetDataSource(), featureId.m_mwmId);
    auto ft = guard.GetFeatureByIndex(featureId.m_index);
    if (ft)
      mapObject.SetFromFeatureType(*ft);

    string str = jsoner<osm::MapObject>::to_json("200","null","data", mapObject);
    
    res.set_content(str, "application/json");
  });

  svr.Get("/stop", [&](const Request& req, Response& res) { svr.stop(); });
  svr.listen(ipAddress.c_str(), atoi(port.c_str()));

  return 1;
}
