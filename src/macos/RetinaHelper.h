//
// https://forums.wxwidgets.org/viewtopic.php?t=36684
//
// file RetinaHelper.h

#ifndef __WXOSX__
#define RETINA_SCALE(n) (n)
#else
#define RETINA_SCALE(n) ((int) ((float) (n) * m_retinaHelper->getBackingScaleFactor()))
#endif

#ifndef RetinaHelper_h
#define RetinaHelper_h

class wxWindow;

class RetinaHelper
{
public:
   RetinaHelper(wxWindow* window);
   ~RetinaHelper();

   void setViewWantsBestResolutionOpenGLSurface(bool value);
   bool getViewWantsBestResolutionOpenGLSurface();
   float getBackingScaleFactor();

private:
   wxWindow* _window;
   void* _self; // pointer to obj-c++ implementation
};
#endif // RetinaHelper_h

