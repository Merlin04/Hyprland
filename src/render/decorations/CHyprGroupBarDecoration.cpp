#include "CHyprGroupBarDecoration.hpp"
#include "../../Compositor.hpp"

CHyprGroupBarDecoration::CHyprGroupBarDecoration(CWindow* pWindow) {
    m_pWindow = pWindow;
}

CHyprGroupBarDecoration::~CHyprGroupBarDecoration() {}

SWindowDecorationExtents CHyprGroupBarDecoration::getWindowDecorationExtents() {
    return m_seExtents;
}

eDecorationType CHyprGroupBarDecoration::getDecorationType() {
    return DECORATION_GROUPBAR;
}

void CHyprGroupBarDecoration::updateWindow(CWindow* pWindow) {
    damageEntire();

    const auto PWORKSPACE = g_pCompositor->getWorkspaceByID(pWindow->m_iWorkspaceID);

    const auto WORKSPACEOFFSET = PWORKSPACE && !pWindow->m_bPinned ? PWORKSPACE->m_vRenderOffset.vec() : Vector2D();

    if (pWindow->m_vRealPosition.vec() + WORKSPACEOFFSET != m_vLastWindowPos || pWindow->m_vRealSize.vec() != m_vLastWindowSize) {
        // we draw 3px above the window's border with 3px
        // actually we are going to make it 48px for testing
        const auto PBORDERSIZE = &g_pConfigManager->getConfigValuePtr("general:border_size")->intValue;

        m_seExtents.topLeft     = Vector2D(0, *PBORDERSIZE + 3 + 3);
//        m_seExtents.topLeft = Vector2D(0, *PBORDERSIZE + 3 + 48);
        m_seExtents.bottomRight = Vector2D();

        m_vLastWindowPos  = pWindow->m_vRealPosition.vec() + WORKSPACEOFFSET;
        m_vLastWindowSize = pWindow->m_vRealSize.vec();
    }

    // let's check if the window group is different.

    if (g_pLayoutManager->getCurrentLayout()->getLayoutName() != "dwindle") {
        // ????
        m_pWindow->m_vDecosToRemove.push_back(this);
        return;
    }

    // get the group info
    SLayoutMessageHeader header;
    header.pWindow = m_pWindow;

    m_dwGroupMembers = std::any_cast<std::deque<CWindow*>>(g_pLayoutManager->getCurrentLayout()->layoutMessage(header, "groupinfo"));

    damageEntire();

    if (m_dwGroupMembers.size() == 0) {
        // remove
        m_pWindow->m_vDecosToRemove.push_back(this);
        return;
    }
}

void CHyprGroupBarDecoration::damageEntire() {
    wlr_box dm = {m_vLastWindowPos.x - m_seExtents.topLeft.x, m_vLastWindowPos.y - m_seExtents.topLeft.y, m_vLastWindowSize.x + m_seExtents.topLeft.x + m_seExtents.bottomRight.x,
                  m_seExtents.topLeft.y};
    g_pHyprRenderer->damageBox(&dm);
}

void CHyprGroupBarDecoration::draw(CMonitor* pMonitor, float a, const Vector2D& offset) {
    // get how many bars we will draw
    int barsToDraw = m_dwGroupMembers.size();

    if (barsToDraw < 1 || m_pWindow->isHidden() || !g_pCompositor->windowValidMapped(m_pWindow))
        return;

    if (!m_pWindow->m_sSpecialRenderData.decorate)
        return;

    const int PAD = 2; //2px

    const int BARW = (m_vLastWindowSize.x - PAD * (barsToDraw - 1)) / barsToDraw;

    int       xoff = 0;

    for (int i = 0; i < barsToDraw; ++i) {
        wlr_box rect = {m_vLastWindowPos.x + xoff - pMonitor->vecPosition.x + offset.x, m_vLastWindowPos.y - m_seExtents.topLeft.y - pMonitor->vecPosition.y + offset.y, BARW, 3};

        if (rect.width <= 0 || rect.height <= 0)
            break;

        scaleBox(&rect, pMonitor->scale);

        static auto* const PGROUPCOLACTIVE   = &g_pConfigManager->getConfigValuePtr("dwindle:col.group_border_active")->data;
        static auto* const PGROUPCOLINACTIVE = &g_pConfigManager->getConfigValuePtr("dwindle:col.group_border")->data;

        CColor             color = m_dwGroupMembers[i] == g_pCompositor->m_pLastWindow ? ((CGradientValueData*)PGROUPCOLACTIVE->get())->m_vColors[0] :
                                                                                         ((CGradientValueData*)PGROUPCOLINACTIVE->get())->m_vColors[0];
        color.a *= a;
        g_pHyprOpenGL->renderRect(&rect, color);

        xoff += PAD + BARW;
    }

    // draw text as test
    // attempt at renderText method wasn't very well done so I will just put it here

    const auto CAIROSURFACE = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, m_vLastWindowSize.x, m_vLastWindowSize.y);
    const auto CAIROCTX     = cairo_create(CAIROSURFACE);

    cairo_save(CAIROCTX);
    cairo_set_operator(CAIROCTX, CAIRO_OPERATOR_CLEAR);
    cairo_paint(CAIROCTX);
    cairo_restore(CAIROCTX);

    cairo_set_source_rgba(CAIROCTX, 1, 1, 1, 1);
    cairo_select_font_face(CAIROCTX, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(CAIROCTX, 12);
    cairo_move_to(CAIROCTX, 0, 0);
    cairo_show_text(CAIROCTX, "te=oienienonyo=s=othrs=trshtarhthrsietinvnxecivonx   nveixontisrnaietnoritneisnieneivn   cnvist");

    // draw a blue rectangle on the entire surface for debugging
    cairo_set_source_rgba(CAIROCTX, 0, 0, 1, 0.5);
    cairo_rectangle(CAIROCTX, 0, 0, m_vLastWindowSize.x, m_vLastWindowSize.y);
    cairo_fill(CAIROCTX);

    cairo_surface_flush(CAIROSURFACE);
    cairo_destroy(CAIROCTX);

    const auto CAIRODATA = cairo_image_surface_get_data(CAIROSURFACE);

    wlr_box rect = {m_vLastWindowPos.x - pMonitor->vecPosition.x + offset.x, m_vLastWindowPos.y - m_seExtents.topLeft.y - pMonitor->vecPosition.y + offset.y, m_vLastWindowSize.x, m_vLastWindowSize.y};
    scaleBox(&rect, pMonitor->scale);

    // debug: print out the box
    printf("rect: %d %d %d %d", rect.x, rect.y, rect.width, rect.height);

    // make texture

    m_tTexture.allocate();
    glBindTexture(GL_TEXTURE_2D, m_tTexture.m_iTexID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

#ifndef GLES2
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
#endif

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rect.width, rect.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, CAIRODATA);

    cairo_surface_destroy(CAIROSURFACE);

    // draw texture

    g_pHyprOpenGL->renderTexture(m_tTexture, &rect, a, 0);

    // clean up

    m_tTexture.destroyTexture();

    // end of test


}