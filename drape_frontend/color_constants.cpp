#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/apply_feature_functors.hpp"

#include "platform/platform.hpp"

#include "indexer/drawing_rules.hpp"
#include "indexer/map_style_reader.hpp"

#include "coding/reader.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include "3party/jansson/myjansson.hpp"

#include <fstream>

using namespace std;

namespace
{
string const kTransitColorFileName = "transit_colors.txt";

// added by baixiaojun, 2022.04.17
    template<class TT>
    static uint32_t toIntFromHexString(const TT &t) {

      uint32_t x;
      std::stringstream ss;
      ss << std::hex << t;
      ss >> x;
      return x;

    }


    static dp::Color stringToDpColor(std::string colorString) {
      if (colorString[0] == '#') {
        colorString.erase(0, 1);
      }
      else{
        return dp::Color();
      }

      std::string stringR = colorString.substr(0, 2);
      std::string stringG = colorString.substr(2, 2);
      std::string stringB = colorString.substr(4, 2);
      std::string stringA = colorString.substr(6, 2);

      uint32_t r = toIntFromHexString(stringR);
      uint32_t g = toIntFromHexString(stringG);
      uint32_t b = toIntFromHexString(stringB);
      uint32_t a = 255;//toIntFromHexString(stringA);
      if (stringA.length() > 0) {
        a = toIntFromHexString(stringA);
      }
      return dp::Color(r, g, b, a);
    }
// end by baixiaojun

class TransitColorsHolder
{
public:
  dp::Color GetColor(string const & name) const
  {
    auto const style = GetStyleReader().GetCurrentStyle();
    auto const isDarkStyle = style == MapStyle::MapStyleDark || style == MapStyle::MapStyleVehicleDark;
    auto const & colors = isDarkStyle ? m_nightColors : m_clearColors;
    auto const it = colors.find(name);
    if (it == colors.cend())
      return dp::Color();
    return it->second;
  }

  void Load()
  {
    string data;
    try
    {
      ReaderPtr<Reader>(GetPlatform().GetReader(kTransitColorFileName)).ReadAsString(data);
    }
    catch (RootException const & ex)
    {
      LOG(LWARNING, ("Loading transit colors failed:", ex.what()));
      return;
    }

    try
    {
      base::Json root(data);

      if (root.get() == nullptr)
        return;

      auto colors = json_object_get(root.get(), "colors");
      if (colors == nullptr)
        return;

      const char * name = nullptr;
      json_t * colorInfo = nullptr;
      json_object_foreach(colors, name, colorInfo)
      {
        ASSERT(name != nullptr, ());
        ASSERT(colorInfo != nullptr, ());

        string strValue;
        FromJSONObject(colorInfo, "clear", strValue);
        m_clearColors[df::GetTransitColorName(name)] = ParseColor(strValue);
        FromJSONObject(colorInfo, "night", strValue);
        m_nightColors[df::GetTransitColorName(name)] = ParseColor(strValue);
        FromJSONObject(colorInfo, "text", strValue);
        m_clearColors[df::GetTransitTextColorName(name)] = ParseColor(strValue);
        m_nightColors[df::GetTransitTextColorName(name)] = ParseColor(strValue);
      }
    }
    catch (base::Json::Exception const & e)
    {
      LOG(LWARNING, ("Reading transit colors failed:", e.Msg()));
    }
  }

  map<string, dp::Color> const & GetClearColors() const { return m_clearColors; }

private:
  dp::Color ParseColor(string const & colorStr)
  {
    unsigned int color;
    if (strings::to_uint(colorStr, color, 16))
      return df::ToDrapeColor(static_cast<uint32_t>(color));
    LOG(LWARNING, ("Color parsing failed:", colorStr));
    return dp::Color();
  }

  map<string, dp::Color> m_clearColors;
  map<string, dp::Color> m_nightColors;
};

TransitColorsHolder & TransitColors()
{
  static TransitColorsHolder h;
  return h;
}
}  // namespace

namespace df
{
ColorConstant GetTransitColorName(ColorConstant const & localName)
{
  return kTransitColorPrefix + kTransitLinePrefix + localName;
}

ColorConstant GetTransitTextColorName(ColorConstant const & localName)
{
  return kTransitColorPrefix + kTransitTextPrefix + localName;
}

bool IsTransitColor(ColorConstant const & constant)
{
  return strings::StartsWith(constant, kTransitColorPrefix);
}

dp::Color GetColorConstant(ColorConstant const & constant)
{
  if (IsTransitColor(constant))
    return TransitColors().GetColor(constant);
  uint32_t const color = drule::rules().GetColor(constant);
  // modified by baixiaojun, 2022.4.17
  //return ToDrapeColor(color);
  if( color != 0 ){
    return ToDrapeColor(color);
  }
  else{
    return stringToDpColor(constant);
  }
  // end by baixiaojun
}

map<string, dp::Color> const & GetTransitClearColors() { return TransitColors().GetClearColors(); }

void LoadTransitColors()
{
  TransitColors().Load();
}
}  // namespace df
