<h1 align=center>
  📡 Micro Radar
</h1>
<h6 align=center>
  a tiny open-source flight radar for your desk
</h6>
<p align=center>
  <img src="https://github.com/user-attachments/assets/52e1c6fd-17ba-4838-9518-a1c8790af4a8" alt="drawing" width="500"/>
</p>
<p align=center>
  <a href="#prerequisites">PREREQUISITES</a> - <a href="#assembly">ASSEMBLY</a> - <a href="#usage">USAGE</a>
</p>

## Prerequisites
At the core of this project is the the ESP32-C3 module, which comes attached to a 240x240 IPS screen. The module powers the entire project and makes everything possible without needing to solder any components together.

I decided to use a dark grey PLA filament for the print, but any filament should work fine (and of course any colour you'd like to use!)

I've also included a glass lens in front of the screen for mine, it's entirely optional, but in my opinion the result looks more polished with the lens. If you go with the lens, you'll also need some clear-drying epoxy (not super glue, it'll fog up the lens. Ask me how I know.)

### Shopping List
Here is everything you'll need to purchase before starting the build. 

I've linked some of the specific products that I bought, I recommend these for ease of bulding. Please be prepared to make modifications to the enclosure and/or code if deviating from the hardware I've used. A lot of these can be found elsewhere, too.

- [ ] [1.28" Round GC9A01 240×240 IPS Display Module with ESP32-C3 (no-touch)](https://www.aliexpress.com/item/1005008482665220.html)
- [ ] [USB-C Ribbon Extension Cable (5cm, CMUP-CFPCB-BK)](https://www.aliexpress.com/item/1005005371248824.html)
- [ ] [M2 Heat-set Threaded Inserts (+ soldering iron)](https://www.aliexpress.com/item/1005008493831823.html)
- [ ] [32.5mm Round Mineral Glass Lens (optional, recommended)](https://www.aliexpress.com/item/1005004783554496.html)
- [ ] [Gorilla Epoxy (necessary for fitting lens, useful anyway)](https://www.amazon.co.uk/Gorilla-Glue-25ml-Epoxy/dp/B009NQQJFC)

### Accounts / API
This project uses OpenSky's API for retrieving flight data.

I highly recommend making an account, as it's free, and allows the radar to make many more requests per day (400 -> 4000), which makes the live view much more accurate. However, it isn't necessary if you prefer.

You can sign up [here](https://opensky-network.org), or search "OpenSky".

Further info on what to do with the account is in the usage section.

## Assembly

Once you have everything needed for the build, the next step is to assemble everything.

I recommend going through the [Usage](#usage) section BEFORE assembly! It'll make any potential troubleshooting much easier :)

### Step 1 - 3D Print

<img width="400" alt="FFCBBECA-6165-4138-8C84-16AB375511A2_1_105_c" src="https://github.com/user-attachments/assets/21c0753c-7d7c-425c-bdf6-0df037a8fdaa" />

Print all of the STLs provided in `./hardware/stl/`. You should have:
- 1 main enclosure
- 1 front plate
- 1 bezel
- 2 spacers

### Step 2 - Heat-set threaded inserts

First, insert two 2mm M2 threaded inserts into the larger holes on the front plate.

<img width="400" alt="IMG_7882" src="https://github.com/user-attachments/assets/defcfb2c-cdff-4bf1-84b9-7fceeefb0caf" />

Next, the spacers (these may warp, it's fine. They'll also be a little thicker than in the picture). 2x6mm M2 inserts.

<img width="400" alt="IMG_7887" src="https://github.com/user-attachments/assets/73b95049-5f12-4e2b-983a-5242c05f9106" />

Finally, the main enclosure, 2x5mm M2 inserts.

<img width="400" alt="IMG_7891" src="https://github.com/user-attachments/assets/e36f3eec-31b5-468e-8451-9c428eaf9c21" />

Et voilà.

<img width="400" alt="IMG_7896" src="https://github.com/user-attachments/assets/97337223-223c-4531-90e1-f511adfb3d66" />

### Step 3 (optional) - Fitting the lens

<img width="400" alt="IMG_7902" src="https://github.com/user-attachments/assets/e555f787-ca87-4558-b1eb-107f9071f96e" />

This is, in my opinion, the most stressful part of the build. It's quite a pain to not get epoxy on the lens. Here are some tips:
- Apply epoxy to the front plate, not the lens
- Lower the front plate onto the lens (easier to clean any excess epoxy that's squeezed out)
- Have some kind of cleaner nearby to clean up the edges (I just used nail polish, ymmv.)
- Less is more when it comes to applying the epoxy
- Work on a sheet which the epoxy won't stick to to avoid gluing it to the table

<img width="400" alt="IMG_7911" src="https://github.com/user-attachments/assets/aa497389-efd5-45c3-84dc-c997232889ac" />

### Step 4 - Bezel

To secure the bezel to the front plate, use 2x5mm M2 screws through the threaded inserts added to the front plate.

<img width="400" alt="IMG_7914" src="https://github.com/user-attachments/assets/37a3502a-83e1-4552-a399-9a914e0ec973" />

Then screw 2x10mm M2 screws through the remaining two holes, they should stick through the back like so:

<img width="400" alt="IMG_7915" src="https://github.com/user-attachments/assets/9ccfe5f2-347d-4563-a2b1-eb5e65e1d83f" />

With those screws poking through, remove the protective film from the screen and position it over the lens.

Screw on the spacers to clamp the module in place. I recommend keeping the board plugged in for this to make aligning the screen with the lens easier.

Also, don't forget to attach the antenna to the chip - you likely won't receive any WiFi signal without it!

<img width="400" alt="IMG_7917" src="https://github.com/user-attachments/assets/ee53aac0-d119-4941-a814-f7ef23ffe7a0" />

<img width="400" alt="IMG_7920" src="https://github.com/user-attachments/assets/0d4d7d86-9787-4972-aa55-8ae43c9a078b" />

You don't want to use much force to keep the board in place, otherwise it may place too much tension on the screen, causing discolouration.

I used a bit of epoxy once it was secured in place, as I knew I wouldn't be using the module for anything else. I do recommend this to ensure it stays in place if knocked.

### Step 5 - Final assembly

Attach the USB-C ribbon cable to the case with the provided nuts and bolts.

<img width="400" alt="IMG_7921" src="https://github.com/user-attachments/assets/f40a7943-c880-4718-9e69-c87a4f5d33aa" />

<img width="400" alt="IMG_7923" src="https://github.com/user-attachments/assets/2daccb36-421f-4a3e-812a-51dae4444d4e" />

Optionally, remove the supports on the bottom and insert rubber feet.

<img width="400" alt="IMG_7924" src="https://github.com/user-attachments/assets/fdeb69f2-ec0d-441e-95ca-abd7523f7c61" />

Finally, plug in the board and attach the front plate using 4x7mm M2 screws.

<img width="400" alt="IMG_7925" src="https://github.com/user-attachments/assets/40da22d9-447d-4ad0-a500-02f862050e5c" />

Done!

<img width="400" alt="IMG_7930" src="https://github.com/user-attachments/assets/989fb56f-dacc-4bf5-a9ab-cb1311e534e4" />

## Usage

### Flashing the Firmware

You'll need [VS Code](https://code.visualstudio.com/) with the [PlatformIO IDE extension](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide) installed. Once installed, restart VS Code, open the repository folder, all dependencies will be pulled in automatically.

Plug the board in via USB-C, then hit the upload button (→) in the bottom status bar. If the board doesn't boot into the new firmware automatically after flashing, hold the BOOT button on the back of the board and press RESET once, then let go of BOOT. 

It should auto-detect your board, but if not, make sure the correct board is selected in the bottom status bar if you get an upload failure.

Read more about PlatformIO [here](https://docs.platformio.org/en/latest/).

### First Boot

On first boot, the radar will broadcast a WiFi hotspot called `MicroRadar-Setup`. Connect to it on your phone or laptop and a configuration page will appear automatically (go to your browser if not). Enter your WiFi credentials and hit save. The board will restart and connect.

### Configuration

Once connected to your network, the radar config is accessible at [http://microradar.local](http://microradar.local) from any device on the same network. From here you can set your location (latitude and longitude), radar radius, display options, and your OpenSky credentials.

If you've made an OpenSky account (again, I highly recommend this), enter your client ID and secret here. You can find these under your account settings at opensky-network.org. Read more about the OpenSky API here.

This webpage will be available whenever the device is on and connected to WiFi, should you want to change any settings.

That's it! Once finished, you should see a live view of all flights over your configured location. Enjoy :)


<img width="400" alt="IMG_7935" src="https://github.com/user-attachments/assets/118b9a1c-c2c0-488d-b638-d8684a30b1d7" />

## Notes
> Designed and developed as part of a wedding present for a mate who loves aviation (congratulations to both him and his wife!)

> Inspired by [therealhacksaw](https://www.instagram.com/therealhacksaw/)'s desk radar

> Built with ♥︎ in London
