<p align="center">
 <img src="Images/adafruit_products_3wired.jpg" alt="KJSCE_code_breakers"/>
 <img src="Images/adafruit_pt100.jpg" alt="KJSCE_code_breakers"/>
</p>
<a name="top"></a><h1> <span style="color:red"> Adafruit guide for max 31865 with pt100 sensor</span> <b> </b> <span style="color:red"> Experiment </span> <b> </b> <span style="color:red"> Developer </span> <b> </b> <span style="color:red"> Formula </span> <b> </b> <span style="color:red"> Contact </span> </h1>

<a name="Guide"></a>
# Guide
<b> <span style="color:orange"> Click on the below link for the guide to adafruit connection and code explanation </span> </b>
<b><span style="color:violet"> <br> <a href = "https://learn.adafruit.com/adafruit-max31865-rtd-pt100-amplifier/overview"> Link</span></b> <br/></a>
<br/>
<hr>

<a name="What to do!"></a>
# What to do!
<b> <span style="color:orange"> For 3 wired pt100, we have to first short the 2/3 Wire terminals and also open circuit the 24 terminal. Please check the circuit connection for these soldered terminals. Soldering of all the connection for spi pins are preferable.   </span> </b>
<div align="right">
    <b><a href="#top">↥ back to top</a></b>
</div>
<br/>
<hr>

<a name="What not to do!"></a>
## What not to do!
<b> <span style="color:orange"> Do not use 3 wired connection as a 4wire pt100 by shorting the red wired connections. If we do the connection of 4 wired then the resistance of the Force+ and RTD+ does not get subtracted by the differnece of RTD+  and RTD- resistances. Thus wire 3 wired connection only for 3 wired pt100. </span> </b>
<div align="right">
    <b><a href="#top">↥ back to top</a></b>
</div>
<br/>
<hr>

<a name="Testing values"></a>
## Testing values
<b> <span style = "color:orange"> For testing of higher temperatures we can't test by keeping pt100 at high temperatures so we instead use resistors for testing. By adding appropriate resistors and matching the values in the given conversion table and thus ensuring the corresponding temperatures.
<b><span style="color:violet"> <br> <a href = "https://www.intech.co.nz/products/temperature/typert.html"> Link for conversion table</span></b> <br/></a>
 
<b> Resistance between RTD+ and RTD- | <b> Resistance between RTD+ and Force+| <b>  Theoritical temperature difference </b> |<b> Practical temperature difference <b>
:--|:--|:-:|:-:
105| 0 | 105 | 105
105| 10 | 95 | 95
202| 39 | 163 | 162
202| 78 | 124 | 122
202| 65 | 137 | 139
202| 75 | 127 | 129
<div align="right">
    <b><a href="#top">↥ back to top</a></b>
</div>
<br/>
<hr>

<a name="Steps helpful while debugging"></a>
## Steps helpful while debugging
<b>1. If showing Low or High threshold error then please check that 24 and 3 terminals are properly open circuited or not and also check with another max31865 breakout board.<br>2. If showing "REFIN- > 0.85 x Bias" or "REFIN- < 0.85 x Bias - FORCE- open" or "RTDIN- < 0.85 x Bias - FORCE- open" then check the proper connection of spi pins corresponding to the spi pins of arduino pins as well.<br>3. If showing high resistance values please check that in code if you have changes the connection for 3WIRE.<br>4. If showing low resistance values then check the connection for VCC and ground</b>

<div align="right">
    <b><a href="#top">↥ back to top</a></b>
</div>
<br/>
<hr>
