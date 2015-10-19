#include <br24radar_pi.h>

#ifdef WIN32
# include <wx/glcanvas.h>
#endif

BEGIN_EVENT_TABLE(RadarWindow, wxGLCanvas)
EVT_SIZE(RadarWindow::resized)
EVT_PAINT(RadarWindow::render)
END_EVENT_TABLE()

// some useful events to use
void RadarWindow::mouseMoved(wxMouseEvent& event) {}
void RadarWindow::mouseDown(wxMouseEvent& event) {}
void RadarWindow::mouseWheelMoved(wxMouseEvent& event) {}
void RadarWindow::mouseReleased(wxMouseEvent& event) {}
void RadarWindow::rightClick(wxMouseEvent& event) {}
void RadarWindow::mouseLeftWindow(wxMouseEvent& event) {}
void RadarWindow::keyPressed(wxKeyEvent& event) {}
void RadarWindow::keyReleased(wxKeyEvent& event) {}

static int attribs[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_DEPTH_SIZE, 16, WX_GL_STENCIL_SIZE, 8, 0 };

RadarWindow::RadarWindow(br24radar_pi * ppi, wxFrame* parent, int* args) :
#if !wxCHECK_VERSION(3,0,0)
    wxGLCanvas( parent, wxID_ANY, wxDefaultPosition, wxSize( 256, 256 ),
                wxFULL_REPAINT_ON_RESIZE | wxBG_STYLE_CUSTOM, _T(""), attribs )
#else
    wxGLCanvas( parent, wxID_ANY, attribs, wxDefaultPosition, wxSize( 256, 256 ),
                wxFULL_REPAINT_ON_RESIZE | wxBG_STYLE_CUSTOM, _T("") )
#endif
{
    pPlugIn = ppi;
    m_context = new wxGLContext(this);

    // To avoid flashing on MSW
    SetBackgroundStyle(wxBG_STYLE_CUSTOM);
}

RadarWindow::~RadarWindow()
{
    delete m_context;
}

void RadarWindow::resized(wxSizeEvent& evt)
{
//      wxGLCanvas::OnSize(evt);

    Refresh();
}

/** Inits the OpenGL viewport for drawing in 2D. */
void RadarWindow::prepare2DViewport(int x, int y, int width, int height)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black Background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);   // textures
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    glViewport(x, y, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glScaled(1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

int RadarWindow::getWidth()
{
    return GetSize().x;
}

int RadarWindow::getHeight()
{
    return GetSize().y;
}

void RadarWindow::render( wxPaintEvent& evt )
{
    int w, h;

    if (!IsShown()) {
        return;
    }

    GetClientSize(&w, &h);

    wxLogMessage(wxT("RadarWindow: rendering %d by %d"), w, h);

    wxGLCanvas::SetCurrent(*m_context);
    wxPaintDC(this); // only to be used in paint events. use wxClientDC to paint outside the paint event

    prepare2DViewport(0, 0, w, h);

    double scale_factor = 1.0 / 512.0; // Radar image is in 0..511 range

    glScaled(scale_factor, scale_factor, 1.);

    glRotatef(270.0, 0, 0, 1);
    pPlugIn->RenderGuardZone(wxPoint(0,0), 1.0, 0);
    glRotatef(180.0, 0, 0, 1); // Works for my demo situation but I don't see why...
    pPlugIn->DrawRadarImage();

    glFlush();
    SwapBuffers();
}

