


class br24Vertex : public br24Draw
{
public:
    br24Vertex()
    {
      pPlugin = 0;
      start_line = LINES_PER_ROTATION;
      end_line = 0;
    }

    bool Init( br24radar_pi * ppi, int colorOption );
    void DrawRadarImage( wxPoint center, double heading, double scale, bool overlay );
    void ClearSpoke( int angle );
    void SetBlob( int angle_begin, int angle_end, int r_begin, int r_end, GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha );

    ~br24Vertex()
    {
    }

private:
    br24radar_pi  * pPlugin;

    static const int VERTEX_PER_TRIANGLE = 3;
    static const int VERTEX_PER_QUAD = 2 * VERTEX_PER_TRIANGLE;
    static const int VERTEX_MAX = 100 * VERTEX_PER_QUAD; // Assume picture is no more complicated than this

    struct vertex_point {
        GLfloat x;
        GLfloat y;
        GLubyte red;
        GLubyte green;
        GLubyte blue;
        GLubyte alpha;
    };

    struct vertex_spoke {
        vertex_point    points[VERTEX_MAX];
        int             n;
    };

    vertex_spoke    spokes[LINES_PER_ROTATION];

    GLfloat         polar_to_cart_x[LINES_PER_ROTATION + 1][RETURNS_PER_LINE + 1];
    GLfloat         polar_to_cart_y[LINES_PER_ROTATION + 1][RETURNS_PER_LINE + 1];

    int             start_line;
    int             end_line;
};

