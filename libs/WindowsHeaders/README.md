Windows OpenGL headers.

The gl.h and glu.h are copied verbatim from the windows SDK. These are
around 1995, and Microsoft does not update these files. Thus, to compile
on windows it's thus necessary to include also glext.h which contains
an updated API.

Other files are in a vanilla, Khronos group state. 
