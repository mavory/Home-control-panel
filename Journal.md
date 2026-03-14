
2/2/2026 5:51 PM - The birth of the project
Time spent: 0.5h
I was thinking about how I could improve my smart home and it occurred to me that I could create a box that I could control e.g. lights, Spotify, Netflix,...
So I thought that if I put together a small ESP32-C3 and other things, I could create a portable box that would have a built-in battery!
I continued and started looking for what I had at home... I only found an OLED display and nothing else, so I had to go to the internet, where I found all the things!
![image](https://blueprint.hackclub.com/user-attachments/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6OTcwODksInB1ciI6ImJsb2JfaWQifX0=--2c3f3237540cd753dbcfcd3a435d69cd7d317b5c/image.png)

2/2/2026 5:55 PM - The beginning of 3D modeling
Time spent: 0.7h
When I had it ready, I prepared all the things and started looking for their sizes etc. It went well at first and then it got even better.
Then I went to TinkerCad where I decided to model the case and I started! I immediately created the walls and then made holes for the buttons, OLED display,...
![Snímek obrazovky 2026-02-02 164954](https://blueprint.hackclub.com/user-attachments/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6OTcwOTMsInB1ciI6ImJsb2JfaWQifX0=--eb437c9ee56d4a36c71958325d03e3c6ce79de03/Sn%C3%ADmek%20obrazovky%202026-02-02%20164954.png)

2/2/2026 5:58 PM - Continuation and a minor mistake...
Time spent: 1.1h
Right after finishing the main parts, I started making the holder for the charging module, the hole for the rings, the lid or even the rings themselves (so that I would know where each button was later)!
It went pretty quickly, but I had a small problem... If I wanted to put a battery in there, I made a case that was too small - so I took everything apart again and made smaller walls and enlarged the hole for the components.
![Snímek obrazovky 2026-02-02 172511](https://blueprint.hackclub.com/user-attachments/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6OTcwOTUsInB1ciI6ImJsb2JfaWQifX0=--c65cb620d28a59c8d721ff22c8affd2f764f5b4b/Sn%C3%ADmek%20obrazovky%202026-02-02%20172511.png)

2/2/2026 7:02 PM - Wiring diagram
Time spent: 1.0h
When I had the 3D models ready, I moved on again! I went to make a wiring diagram so that everything could be seen clearly...
First I looked for all the components, but for example the charging module was missing, so we had to create it, add pins to it,...
It was fast and in about an hour with a few mistakes I had it finished.
![circuit_image (2)](https://blueprint.hackclub.com/user-attachments/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6OTcxMjMsInB1ciI6ImJsb2JfaWQifX0=--b2c4e6909dd5a191f0a0e8d11423718aeec311e2/circuit_image%20(2).png)

2/2/2026 7:50 PM - Codinggggg
Time spent: 1.2h
I threw myself into writing code but I had one big PROBLEM! I had to figure out how to wirelessly connect Google Home to ESP32...
I did some research and found Sinric.pro, which should work great for my project.
It can turn things on and off, it can also trigger routines, etc. So I started writing and defined my code:

= Monitor strip

= Plug

= BambuLab

= BambuLab homing
![image](https://blueprint.hackclub.com/user-attachments/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6OTcxNDcsInB1ciI6ImJsb2JfaWQifX0=--eb4ba4ab928420e2e8e0b442580c7559040974d1/image.png)

2/2/2026 8 PM - BOM and GitHub editing
Time spent: 0.5h
When I had some free time, I started creating BOM and other things for GitHub!
I had to find exactly the things for my project and I did it... I also created a REPO and converted the **.**STL files to .STEP.
![image](https://blueprint.hackclub.com/user-attachments/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6OTcxNjMsInB1ciI6ImJsb2JfaWQifX0=--6ae0056b144898c430badf7c8d48d7f1ce6f8c25/image.png)

2/2/2026 9:26 PM - 3D rendering
Time spent: 0.7h
Lastly, I created a render where you can see how the parts fit into the holes and it fits.
I searched for a touch sensor on the internet for a while and couldn't find it, so I searched under the exact name and finally found it!!
![image](https://blueprint.hackclub.com/user-attachments/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6OTcxODgsInB1ciI6ImJsb2JfaWQifX0=--bef1885b6b9a9bb103581271ee009071b32e2f8c/image.png)
2/2/2026 9:30 PM - Home control panel
Time spent: 0.9h
With the ESP32-C3 you can control your smart home and other things, all wirelessly!
![image](https://blueprint.hackclub.com/user-attachments/representations/redirect/eyJfcmFpbHMiOnsiZGF0YSI6OTcxODgsInB1ciI6ImJsb2JfaWQifX0=--bef1885b6b9a9bb103581271ee009071b32e2f8c/eyJfcmFpbHMiOnsiZGF0YSI6eyJmb3JtYXQiOiJwbmciLCJyZXNpemVfdG9fbGltaXQiOlsyMDAwLDIwMDBdLCJjb252ZXJ0Ijoid2VicCIsInNhdmVyIjp7InF1YWxpdHkiOjgwLCJzdHJpcCI6dHJ1ZX19LCJwdXIiOiJ2YXJpYXRpb24ifX0=--0f85faa91c373105a0f317054e965c1f47e93a37/image.png)
Why did I do this?
I was thinking about what project to do and I came up with this, I mainly wanted to create it because I needed some kind of device to control my smart home.
Features
4 touch buttons
OLED display as an indicator of e.g. time
Rechargeable battery for carrying
Wireless compatibility
Wiring Diagram
![image](https://blueprint.hackclub.com/user-attachments/representations/redirect/eyJfcmFpbHMiOnsiZGF0YSI6OTcxMjMsInB1ciI6ImJsb2JfaWQifX0=--b2c4e6909dd5a191f0a0e8d11423718aeec311e2/eyJfcmFpbHMiOnsiZGF0YSI6eyJmb3JtYXQiOiJwbmciLCJyZXNpemVfdG9fbGltaXQiOlsyMDAwLDIwMDBdLCJjb252ZXJ0Ijoid2VicCIsInNhdmVyIjp7InF1YWxpdHkiOjgwLCJzdHJpcCI6dHJ1ZX19LCJwdXIiOiJ2YXJpYXRpb24ifX0=--0f85faa91c373105a0f317054e965c1f47e93a37/circuit_image%20(2).png)
Scripts
The script is designed for ESP32-C3 and is just a simple script, so just upload it, adjust the names, API,... and you're done!
How it works?
When you charge the battery, you turn on the system by holding down the 1st button (as far away from the OLED as possible) and the system will connect to the network and other peripherals. Then you just need to press the buttons and, for example, your light will turn on/off.
Libraries:
```
WiFi.h
SinricPro.h
SinricProSwitch.h
Wire.h
Adafruit_GFX.h
Adafruit_SSD1306.h
WiFiUdp.h
NTPClient.h
```

3D models
Here is a view of the top and bottom of the case:
![image](https://github.com/mavory/Home-control-panel/blob/main/Photos/Sn%C3%ADmek%20obrazovky%202026-02-02%20204903.png?raw=true)
![image](https://github.com/mavory/Home-control-panel/blob/main/Photos/Sn%C3%ADmek%20obrazovky%202026-02-02%20205055.png?raw=true)
Bill of Materials (BOM)
Item	Quantity	Price	Link
ESP32-C3 Super Mini	1	$7.17	Laskakit
OLED display	1	$3.05	AliExpress
Touch buttons (TTP223)	4	$1.21	Laskakit
Wires (10cm, 20pcs)	1	$2.71	Laskakit
Battery Li-ion 18650	1	$7.66	Laskakit
Charger module / Boost converter	1	$1.36	Laskakit
Shipping to CZ	-	$3.49	-
Total		$26.55	

3/14/2026 12:07 AM - Beginning with an error
Time spent: 3.2h
After a long time, a package with components arrived and I could start finishing my project!
![1000025877 (1)](https://blueprint.hackclub.com/user-attachments/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6MTIwNTIwLCJwdXIiOiJibG9iX2lkIn19--d9af06afb408eba932b30b095672dcad9488eeb9/1000025877%20(1).jpg)
I said to myself that I would go for it and started working on the 1st part, where I had prepared and unpacked everything first! Then I just prepared a place for soldering and started soldering the first things that were needed.
![1000025901](https://blueprint.hackclub.com/user-attachments/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6MTIwNTIxLCJwdXIiOiJibG9iX2lkIn19--2eb2e11fce869701b8d8b925af62054cd04fdc54/1000025901.jpg)
But before I started soldering everything, I wanted to test if the ESP32-C3 was working at all... It was on but not showing up on my PC. So unfortunately I had to scrap my plans and start fixing the ESP.
It took me a long time to find the cause and then I looked for help on the internet forums - I used a program called "Zadig" which rewrote the port so it would show up properly and I could program it via the Arduino IDE!
![Snímek obrazovky 2026-02-25 183338](https://blueprint.hackclub.com/user-attachments/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6MTIwNTI0LCJwdXIiOiJibG9iX2lkIn19--6fab7f5b2e63d6f94bac6a487ee2d9bd58e4955e/Sn%C3%ADmek%20obrazovky%202026-02-25%20183338.png)

3/14/2026 12:24 AM - Soldering!!
Time spent: 1.9h
After a minor mistake, I came to my senses and started doing something serious! So I took all the buttons and gradually had to fit each one with a cable and then connect all the GND and VCC so that only 1 cable was created from them. The worst thing was when my cables kept falling apart and I couldn't take it anymore...
![1000025906](https://blueprint.hackclub.com/user-attachments/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6MTIwNTMxLCJwdXIiOiJibG9iX2lkIn19--8ba41561fa8d879dd3cc42c771c570639d5e6800/1000025906.jpg)
Immediately after that I continued soldering the battery, where I had to be careful about the temperature... I managed it well! When I had it connected to the cables, I wrapped it with tape to prevent a short circuit in the box and the cable from getting caught.
Next, I took the charging module and everything went quite well...
![1000025903](https://blueprint.hackclub.com/user-attachments/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6MTIwNTM3LCJwdXIiOiJibG9iX2lkIn19--69b5f0cf89d76339c3d9db61f991504d919db80d/1000025903.jpg)
Except for one small thing - it seemed that when I wanted to test if it was charging, the charging module kept showing me that the device was drawing power even though I had nothing connected.
So I decided to use my old charging module, which I had for a long time and I must say that it works very well!
![1000025927](https://blueprint.hackclub.com/user-attachments/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6MTIwNTM4LCJwdXIiOiJibG9iX2lkIn19--dbde7426396d3e08c74415ae1bb8b31abfd4c44a/1000025927.jpg)

3/14/2026 3 PM - Assembling components into a case
Time spent: 2.5h
FAHHHHH.....
I had everything I needed ready and soldered all the cables to the ESP32. It went quickly, but many times the cables came loose from the holes, so I kept having to put it back together... When something fell off, I soldered it even more tightly, then another cable fell off and I thought it was just a hallucination - but it wasn't, so I took the tin and soldered everything tightly to make it hold.
So I started gluing things into place right after that and it went very well! I accidentally glued my finger in place a few times, but after a while I had it done. The OLED display fit into place very well and I was happy about that!
![20260228_232732](https://blueprint.hackclub.com/user-attachments/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6MTIwNzc3LCJwdXIiOiJibG9iX2lkIn19--4dc7b2637202fc0b815adebb9d86b57aa2d1da0c/20260228_232732.jpg)
Furthermore, I didn't know how to put it all together at all and I thought for a long time how to do it. First, I experimented with putting the battery on top and the ESP32 below it, but as I found out, the small integrated antenna couldn't handle it and the WiFi didn't work. It took me a long time to solve this case because I didn't know it would cause such a problem.
![1000025993](https://blueprint.hackclub.com/user-attachments/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6MTIwODEzLCJwdXIiOiJibG9iX2lkIn19--62d2a824313591498921f8d4f19331066f32d4ae/1000025993.jpg)
So I continued with the idea that I would put the battery on the bottom, spread the cables all over the case and the ESP32 would go on top. Hopefully it will work....
![1000026701](https://blueprint.hackclub.com/user-attachments/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6MTIwODE0LCJwdXIiOiJibG9iX2lkIn19--95cddf20bd86611222ca6ad7a3054d75d659e8a2/1000026701.jpg)
3/14/2026 4:10 PM - My first test!!!
Time spent: 0.8h
I was excited, so I quickly ran closer to the WiFi to test if the system would connect! And suddenly I plugged in the cable and everything worked fine. I was happy that the buttons were recognized well and when I plugged the battery into the adapter, it charged (YAYYYY).
The display lit up, but the WiFi didn't connect very well and I said to myself that I'll have to redo the whole code from scratch because it's terrible for now and I'll probably change it overall, if only it would work.
![1000026028 (1)](https://blueprint.hackclub.com/user-attachments/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6MTIwODIxLCJwdXIiOiJibG9iX2lkIn19--ad43fb2fda2afa13246f497c160a54743c22a2a1/1000026028%20(1).jpg)
When I put it back, some cables "unexpectedly" broke off again and I had to take out my soldering iron again and solder it carefully... But I had it finished in a few minutes and I could continue!
![1000026700](https://blueprint.hackclub.com/user-attachments/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6MTIwODIzLCJwdXIiOiJibG9iX2lkIn19--8c7ccddd87a8da7bc387bf1c9bb7806e2cd6f262/1000026700.jpg)

3/14/2026 4:53 PM - Case completion
Time spent: 1.1h
I told myself that if it works for me, I'll glue it and upload the code online via .bin! I had the quick transfer code ready and I was able to upload it before closing. It went well and I took the glue in my hand and glued the 2 parts together.
It was tedious, because when I glued it somewhere, it came off somewhere again, so it took longer than I expected. I also couldn't avoid sticky fingers... Then I sanded the case to clean it of the glue that came off my 3D case and that was it for now.
![1000026703](https://blueprint.hackclub.com/user-attachments/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6MTIwODQxLCJwdXIiOiJibG9iX2lkIn19--54957a5352e08d79bec9100bcee47c58ee167a1f/1000026703.jpg)

3/14/2026 7:23 PM - Codinggggg
Time spent: 1.8h
At first I thought it would be fun to code it, but after a while I still didn't know how to make the code work, but to connect to the Bambu printer and other things. So I wrote the basics, which also took me a long time. Sometimes the WiFi simply wouldn't load at all, or for example the buttons weren't recognized when I pressed them 3 times, etc.
![20260228_230813](https://blueprint.hackclub.com/user-attachments/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6MTIwOTU4LCJwdXIiOiJibG9iX2lkIn19--858c30d4429e09f0e80189fa7b44af532f6d6a5d/20260228_230813.jpg)
So I already had the basics, and I also added an improvement - monitoring the information from the printer on the OLED display, so I wouldn't have to go to the printer screen all the time.
Everything worked very well, but I had to do a few things that were important for my code, because I was already at the stage where I couldn't solve the bugs, so I asked for help and the code is now working beautifully!!!
![20260314_151218](https://blueprint.hackclub.com/user-attachments/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6MTIwOTYyLCJwdXIiOiJibG9iX2lkIn19--e945b9aeaccc562a6fb57c9504899314cc074ef4/20260314_151218.jpg)

3/14/2026 7:36 PM - Testing and completion
Time spent: 2.5h
![20260314_193845](https://blueprint.hackclub.com/user-attachments/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6MTIwOTc0LCJwdXIiOiJibG9iX2lkIn19--d0e49f89a1e6b126ed3b6bd25a119fa4626b30ec/20260314_193845.jpg)
After a long time and high cortisol because of everything, I finished my project and went to try everything!! I thought it would be best if I tried everything at once and I started with the printer - it worked perfectly and then I also tried the device controlled via Sinric and everything worked perfectly. Only sjem sometimes had problems with WiFi, but that was to be expected, because sjem had almost no antenna and everything was still in the box.
![Snímek obrazovky 2026-03-14 192722](https://blueprint.hackclub.com/user-attachments/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6MTIwOTY3LCJwdXIiOiJibG9iX2lkIn19--c32b841cb60b0efd843b0801b3b4dbcd8f0d72a1/Sn%C3%ADmek%20obrazovky%202026-03-14%20192722.png)
Website completion
I forgot to finish the website so I could control everything and change settings through it. So I had that done in about a couple of hours and I did it in my favorite black and white theme! I also added the ability to upload new code (I already had all these things in the old one, so I just added it and didn't have to code it from scratch) and it was done.
![Snímek obrazovky 2026-03-14 193538](https://blueprint.hackclub.com/user-attachments/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6MTIwOTY5LCJwdXIiOiJibG9iX2lkIn19--a8b7c00eabe1aa90d29b68ed2bd926aeabeb6830/Sn%C3%ADmek%20obrazovky%202026-03-14%20193538.png)
