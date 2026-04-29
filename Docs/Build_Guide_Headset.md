​1. The Hardware List
​Microcontroller: ESP32-S3 Sense (mainboard + OV5640 camera board).
​Display: 12.2mm x 17.8mm I2C OLED.
​Optics: 3x jeweler's loupe lens, salvaged from this: https://www.michaels.com/product/enkay-head-magnifier-visor-10782298
​Reflector: 2x rectangular lens also salvaged from above visor.
​Power Module (External): 3.7V LiPo, TP4056 1A Charging Module, SPDT physical slide switch.
​Consumables: 30 AWG highly flexible silicone wire, hot glue, 120 and 400 grit sandpaper.
​2. Printing the Chassis
​Print the V64 Upper Canopy vertically with supports and the Lower Sled with the angled face on the bed, no supports needed.
​The Why: Print in PETG. It will handle the marine environment way better than PLA. The ESP32-S3 also generates significant heat when processing video streams. PLA will eventually reach its glass transition temperature and warp under the thermal load, throwing your optics out of alignment. PETG gives you the necessary thermal stability while maintaining the mechanical flex needed for the OLED pinch-locks to snap without shearing off.
​3. Hacking the Optics (The 3x Lens)
​We need 65mm focal length for the display to project clearly into your eye, which makes the 3x jeweler's lens perfect.
​Pop the acrylic lens out of the headset frame.
​Mark an 18mm x 18mm square dead in the center of the lens.
​Lay your 120-grit sandpaper flat on the bench, and grind the edges down to your marks. Keep the lens perfectly perpendicular so you don't introduce a bevel. Finish with 400-grit to polish out micro-fractures.
​The Why: The front barrel of the CAD model has an exact 18mm x 18mm interior void. By sanding the lens into an 18mm square block, it will friction-fit perfectly into the barrel, self-aligning perfectly parallel to the OLED. This completely eliminates the need to design and print fragile threaded retaining rings.
​4. External Power & Harness Pre-Wiring
​Do all your soldering before inserting anything into the plastic shell.
​The I2C Harness: Cut four 25mm lengths of 30 AWG silicone wire. Solder the OLED (3.3V, GND, SDA, SCL) to the ESP32.
​The Why: The routing channel between the OLED and the ESP32 is only 2mm tall. You have to make a hard 90-degree bend inside the plastic. Stiff PVC wire will act like a lever and tear the copper pads right off your OLED screen. Silicone wire bends effortlessly.
​The External Power Module: Solder your LiPo battery directly to the B+ and B- terminals on the TP4056 board.
​The Cutoff Switch: Solder a wire from the TP4056 OUT+ terminal to the middle pin of your SPDT slide switch. Solder a long lead wire to one of the outer pins of the switch. Solder another long lead wire to the TP4056 OUT- terminal.
​Sled Pass-Through: Feed those two long lead wires through the 3.0mm hole in the rear wall of the printed lower sled (going from the outside to the inside).
​Mainboard Power: Solder those leads directly to the BAT+ and BAT- pads on the bottom of the ESP32-S3.
​The Why: By offloading the charging to the TP4056, you bypass the ESP32's painfully slow 50mA trickle charge, allowing you to charge the LiPo at a full 1A. The SPDT switch provides a true hardware power cutoff so the 3.3V regulator doesn't drain your battery while the glasses sit on your desk.
​5. Loading the Sled
​Drop the ESP32 Stack: Take the dual-board ESP32 stack and drop it into the 16.0mm rear pocket of the lower sled. Push it down so it rests flat against the completely sealed 1.6mm plastic floor.
​The Why: Sealing the bottom floor creates a completely captive pocket. The board cannot fall out, and dust/sweat cannot ingress through the bottom.
​Route the Camera: Carefully fold the camera ribbon forward and seat the OV5640 lens into the 9mm aperture sitting immediately under the OLED slot.
​Seat the Screen: Slide the OLED down into its focal slot. It’s going to feel slightly loose resting in the bottom half of the sled—that is intentional.
​Wire Check: Make sure your I2C wires are tucked cleanly into the gap space and aren't overlapping any of the four structural peg holes.
​6. The Pinch-Lock & Seal
​The Clamshell Snap: Take the Upper Canopy, align the four structural pegs with the lower sled, and squeeze the halves shut.
​The Why: As you force the halves together, the top halves of the 1.2mm spheres built into the canopy seam bite down hard onto the top edges of the OLED PCB. You should feel a distinct "click" as the screen gets mechanically clamped in place so it cannot vibrate out of focus.
​Strain Relief: Inject a bead of hot glue into the 3.0mm rear hole from the inside to seal the power wires.
​The Why: It blocks dust and acts as a brutal, highly effective strain relief so a snagged external battery cable won't destroy your ESP32.
​Attach Reflector: Mount your 45-degree reflector onto the front clevis hinge using a 3.2mm pin.
