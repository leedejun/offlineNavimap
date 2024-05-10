#include "drape_frontend/drape_api.hpp"
#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/message_subclasses.hpp"

namespace df
{
    void DrapeApi::SetDrapeEngine(ref_ptr<DrapeEngine> engine)
    {
      m_engine.Set(engine);
    }

// added by baixiaojun
    DrapeApiLineData* DrapeApi::GetData(std::string const & id)
    {
      auto it = m_lines.find("id");
      if( it != m_lines.end() )
        return &it->second;
      return nullptr;
    }

    void DrapeApi::AddLine(std::string const & id, DrapeApiLineData const & data)
    {
      DrapeEngineLockGuard lock(m_engine);
      if (!lock)
        return;

      auto & threadCommutator = lock.Get()->m_threadCommutator;
      auto const it = m_lines.find(id);
      if (it != m_lines.end())
      {
        threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                      make_unique_dp<DrapeApiRemoveMessage>(id),
                                      MessagePriority::Normal);
//    this->Invalidate();
//    return;
      }

      m_lines[id] = data;

      TLines lines;
      lines.insert(std::make_pair(id, data));
      threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                    make_unique_dp<DrapeApiAddLinesMessage>(lines),
                                    MessagePriority::Normal);
    }

    void DrapeApi::RemoveLine(std::string const & id)
    {
      DrapeEngineLockGuard lock(m_engine);
      if (!lock)
        return;

      auto & threadCommutator = lock.Get()->m_threadCommutator;
      m_lines.erase(id);
      threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                    make_unique_dp<DrapeApiRemoveMessage>(id),
                                    MessagePriority::Normal);
    }

    void DrapeApi::AddPolygon(std::string const & id, DrapeApiPolygonData const & data)
    {
      DrapeEngineLockGuard lock(m_engine);
      if (!lock)
        return;

      auto & threadCommutator = lock.Get()->m_threadCommutator;
      auto const it = m_polygons.find(id);
      if (it != m_polygons.end())
      {
        threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                      make_unique_dp<DrapeApiRemoveMessage>(id),
                                      MessagePriority::Normal);
      }

      m_polygons[id] = data;

      TPolygons polygons;
      polygons.insert(std::make_pair(id, data));
      threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                    make_unique_dp<DrapeApiAddPolygonsMessage>(polygons),
                                    MessagePriority::Normal);

    }

    void DrapeApi::RemovePolygon(std::string const & id)
    {
      DrapeEngineLockGuard lock(m_engine);
      if (!lock)
        return;

      auto & threadCommutator = lock.Get()->m_threadCommutator;
      m_polygons.erase(id);
      threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                    make_unique_dp<DrapeApiRemoveMessage>(id),
                                    MessagePriority::Normal);
    }

    void DrapeApi::AddCustomMark(std::string const & id, DrapeApiCustomMarkData const & data)
    {
      DrapeEngineLockGuard lock(m_engine);
      if (!lock)
        return;

      auto & threadCommutator = lock.Get()->m_threadCommutator;
      auto const it = m_marks.find(id);
      if (it != m_marks.end())
      {
        threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                      make_unique_dp<DrapeApiRemoveMessage>(id),
                                      MessagePriority::Normal);
      }

      m_marks[id] = data;

      TCustomMarks marks;
      marks.insert(std::make_pair(id, data));
      threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                    make_unique_dp<DrapeApiAddCustomMarksMessage>(marks),
                                    MessagePriority::Normal);
    }

    void DrapeApi::RemoveCustomMark(std::string const & id)
    {
      DrapeEngineLockGuard lock(m_engine);
      if (!lock)
        return;

      auto & threadCommutator = lock.Get()->m_threadCommutator;
      m_marks.erase(id);
      threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                    make_unique_dp<DrapeApiRemoveMessage>(id),
                                    MessagePriority::Normal);
    }

    void DrapeApi::RemoveCustomMarkBatch(std::vector<std::string> const & idList)
    {
        DrapeEngineLockGuard lock(m_engine);
        if (!lock)
            return;

        auto & threadCommutator = lock.Get()->m_threadCommutator;
        for (int i = 0; i < idList.size(); ++i)
        {
            m_marks.erase(idList.at(i));
        }

        threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                      make_unique_dp<DrapeApiRemoveBatchMessage>(idList),
                                      MessagePriority::Normal);
    }

    void DrapeApi::Clear()
    {
      DrapeEngineLockGuard lock(m_engine);
      if (!lock)
        return;

      auto & threadCommutator = lock.Get()->m_threadCommutator;
      m_lines.clear();
      threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                    make_unique_dp<DrapeApiRemoveMessage>("", true /* remove all */),
                                    MessagePriority::Normal);
    }

    void DrapeApi::Invalidate()
    {
      DrapeEngineLockGuard lock(m_engine);
      if (!lock)
        return;

      auto & threadCommutator = lock.Get()->m_threadCommutator;
      threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                    make_unique_dp<DrapeApiRemoveMessage>("", true /* remove all */),
                                    MessagePriority::Normal);

      threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                    make_unique_dp<DrapeApiAddLinesMessage>(m_lines),
                                    MessagePriority::Normal);

      threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                      make_unique_dp<DrapeApiAddPolygonsMessage>(m_polygons),
                                      MessagePriority::Normal);
      threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                      make_unique_dp<DrapeApiAddCustomMarksMessage>(m_marks),
                                      MessagePriority::Normal);
    }
}  // namespace df
