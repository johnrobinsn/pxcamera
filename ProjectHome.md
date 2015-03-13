pxCamera is a simple video camera capture class that layers on top of pxCore, my portable opensource framebuffer library.  The design of pxCamera make it well suited to do a variety of fun and interesting things with captured video frames like computer vision or image processing applications.

The design of the pxCamera API followed a few principles as follows:

  * Be Simple – Allows for enumeration of available capture devices and the ability to initiate capturing frames from a given camera into a pxBuffer(framebuffer).
  * No UI Policy – Simple capture into a framebuffer.  No UI is assumed.
  * Support multiple capture sources simultaneously. [I’ve tested with two webcams] pxCamera should support any DirectShow compatible video source.
  * Portable API – The API is portable even though the implementation is currently only for Windows.

Related Projects:

[pxCore](http://code.google.com/p/pxcore) - Portable Framebuffer Library

