#pragma once

#include "IHyprWindowDecoration.hpp"
#include "src/render/Texture.hpp"
#include <deque>

class CHyprGroupBarDecoration : public IHyprWindowDecoration {
  public:
    CHyprGroupBarDecoration(CWindow*);
    virtual ~CHyprGroupBarDecoration();

    virtual SWindowDecorationExtents getWindowDecorationExtents();

    virtual void                     draw(CMonitor*, float a, const Vector2D& offset);

    virtual eDecorationType          getDecorationType();

    virtual void                     updateWindow(CWindow*);

    virtual void                     damageEntire();

  private:
    SWindowDecorationExtents m_seExtents;

    CWindow*                 m_pWindow = nullptr;
    CTexture                 m_tTexture;

    Vector2D                 m_vLastWindowPos;
    Vector2D                 m_vLastWindowSize;

    std::deque<CWindow*>     m_dwGroupMembers;
};