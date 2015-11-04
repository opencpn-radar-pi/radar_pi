
#define SHADER_COLOR_CHANNELS (4) // RGB + Alpha

class br24Shader
{
    br24radar_pi  * pPlugin;
    unsigned char * data;
    int             start_line;
    int             end_line;

public:
    br24Shader(br24radar_pi *ppi)
    {
      colorOption = 0;
      pPlugin = ppi;
      data = (unsigned char *) calloc(SHADER_COLOR_CHANNELS, LINES_PER_ROTATION * RETURNS_PER_LINE);
    };

    bool Init( int colorOption );
    void DrawRadarImage();
    void ClearSpoke( int angle );
    void SetBlob( int angle, int r_begin, int r_end, GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha );

    ~br24Shader( void )
    {
        if (program) {
            DeleteProgram(program);
        }
        if (vertex) {
            DeleteShader(vertex);
        }
        if (fragment) {
            DeleteShader(fragment);
        }
        if (data) {
            free(data);
        }
    };

private:
    int colorOption;
    int lastColorOption;

    GLuint texture;
    GLuint fragment;
    GLuint vertex;
    GLuint program;
};

