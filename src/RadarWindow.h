#ifndef _RadarCanvas_
#define _RadarCanvas_

#include "wx/wx.h"
#include "wx/glcanvas.h"

class RadarWindow : public wxGLCanvas
{
    //DECLARE_CLASS(RadarWindow)
    DECLARE_EVENT_TABLE()

    wxGLContext*        m_context;

public:
    RadarWindow(br24radar_pi *ppi, wxFrame* parent, int* args);
    virtual ~RadarWindow();

    void resized(wxSizeEvent& evt);

    int getWidth();
    int getHeight();

    void render(wxPaintEvent& evt);
    void close(wxCloseEvent& evt);

    // events
    void mouseMoved(wxMouseEvent& event);
    void mouseDown(wxMouseEvent& event);
    void mouseWheelMoved(wxMouseEvent& event);
    void mouseReleased(wxMouseEvent& event);
    void rightClick(wxMouseEvent& event);
    void mouseLeftWindow(wxMouseEvent& event);
    void keyPressed(wxKeyEvent& event);
    void keyReleased(wxKeyEvent& event);

private:
    void prepare2DViewport(int topleft_x, int topleft_y, int bottomright_x, int bottomright_y);
    br24radar_pi *pPlugIn;
};
#endif
