# VR-For-Mac

Building a custom VR headset that tracks head movement for use in X-Plane 12 on Mac.

Isn't it a shame that X-Plane 12's VR does not work on Mac? 

Well, it's time that ends. The plan is a custom system to add pseudo-VR capabilities to a Mac.

This means that it is not true dual-camera VR; rather, by rendering a wider field of view, a custom app will split the screen and apply lens corrections, allowing the screen to be sent to an LCD module in a VR headset.

On the VR headset there will be an IMU to record head movement.

The plan is also to run an AprilTag detector on the computer seeing through the webcam.
This detector would correct IMU drift for long flight sessions.
NOTE: This got axed.The code is still there, just got commented out.

The headset is designed around a Waveshare 5.5-inch 2K LCD screen. It features two 50 mm convex lenses.
Link to that https://www.amazon.com/waveshare-5-5inch-Capacitive-LCD-Compatible/dp/B0BG2CLS6Q/ref=sr_1_4?crid=1PW1JXOJSZ90S&dib=eyJ2IjoiMSJ9.fIucvloDRY1Vt3gLX4zHJQ7zOmcuBf_8hEvcNTJxhD3_wmu0ot1EdpEb1jB9VPIPNx4d5xSGB-5spCoqiOamOcCYUsEycqzmaGUTK2dWCYP8vTKjxHnAZ55bJeZ_7ZBqkReaosO31rBJ-rIGrz6_dRs2G43d3K8AOaeyn4nbOsZv3LUweu9G4QdJIzQzkOdxREgGP064MCHJ2OArxN7sn7C8yVUtudRzPXUy58DCKbA.fnyGcQEGuBn7EJiPyXoOUjB5d5RO1fcJtmZFysQtRiw&dib_tag=se&keywords=waveshare+display+5.5+inch+captive&qid=1782942833&sprefix=waveshare+diapy+5.5+inch+captive%2Caps%2C216&sr=8-4

https://www.amazon.com/Eisco-Labs-Glass-Lenses-Diameter/dp/B01F9KXRX2/ref=sr_1_4?crid=VG2RQQSZL7KE&dib=eyJ2IjoiMSJ9.a3QBGz8O3uE64Cl7RQZW_J2JaZCTR6DjIo_oPqcGdVo0mZQz57vJthu8clt5l3q8fEdgu2bBzY7m23bDltoeBPxcLkyWB6O-EgfO48m8zVP67eM5I-7mQ6OhIxEQoOafHyWXDX20ehej3onBdYBVH3pX_s0UPlKjPO21IvICDBG1yt-xZfHf0grfAhjWRSGmCzIlsj4kpSAwucYBfubsEPmrRX9Oc5uA2szrD39piRg.MJIBnx-w15U4hJ6WBniv1wxA-yOseXuKLM4ZfUdnD-0&dib_tag=se&keywords=50+mm+convex+lens&qid=1782942896&sprefix=50+mm+convex+len%2Caps%2C228&sr=8-4

It is also designed around my head. This means that the IPD (interpupillary distance) is set custom to me.

I also made a custom face mask printed with TPU. 

It's an ambitious project, but it's rewarding. As I control all the software, I can optimize it as much as possible. Given the whole headset is just an HDMI display for all the computer is concerned, the lag from machine to headset should be very low.

I look forward to seeing the finished project.

NOTE:

This is my first time doing anything with GitHub and doing a project of this scale. I apologize for anyone trying to use any of this.

Disclaimer

The X-Plane split-screen window app is almost entirely coded by AI.

I do not have the skills or experience with shaders/Swift to code it.

