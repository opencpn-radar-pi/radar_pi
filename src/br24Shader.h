
#define SHADER_COLOR_CHANNELS (4) // RGB + 2lpha

class br24Shader : public br24Draw
{
public:
    br24Shader()
    {
        pPlugin = 0;
        start_line = LINES_PER_ROTATION;
        end_line = 0;
        texture = 0;
        fragment = 0;
        vertex = 0;
        program = 0;
    }

    bool Init( br24radar_pi * ppi, int colorOption );
    void DrawRadarImage( wxPoint center, double heading, double scale, bool overlay );
    void ClearSpoke( int angle );
    void SetBlob( int angle_begin, int angle_end, int r_begin, int r_end, GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha );

    ~br24Shader()
    {
    }

private:
    br24radar_pi  * pPlugin;
    unsigned char   data[SHADER_COLOR_CHANNELS * LINES_PER_ROTATION * RETURNS_PER_LINE];
    int             start_line;
    int             end_line;

    int             colorOption;

    GLuint          texture;
    GLuint          fragment;
    GLuint          vertex;
    GLuint          program;
};

