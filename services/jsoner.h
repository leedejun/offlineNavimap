#ifndef __JSON_HELPER_H
#define __JSON_HELPER_H
 
#include <time.h>
#include <string>
#include <vector>
#include <sstream>
#include <boost/shared_ptr.hpp>
#include <search/result.hpp>

#include "indexer/map_object.hpp"

template<typename T>
class jsoner {
 public:
  static std::string to_json (std::string const& code, std::string const& error,std::string const& name, T const& value) {
    std::stringstream stream;
    stream << "{\"code\":" << "\"" << code <<"\",";  
    stream << "\"error\":" << "\"" << error <<"\",";  
    stream << "\"" << name << "\":";
    stream << value.to_json();
    stream << "}";
    return stream.str();
  }
};
 
template<>
class jsoner<std::string> {
 public:
  static std::string to_json (std::string const& code, std::string const& error,std::string const& name, std::string const & value) {
    std::stringstream stream;
    stream << "{\"code\":" << "\"" << code <<"\",";  
    stream << "\"error\":" << "\"" << error <<"\",";  
    stream << "\"" << name << "\":\""<< value <<"\"}";
    return stream.str();
  }
};

template<>
class jsoner<std::vector<std::string> > {
 public:
  static std::string to_json (std::string const& code, std::string const& error,std::string const& name, std::vector<std::string> const & value) {
    std::vector<std::string>::const_iterator itor, last = value.end();
    std::stringstream stream;
    stream << "{\"code\":" << "\"" << code <<"\",";  
    stream << "\"error\":" << "\"" << error <<"\",";  
    stream << "\"" << name << "\":[";
    int i = 0;
    for (itor = value.begin(); itor != last; ++itor) {
      stream << "{";
      stream << "\"index\":" << "\"" << i << "\",";
      stream << "\"value\":" << "\"" << *itor << "\"";
      stream << "}";
      if(itor != last -1) {
	stream << ",";
      }
      ++i;
    }
    stream << "]}";
    return stream.str();
  }
 
};

template<>
class jsoner<std::vector<double> > {
 public:
  static std::string to_json (std::string const& code, std::string const& error,std::string const& name, std::vector<double> const & value) {
    std::vector<double>::const_iterator itor, last = value.end();
    std::stringstream stream;
    stream << "{\"code\":" << "\"" << code <<"\",";  
    stream << "\"error\":" << "\"" << error <<"\",";  
    stream << "\"" << name << "\":[";
    int i = 0;
    for (itor = value.begin(); itor != last; ++itor) {
      stream << "{";
      stream << "\"index\":" << "\"" << i << "\",";
      stream << "\"value\":" << "\"" << *itor << "\"";
      stream << "}";
      if(itor != last -1) {
	stream << ",";
      }
      ++i;
    }
    stream << "]}";
    return stream.str();
  }
 
};

template<>
class jsoner<Route> {
 public:
  static std::string to_json (std::string const& code, std::string const& error,std::string const& name,Route const & route) {
    std::vector<RouteSegment> segment = route.GetRouteSegments();
    std::vector<m2::ParametrizedSegment<m2::PointD>> point = route.GetFollowedPolyline().GetSegProjMeters();
    std::vector<search::Result> viaPoi = route.GetViaPOI();
    std::vector<RouteSegment>::const_iterator sItor, sLast = segment.end();
    std::vector<m2::ParametrizedSegment<m2::PointD>>::const_iterator pItor, pLast = point.end();
    std::vector<search::Result>::const_iterator vItor, vLast = viaPoi.end();
    std::stringstream stream;
    stream << "{\"code\":" << "\"" << code <<"\",";  
    stream << "\"error\":" << "\"" << error <<"\",";  
    stream << "\"" << name << "\":[";
    int i = 0;
    for (sItor = segment.begin(); sItor != sLast; ++sItor) {
      stream << "{";
      stream << "\"index\":" << "\"" << i << "\",";
      std::string street = sItor->GetStreet();
      std::string turnStr = GetTurnString(sItor->GetTurn().m_turn);
      std::string targetName = sItor->GetTurn().m_targetName;
      std::string sourceName = sItor->GetTurn().m_sourceName;
      if(street==""){
        street = "无名路";
      }
      if(targetName==""){
        targetName = "无名路";
      }
      if(sourceName==""){
        sourceName = "无名路";
      }
      stream << "\"m_street\":" << "\"" << street << "\",";
      stream << "\"turnString\":" << "\"" << turnStr << "\",";
      stream << "\"targetName\":" << "\"" << targetName << "\",";
      stream << "\"sourceName\":" << "\"" << sourceName << "\",";
      int j = 0;
      for(pItor = point.begin(); pItor != pLast; ++pItor){
        if(i!=j){
          ++j;
          continue;
        }
        stream << "\"lon\":" << "\"" << std::to_string(pItor->GetP0().x) << "\",";
        stream << "\"lat\":" << "\"" << std::to_string(pItor->GetP0().y) << "\",";
        stream << "\"length\":" << "\"" << pItor->GetLength() << "\"";
        break;
      }
      stream << "}";
      if(sItor != sLast -1) {
	      stream << ",";
      }
      ++i;
    }
    stream << "],\"viaPoi\":[";
    i = 0;
    for (vItor = viaPoi.begin(); vItor != vLast; ++vItor) {
      stream << "{";
      stream << "\"index\":" << "\"" << i << "\",";
      std::string poiName = vItor->GetString();
      std::string address = vItor->GetAddress();
      if(poiName==""){
        poiName = "无名";
      }
      if(address==""){
        address = "暂无地址";
      }
      stream << "\"poiName\":" << "\"" << poiName << "\",";
      stream << "\"address\":" << "\"" << address << "\",";
      stream << "\"lon\":" << "\"" << std::to_string(vItor->GetCenter().x) << "\",";
      stream << "\"lat\":" << "\"" << std::to_string(vItor->GetCenter().y) << "\"";
      stream << "}";
      if(vItor != vLast -1) {
	      stream << ",";
      }
      ++i;
    }
    stream << "]}";
    return stream.str();
  }
};

template<>
class jsoner<std::vector<search::Result>> {
 public:
  static std::string to_json (std::string const& code, std::string const& error,std::string const& name, std::vector<search::Result> const & value) {
    std::vector<search::Result>::const_iterator itor, last = value.end();
    std::stringstream stream;
    stream << "{\"code\":" << "\"" << code <<"\",";  
    stream << "\"error\":" << "\"" << error <<"\",";  
    stream << "\"" << name << "\":[";
    int i = 0;
    for (itor = value.begin(); itor != last; ++itor) {
      stream << "{";
      stream << "\"index\":" << "\"" << i << "\",";
      std::string poiName = itor->GetString();
      std::string address = itor->GetAddress();
      if(poiName==""){
        poiName = "无名";
      }
      if(address==""){
        address = "暂无地址";
      }
      stream << "\"poiName\":" << "\"" << poiName << "\",";
      stream << "\"address\":" << "\"" << address << "\",";
      stream << "\"lon\":" << "\"" << std::to_string(itor->GetCenter().x) << "\",";
      stream << "\"lat\":" << "\"" << std::to_string(itor->GetCenter().y) << "\"";
      stream << "}";
      if(itor != last -1) {
	      stream << ",";
      }
      ++i;
    }
    stream << "]}";
    return stream.str();
  }
 
};

template<>
class jsoner<osm::MapObject> {
 public:
  static std::string to_json (std::string const& code, std::string const& error,std::string const& name, osm::MapObject const & mapObject) {
    std::stringstream stream;
    stream << "{\"code\":" << "\"" << code <<"\",";  
    stream << "\"error\":" << "\"" << error <<"\",";  
    stream << "\"" << name << "\":{";

    std::string poiName = mapObject.GetDefaultName();
    if(poiName==""){
      poiName = "无名";
    }
    stream << "\"poiName\":" << "\"" << poiName << "\",";
    stream << "\"lon\":" << "\"" << std::to_string(mapObject.GetLatLon().m_lon) << "\",";
    stream << "\"lat\":" << "\"" << std::to_string(mapObject.GetLatLon().m_lat) << "\"";

    stream << "}}";
    return stream.str();
  }
};

template<typename T>
class jsoner<std::vector<boost::shared_ptr<T> > > {
 public:
  static std::string to_json (std::string const& code, std::string const& error,std::string const& name, std::vector<boost::shared_ptr<T> > const & value) {
    typename std::vector<boost::shared_ptr<T> >::const_iterator itor, last = value.end();
    std::stringstream stream;
    stream << "{\"code\":" << "\"" << code <<"\",";  
    stream << "\"error\":" << "\"" << error <<"\",";  
    stream << "\"" << name << "\":[";
    int i = 0;
    for (itor = value.begin(); itor != last; ++itor) {
      stream << "{";
      stream << "\"index\":" << "\"" << i << "\",";
      stream << "\"value\":" << (*itor)->to_json();
      stream << "}";
      if(itor != last -1) {
	stream << ",";
      }
      ++i;
    }
    stream << "]}";
    return stream.str();
  }
 
};
 
 
#endif
 