
class br24Draw
{

public:
    static br24Draw * make_Draw( int useShader );

    virtual bool Init( br24radar_pi * ppi, int colorOption ) = 0;
    virtual void DrawRadarImage( wxPoint center, double heading, double scale, bool overlay ) = 0;
    virtual void ClearSpoke( int angle ) = 0;
    virtual void SetBlob( int angle_begin, int angle_end, int r_begin, int r_end, GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha ) = 0;

    virtual ~br24Draw() = 0;
};

