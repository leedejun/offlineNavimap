#pragma once

#include "drape_frontend/drape_engine_safe_ptr.hpp"

#include "drape/color.hpp"
#include "drape/pointers.hpp"

#include "geometry/point2d.hpp"

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace df
{
    struct DrapeApiLineData
    {
        DrapeApiLineData() = default;

        DrapeApiLineData(std::vector<m2::PointD> const & points,
                         dp::Color const & color)
                : m_points(points)
                , m_color(color)
        {}

        DrapeApiLineData & ShowPoints(bool markPoints)
        {
          m_showPoints = true;
          m_markPoints = markPoints;
          return *this;
        }

        DrapeApiLineData & Width(float width)
        {
          m_width = width;
          return *this;
        }

        DrapeApiLineData & ShowId()
        {
          m_showId = true;
          return *this;
        }

        std::vector<m2::PointD> m_points;
        float m_width = 1.0f;
        dp::Color m_color;

        bool m_showPoints = false;
        bool m_markPoints = false;
        bool m_showId = false;
        bool m_showLine = true;
    };

    struct DrapeApiPolygonData
    {
        DrapeApiPolygonData() = default;

        DrapeApiPolygonData(std::vector<m2::PointD> const & points,
                            dp::Color const & color,
                            dp::Color const & outlineColor,
                            float outlineWidth)
                : m_points(points)
                , m_color(color)
                , m_outlineColor(outlineColor)
                , m_outlineWidth(outlineWidth)
        {}

        std::vector<m2::PointD> m_points;
        dp::Color m_color;
        dp::Color m_outlineColor;
        float m_outlineWidth = 0.0f;
    };

    struct DrapeApiCustomMarkData
    {
        DrapeApiCustomMarkData() = default;
        DrapeApiCustomMarkData(std::string const & id,
                               m2::PointD const & center,
                               m2::PointD const & displaySize,
                               std::string const & text,
                               std::string const & path,
                               dp::Color const & color,
                               int fontSize)
                :
                m_id(id)
                , m_center(center)
                , m_displaySize(displaySize)
                , m_text(text)
                , m_path(path)
                , m_color(color)
                , m_fontSize(fontSize)
        {
        }
        std::string m_id;
        m2::PointD m_center;
        m2::PointD m_displaySize; //mark 显示的像素大小
        std::string m_text;
        std::string m_path;
        dp::Color m_color;
        int m_fontSize = 24;
    };


    class DrapeApi
    {
    public:
        using TLines = std::unordered_map<std::string, DrapeApiLineData>;
        using TPolygons = std::unordered_map<std::string, DrapeApiPolygonData>;
        using TCustomMarks = std::unordered_map<std::string, DrapeApiCustomMarkData>;

        DrapeApi() = default;

        void SetDrapeEngine(ref_ptr<DrapeEngine> engine);

        void AddLine(std::string const & id, DrapeApiLineData const & data);
        void RemoveLine(std::string const & id);

        void AddPolygon(std::string const & id, DrapeApiPolygonData const & data);
        void RemovePolygon(std::string const & id);

        void AddCustomMark(std::string const & id, DrapeApiCustomMarkData const & data);
        void RemoveCustomMark(std::string const & id);
        void RemoveCustomMarkBatch(std::vector<std::string> const & idList);

        void Clear();
        void Invalidate();

        // added by baixiaojun
        DrapeApiLineData* GetData(std::string const & id);
    private:
        DrapeEngineSafePtr m_engine;
        TLines m_lines;
        TPolygons m_polygons;
        TCustomMarks m_marks;
    };
}  // namespace df
