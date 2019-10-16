# ColorMill
ColorMill VirtualDub 64 bit v2.1 plugin

<h2>Why</h2>
Decided to update the plugin as I use VirtualDub2 in 64 bit mode to use DeShaker on very large files and currently there's only a 32bit version of Color Mill.

Edited the original 32 bit v2.1 source code to 64 bit.

<h2>Credits</h2>
Credit goes to Eugene Khoroshavin for the original code.<br/>
http://fdump.narod.ru/rgb.htm

<h2>How?</h2>
Compiled with Visual Studio Community 2017.<br/>
Open the ColorMill.sln file, choose Release and x64 then run Local Windows Debugger.<br/> 
Copy the resulting ColorMill.vdf from "Release" folder to the VirtualDub plugins folder. ("plugin64" for VirtualDub2).<br/>
Should run on any version of VirtualDub, including VirtualDub2 and VirtualDubMod.

<h2>Issues</h2>
Currently crashes if move the slider for Sharpness Preprocess. Happens in original code too so looking into it.
