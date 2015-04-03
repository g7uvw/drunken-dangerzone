▄▄▄        ▄▄▌  ▄▄▌  ▪   ▐ ▄  ▄▄ •     ▄▄▄         ▄▄▄· ·▄▄▄▄  
▀▄ █·▪     ██•  ██•  ██ •█▌▐█▐█ ▀ ▪    ▀▄ █·▪     ▐█ ▀█ ██▪ ██ 
▐▀▀▄  ▄█▀▄ ██▪  ██▪  ▐█·▐█▐▐▌▄█ ▀█▄    ▐▀▀▄  ▄█▀▄ ▄█▀▀█ ▐█· ▐█▌
▐█•█▌▐█▌.▐▌▐█▌▐▌▐█▌▐▌▐█▌██▐█▌▐█▄▪▐█    ▐█•█▌▐█▌.▐▌▐█ ▪▐▌██. ██ 
.▀  ▀ ▀█▄▀▪.▀▀▀ .▀▀▀ ▀▀▀▀▀ █▪·▀▀▀▀     .▀  ▀ ▀█▄▀▪ ▀  ▀ ▀▀▀▀▀• 

Code for a Teensy 3.1 board to control a 1960s rolling road.
This code is pretty much tied to a specific system, but you're welcome to make use of whatever you can of it.

At startup, ambient air pressure and temp are logged. Then the road comes up to speed. The Teensy tried to hold the road at that speed as engine load is varied. This is achived by PWMing the brake on the road.

