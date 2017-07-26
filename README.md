# Application Of Transmission Line Matrix Theory on Modern Day Graphics Processing Unit Hardware

### About

This was my final year project as part of my Software Engineering Degree.

It was an extension on existing work, the full details of which can be found in the dissertation.pdf file in the docs folder (along with the presentation that was done and the proposal documents we had to write).

This project was in fact written in a short period of time towards the start of 2008 to be submitted in May of that year. This somewhat dates the concept of 'modern' (as does the mention of 48 cores in a GPU of the time).

### Details

The project itself is a classic old GPGPU example, using pixel shader and ping pong buffers to perform multipass scatter-gather tasks; this predates compute shaders, at least for the hardware I had at the time.

The project itself is written to use OpenGL with a few custom libs.

- OpenGLWFW is the window lib found else where in these repos
- Maths library was taken from Game Programming Gems
- libFrameEncoder is a tiny custom library to use Lib Theora to write video frames out
- Camera code was taken from another engine project of mine
- GLSL Support is a set of very simple, out dated now, classes to aid with compiling shaders

The project itself simple runs through a list of tests, each one runing the simulation and grabbing video output while timing the amount of time required. This data is then written out to a text file to be later processed in to the results found in the dissertation.

### Notes

Probably won't be able to compile this due to lack of libs.

Required boost for various things.

Released under MIT license - see license text for details.
