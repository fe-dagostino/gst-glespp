gst-gles++
==========

This library is a video sink plug-in intended to be used with GStreamer pipeline.
gst-gles++ has been build on the above of many other libraries and many others will be introduced, but each library used until now:
 - is open source and under a license that allow commercial use
 - is portable through OSs and at least linux and windows are supported
 - supports Opengl ES as well OpenGL
 - supports x86,x86_64 and ARM architectures 

Here a list with all dependecies and their use

 - **FEDLibrary** used to keep code portable between linux and windows OSs
 - **GLFW**       multi-platform library for creating windows with OpenGL contexts and receiving input and events. 
 - **GLM**        OpenGL Mathematics (GLM)
 - **GStreamer**  gStreamer SDK, used to implements video sink classes
 - **libgles++**  had declaration for GLTexture used to exchange video frames with application level.
 
ToDo

