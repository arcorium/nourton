#pragma once

#include <util/enum.h>

namespace ar
{
  enum class PageState : u8
  {
    Login,
    Register,
    Dashboard,
    SendFile,
    End
  };

  class State
  {
  public:
    State(PageState state) noexcept
      : m_state{state}, m_show_page{}
    {
      m_show_page[to_underlying(state)] = true;
    }

    void active(PageState state) noexcept
    {
      m_show_page[to_underlying(m_state)] = false;
      m_show_page[to_underlying(state)] = true;
      m_state = state;
    }

    PageState state() const noexcept
    {
      return m_state;
    }

    bool* page_show(PageState state) noexcept
    {
      return &m_show_page[to_underlying(state)];
    }

  private:
    PageState m_state;
    bool m_show_page[static_cast<int>(PageState::SendFile) + 1];
  };
}
