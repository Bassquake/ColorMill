# Color Mill
Color Mill VirtualDub 64 bit v2.1 plugin. Used as a RGB, HSV, Saturation, Levels, Brightness, Contrast, Sharpness filter.

<h2>Why</h2>
Decided to update the plugin as I use VirtualDub2 in 64 bit mode to use DeShaker on very large files and currently there's only a 32bit version of Color Mill. So edited the original 32 bit v2.1 source code to 64 bit!

<h2>Credits</h2>
Credit goes to Eugene Khoroshavin for the original code.<br/>
http://fdump.narod.ru/rgb.htm

<h2>How?</h2>
<ul>
  <li>Compiled with Visual Studio Community 2017.</li>
  <li>Open the ColorMill.sln file, choose Release and x64 then run Local Windows Debugger.</li>
  <li>Copy the resulting ColorMill.vdf from "Release" folder to the VirtualDub plugins folder. ("plugin64" for VirtualDub2).</li>
</ul>
Should run on any version of VirtualDub, including VirtualDub2 and VirtualDubMod.<br/>
Or if you prefer, download the plugin files directly from the <em>Win32/Release</em> folder for 32bit or <em>x64/Release</em> for 64bit.<br/>

<h2>Issues</h2>
<s>Currently crashes if move the slider for Sharpness Preprocess. Happens in original code too so looking into it.</s><br/>
Commented out the lines causing crashes when using Preprocess in Sharpness.<br/>
<s>Values would bork in Color Mill when open processing settings.</s><br/>
Put hacky fix for now. There's a line in ScripValue.h (Line 86) that works for 32bit but not 64bit so put an if statement for now.<br/>
