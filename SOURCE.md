

radar_pi - Source design
========================

Terminology
-----------

| Name  | Use |
| Spoke | A single 'beam' of radar at a particular angle |

Design
------

The plugin has been refactored in v2.0 to use new abstractions.

There are three main parts to the plugin:

1. Receiving data from the radar and storing it in internal structures.
2. Rendering the various radar displays and overlay.
3. Sending commands to the radar.

The refactoring had the following design principles:

- Allow full use of the dual radar in a 4G.
- Concentrate knowledge of the actual radar in as little code as possible.
- Minimize computations and OpenGL overhead.

The design of the code is such that only the classes named br24... are really Navico specific. The RadarDraw, RadarDrawVertex, RadarDrawShader and GuardZone should be quite portable to other radars. RadarInfo will need some massaging.

Drawing: Vertex or Shader
-------------------------
There are two drawing implementations: Vertex and Shader.

The Vertex code computes as few quadliterals as possible and stores those in a vertex buffer per spoke, and precalculates all trigonomic floating point operations. The vertex lists are generated in the receive thread. This way the amount of data sent to the GPU is minimized, but it does have to be sent on every drawing cycle.

The shader is more brute force, and stores all spoke bytes in a simple array, but it uses the GPU to do the transformation from a linear space to an angular space. This means it does a lot of floating point arc-tangens (atan) operations, but this is quick in modern GPUs. Which one to use is dependent on the relative speed of the CPU and GPU. Old x86 systems with slow GPUs should use the Vertex code. New ARM systems with a fast GPU (but a relatively slow CPU) should use the Shader code. Modern fast CPUs (like an Intel i7) will happily use either. Note that if you want to really check which is the more efficient you should measure total system power usage, not just CPU usage.

If you want to add an extra drawing method, or experiment with improving one of the two existing ones, I highly recommend that you copy one of the existing ones and add your method as a third option in `RadarDraw.cpp`.

1. Receive process
------------------
The flow of data from the radar to the internal data structures is as follows:
```
                  +---------------------------+
    Ethernet ---> | br24Receive::ProcessFrame |
                  +---------------------------+
                               |
                               V
                +-------------------------------+
                | RadarInfo::ProcessRadarSpoke  |
                | RadarInfo::ProcessRadarPacket |
                +-------------------------------+
                               |
                               +---------------------------------------------+
                               |                                             |
                               V                                             V
                  +---------------------------+                 +-------------------------+
                  | RadarDraw interface       |                 | GuardZone::ProcessSpoke |
                  +-------------------------- +                 +-------------------------+
                          /          \                                       |
                         /            \                                      |
               /-------------\    /-------------\             /-----------------------------\
               | RadarVertex |    | RadarShader |             | GuardZone.bogeyCount[spoke] |
               \-------------/    \-------------/             \-----------------------------/
```

The above data runs in a separate thread per radar. You'd think that most people would have only one radar, but a 4G radar behaves electrically as two separate radars. In other words, for 4G radars this does run twice for a single radome.

Note that the radar data is stored up to three times for up to three separate displays: the radar window for radar A, the radar window for radar B and the overlay over the chart. In other words: there are up to three RadarDraw objects.

2. Render process
-----------------
The other thread that runs is the main wxWidgets loop running in the only thread allowed to make wxWidget display calls. This main loop calls the OpenCPN update screen code which in turn calls `plugin->RenderGLOverlay`.

The wxWidget code also calls the `RadarCanvas::Render` method which updates the OpenGL canvas so we see the radar data.

3. Locking
----------

The data stored by the radar receive threads must be displayed by the rendering code, but since this resides in different threads this must be guarded against one thread modifying variables that another thread is reading. This is done using `mutex` objects in the `RadarDraw` implementations (`RadarVertex` and `RadarShader`).

