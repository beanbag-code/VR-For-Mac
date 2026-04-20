# VR-For-Mac
Building a custom VR headset that tracks head movement for use in X-Plane 12 on Mac.

Isn't it a shame that X-Plane 12's VR does not work on Mac? 
Well, it's time that ends...The plan is a custom system to add pseudo VR capabilities to a Mac.
This means that it is not true dual-camera VR; rather, by rendering a wider field of view, a custom app will split the screen and apply lens corrections, allowing the screen to be sent to an LCD module in a VR headset.

On the VR headset there will be an IMU to record head movement.
The plan is also to run an AprilTag detector on the computer seeing through the webcam. 
This detector would correct IMU drift for long flight sessions.

The headset is designed around a Waveshare 5.5-inch 2K LCD screen. It features two 50 mm convex lenses.

It is also designed around my head. This means that the IPD (interpupillary distance) is set custom to me.
I also plan on creating a custom face mask printed with TPU. 
This would be an exact photoscan of my face.

It's an ambitious project, but it's rewarding.As I control all the software, I can optimize it as much as possible. Given the whole headset is just an HDMI display for all the computer is concerned, the lag from machine to headset should be very low.

I look foreward to seeing the finished project.


Disclaimer
The X-Plane to split screen window app is almost entirely coded by AI.
I do not have the skills or experince with shaders/Swift to code it.
