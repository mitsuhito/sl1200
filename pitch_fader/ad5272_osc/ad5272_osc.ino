#include <Wire.h>

//#ifdef ARDUINO_ETHERNET
#include <Ethernet.h>
#include <EthernetUdp.h>
//#endif
//#ifdef ARDUINO_AVR_LEONARDO_ETH
//#include <Ethernet2.h>
//#include <EthernetUdp2.h>
//#endif

#include <OSCBundle.h>

//#define DEBUG true

uint16_t vr = 0;
int return_code = 0; // return code from slave device
int i2c_addr = 0x2e; // default address


EthernetUDP udp;
IPAddress local_ip_addr(10, 0, 2, 10);
const unsigned int local_port = 12000;
IPAddress dst_ip_addr(10, 0, 2, 4);
const unsigned int dst_port = 12000;
byte mac_addr[] = {
  //0x90, 0xA2, 0xDA, 0x10, 0x1A, 0xEA  // given one, leonard
  0x90, 0xA2, 0xDA, 0x10, 0x2C, 0x09  // given one, arduino ethernet r3 #1
  //0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x01  // random one,  arduino ethernet r3 #2
  //0x90, 0xA2, 0xDA, 0x0F, 0xC5, 0xBF    // given one, arduino ethernet r3 #3

};
IPAddress dns_addr(0, 0, 0, 0);
IPAddress gateway_addr(10, 0, 2, 1);
IPAddress subnetmask(255, 255, 255, 0);

uint8_t desired_tx_rate_hz = 15;

unsigned long debug_output_time_stamp;
uint16_t debug_output_rate_ms = 1000;

unsigned long current_vr_output_time_stamp;

void pitch_osc_handler(OSCMessage &msg, int addr_offset)
{
  //Serial.println("pitch_osc_handler()::");
  if (msg.isInt(0))
  {
    int v = msg.getInt(0);
    return_code = write_vr(v);
#ifdef DEBUG
    Serial.print("write value : ");
    Serial.println(v);
    Serial.print("return code : ");
    Serial.println(return_code);
#endif
  }
}

String osc_dst_addr_prefix = "/" + String(local_ip_addr[0]) + "." + String(local_ip_addr[1]) + "." + String(local_ip_addr[2]) + "." + String(local_ip_addr[3]);

void send_osc_msg(uint16_t value)
{
  OSCMessage msg(osc_dst_addr_prefix.c_str());
  msg.add(value);
  udp.beginPacket(dst_ip_addr, dst_port);
  msg.send(udp);
  udp.endPacket();
  msg.empty();
}

void setup()
{
  Serial.begin(9600);
#ifdef ARDUINO_AVR_LEONARDO_ETH
  while (!Serial) {
    ;
  }
#endif
  delay(100);

  Wire.begin();
  Ethernet.begin(mac_addr, local_ip_addr, dns_addr, gateway_addr, subnetmask);
  Serial.print("local ip addr: ");
  Serial.println(Ethernet.localIP());
  udp.begin(local_port);
  debug_output_time_stamp = millis();
  current_vr_output_time_stamp = millis();
}

void loop()
{
  int size;
  OSCMessage msg;

  if ( (size = udp.parsePacket()) > 0)
  {
    while (size--)
      msg.fill(udp.read());

    if (!msg.hasError())
      msg.route("/pitch", pitch_osc_handler);
  }

  vr = read_vr();

#ifdef DEBUG
  if ((millis() - debug_output_time_stamp) > debug_output_rate_ms)
  {
    Serial.print("loop()::");
    Serial.print(millis());
    Serial.print("\t");
    Serial.print("read_vr()::");
    Serial.println(vr, DEC);
    debug_output_time_stamp = millis();
  }
#endif

  if (Serial.available())
  {
    int i = Serial.parseInt();
    Serial.print("write value : ");
    Serial.println(i);
    return_code = write_vr(i);
    Serial.print("return code : ");
    Serial.println(return_code);
  }

  if ((millis() - current_vr_output_time_stamp) > round(1000.0 / desired_tx_rate_hz))
  {
    send_osc_msg(vr);
    current_vr_output_time_stamp = millis();
  }
}

int write_vr(uint16_t r)
{
  r = constrain(r, 0, 1023);
  Wire.beginTransmission(i2c_addr);

  // write contens of the serial register data to the control register
  byte command = 0b00011100;
  Wire.write(command);
  byte data = 0b0110;
  Wire.write(data);

  // write contents of serial register data to RDAC
  command = 0b00000100 | (r >> 8);
  data = r & 0x00ff;
  Wire.write(command);
  Wire.write(data);

  // enable RDAC protection
  command = 0b00011100;
  Wire.write(command);
  data = 0b0100;
  Wire.write(data);

  int code = Wire.endTransmission();     // stop transmitting
  return code;
}

uint16_t read_vr()
{
  Wire.beginTransmission(i2c_addr);

  // write contens of the serial register data to the control register
  byte command = 0b00001000;
  Wire.write(command);
  byte data = 0;
  Wire.write(data);
  int code = Wire.endTransmission();     // stop transmitting

  if (code != 0)
  {
#ifdef DEBUG
    Serial.print("ad5272 read error, return code: ");
    Serial.println(code);
#endif

    // wire transmittion error
    return 0;
  }
  //  Serial.print("request return code:");
  //  Serial.println(code);

  Wire.requestFrom(i2c_addr, 2);
  uint16_t rdac_value = 0;
  if (Wire.available() >= 2)
  {
    byte b = 0;
    b = Wire.read();
    rdac_value = b << 8;
    b = Wire.read();
    rdac_value = rdac_value | b ;
  }
  return rdac_value;
}


/*
<pre><code>
----------begin_max5_patcher----------
4276.3oc6cssjaabD8YouBTa4GjSjflom64o3eg7ZRJWbIgVQadYKRtJx1k+
2ybZ.BBRQtb.AAHsK6xcqk3Fab5d54LW4u8127viK+Zw5Gx9GY+6r27le6su
4M7gvAdS0meyCyG80wyFslurGlWrd8nmJd38kmaSwW2vG+iRQtHmxkhLR32d
5EuLe4KalUrguYY0QmNgukkO9SevY1doeZ4hMKFMufO0OrZ5nYaOS4SXyu7b
Qog9vCY+2F2z5o+JeBIkK180Ncw1uUp5fOOZy3OOcwS+3phwaJeRRkM2QgfL
79LavEu+Le.ZoOWj8ew886u8sP89NBORgHcLgtoXhLHyMVgvDAEiVm6TgP7O
UTtoG.FSKvE4sEWLpbk7ZiKeZ1xnEbbLf9VLvFNOF77nUwiuoX0OVrXziy3q
PbJ7I98OZyCuO6gGGs3o1AVxSCVAeDqbgfV+9LGEimh+swFgIAWHiDWHZ83K
a1rbQxQLV2IiK1+08RdurTX66RGdiVT7+hV52TrXyz4EqROp3RxhV64uZEQ1
CcbL5n5l+N9M+XavA5JW5X5hNV1HdnoyikuwgDmF4rZULmaD4PYDoEXloawU
aV9zSyJRujh5hwmVWHxvUvr2Ka+THJ9ZrZYDHaQcuV8ETRp8gGThni1.bwI6
D5bZNIjBnS5fyMsBXaLTYKvnjxbuKVWb7usxdgwlncPi3VBMFq7nPitaPSKq
s03ux011ziSdStwKjhPmyVzxWKc3J+Z0rD9U705DIAeZzlhLJ4pP0WTY7qIK
h8fmXBPGIDVJFLqGRutR1idcYjnjO1DKosu75iW9xhXc3YxS450e6qLYu.We
ECosxdDBRIPvjXffzVGH3TcBwZYKtBptwoDtjwKmsbU4KkHO1RH90f+KzyC3
OhY.bw3B7J0isT6wm1yRDgf05fAHLdogsIgxnCGXJGw2PNQCVJk8hhuyjVaq
qg9SqqI3hQ+V1g.kVEbRyY8JVmGdBugcEwmRzqDTCtaQ9Wtk8bKJT.IV9HDu
SubWugLvtEwedSjYcAAa.Fiv47klhxXNuuQKzbhLWzpMNcNoiTmkc12Ld474
EnpwCpWdzje5k0axVMYz3rIEyF8KG28s8k+Sqd5Q9y0vQCGpLgd8oi8iWrIh
hbuIF6FQRSrgDaynH6H9bxVckdKttocF7d3xUqyfOUTypulsBz3e270eeGBX
Ff3kXKNopATw4n5Lc9dgjaLOxDNGRpwLzfz4NuB9PjO2FyvfJMBdfI19IOy3
WVsJd7rIOm87x0YuSJdb5ltD6HSYjG5H5zLWby5I6ZxlSD9rJhAEY6F3xjqI
UFt0sQtIihlkyBlgjQgT7GhAiBE5.X4kG.Vta+H23uKF3l8PHLTDUHjZPGst
q8f0s2akHT+V0O83xKSdN9UWL8KEQOPaFxg9uS0SDirVSiAp01K0Kw0Ck87z
ngj8oQSJV8eV7teHKVa0WJpqcZ1zEEb+W0Lp8hpxp+I7Xib8rhPfzfGXfI+3
wvZIxkxPPGadjRckwvJ7J6SSms4ToZRDfz8N.40Qhx9X4ACFNBQtJTx9wqx0
noDgAJHCAUq5gPLY+yoVZy8Bt4EjkhszHBfRTBUFi7h+msCgXsb70klvePGf
civVSs75L.6mrE9urY47QalNNa7miuUwFs8okqx1TrtazuMt9m9sYG8amobj
CiHU7HApG.pHhLMBTESxV877r2oMw+oaPjYPaghUDueWr.HBozT2ApSPr3uG
cL4oylvPWLkxqEcx85SsFcNhuO3c82xT46pn+73iNbegORSt1DChTw5GodIi
z6lWLZ8KqhEzl7Kw22XloUHwz2m8wSM3foUdS0+ojHSHWgdjMhT5XS2pGzTc
j1kqrqH66DSJUWSLo5+DSM6Xoqdho0ylN4TrDNVQLJbQbA1U9pFHDjt0PgKx
3b6rPTGLaGZxxV8I0W6nkOsZ47rwil8x3Xk9Kx9m+qrEyV+tNEuP8eAqHw6b
UEs7XEm4BoPP7eG4kScrf00p6Zkj4V2gstXkVaClhlSrMKkXlUzO0ksq2.R.
cnKb9ZbM61nsIczwFBuM6b4nfb8muh4Bgx68s.gtoSXw8hcZTFqiSXwSD67g
HUBJcnQ5tiBdTA4tfG+f1E1R6eMp3uty4JNp3mrENJUtpEwt5aNE98.ncMwo
iyQwSVs.0pl3HU2W3y0qINm.f9Hx80h1HKo6J.RGafuGUN32Nksu50c1l5Fb
2zoVQSfowLKriyrhV1DFm9V1BllP.IB4TUW910lv7pDqLsXU6Zto7p7N6.xq
ZS1iYeJ8oOu5Rawx6e8LLpN0zERtaBmycpKNz0Emddz3eNSjk93dp0WufHU6
Im2nWBhw9L5nqWMQWavYS1zroIGDoDW9Dw+zTzUcpmTnfomwnOjkdaeovUs0
KcK7grgbUEzT0z2qLz7xuNkqI433i5H3iuCog19uWU7pYxHcihahNEJMcCWm
bpErnjVkq0v0CsZTKaVke0gvXA94okix71mN226GbcSeZwxHVMa53etY.+Ek
KysqeXh0Kx0HJDMZsGiX+3jQaFU0Nw2i+GMT8bBIZoHaoPsTTIJ5VHlDEahh
KAwmfDNunDmQjIHzYD0qH5yHlWQruh3dEw+JR33hVbBQ9JBcBQcBQeDwbBwd
BwcDweDIbbwHNhHOhPGQTGH5iHliH1CD2QD+AR3aEq3.QdfPGHpCD8Ah4.wd
f3NP7GHgchSbfHOPnFh5.Q2PLGH1Fhqg3OPB6DungHaHTCQ0PzMDSCw1PbMD
eCIrSBhFhrgPMDUknaHlFhsRbMDeCITJw5u1WIqUz9JUsRuuxTqr6qb0JesJ
rmRJpUxZEsuRUqz0Jy9JasxUq70pvVEp2rRI2WQ0JUsRWqL0JasxUq70pvVE
pPnRIqUTsRUqz0JSsBOdj8sR4qUgsJsnVIqU3IijiUJcsxTqr0Jdvq80pvVE
xhIQZqJEUqT0pxNFnVYqU3ghTNUJ77P1kJkrVgmGRiTovyCYMpT3QgjDUJ7n
rgsJjQnRgGERBToviBk6knvdkBOJdNoUpvixE1p7b2YHqUDuxwgRWq3NEyVq
vSAkUk9vVUfo2gG.JTVovSAkGknfXkBO.do+xyf0RES6i4yw8+DUq3wGFDlD
lZEO2pAgFAOd63dkhZEyxi39h.JcshavDuFuw8hBDDS2j4PVpX1gLsObuLQM
lEFSwpTg6kYJwzgPjMgfZRwLEYpf31P.bkB2FBfID6R7LQEQrDBVIDrRZl8H
SMD2Q4NRfim9IPg6.giDhDIDDRH9iLLQRSsB2FB8HD0QHViPXFgvLBQXjkaF
IyqjINhK1xWL9NPzDgnIBARDBjHDHQHFhbLMSlKItCD4PHxgP7Bg3EBgJDBU
HOumSvjNYlkL0Qbw7DDgG0+PYq8gBWLhHHtGXELORPRDg.JDBnPHfhmBpvwq
D70AdVRlvItN3tUvSq3g5iGiO3jURdAygqiXlm35fqUAWqh3VSy87KtNl.aI
CTltItNl3HyBjo6w76fySwS6ZMSIEWB2vb32TvkofKSAWlBdKkgolhKA9HE7
QJ3iTvGof6QA2ixxzTwk.Oixw6tU3RfSQ431dgKwwTSYNn3RPQWE7BJ3ETdd
HNYlqLcTbI.6U.6UAdvPwkvHNOjR.w0.r0.r0.r0.r0.m0Rl3JXhBHVWNT73
r.c0.c0DyUkG7EbVTRQyC0.vTshYwxzVwYAbpYVxLkXliaIAVdfJvYY9k.+z
HjWaXpsvp.zoQRUMfNMfNskI3haCnlFnlF.lF.l1wzbYdr3D.qz.qzHXU6YB
uLaVbBfPZfP7FTfNvTbYdsfvJ.GifWU0fSojI6hS.bwfnNCh5L.RLDSyEm.g
YFfFFfFFEy2EDYAPXT7d8G3HqYRv7n8wypQbBl7bISX7TXtrHxwfB1FDzXva
tAEhMHTwfhtFGyKlI+hKFESM75H0yjiYFv3hQAQCBFLHXvh3.KhCrkS.EbLI
SMF7e4I9Eb7V7BZIlWLuK0wGCTWULOYbL7tYw6lE9WKuesgWKKdsrvqZMLoY
bLlNNywlIRWxPFeGvMZQHu0wrjwwfyyhWFqmYMCaF9MKdOrAl8LnECukCQwN
IyfFTiwqfC9HGwrn48NPbIv5cJlMMtDTX2o4kpDyjFWBLbGLbmgYSiKAAgNX
yNKyjFeQLG7RB13N.16f457LwZbG.1c.w8.w8vR8BdEIxbrAIZDM4AN6QwJO
weDLiULca7QftdftdDz3g84g84MLia7QfodKS9FeDvoGvomI1yL2Y53kbtwW
DyVVvztAuZXUAIy.GTqg2OPLIb7QDTGTLObP1FFDudT38ri.RBFP9u.bxAfU
AKSJG2Frk.fo.7pAj4Jfny.ywmIuWwLmYaWxftZUIvzrYxrX+Ea.lCGZ2MXY
HpRdYI7A4tY424lkPM1Wfp1katx8j75wilUjIJ8MU+iL8QNW668wrw9JPkm1
0OmDU2IxxpYUf7puNhHUGlGmjXPmezMGo3dZljK5zhMaPQCE43AFl4v0KSoB
LmkToOqkLta6FVQHbrcuXsqm1tqSGWtwajGVQOrqNepruOWTLY1z4sXTxM5a
I7DhkhTT4RIv3kaQGaXH2aZMC3dSay0.281dSq1bq2aZ2Cc508l1uSBZks.a
to6Y9MwExJyQIlxFL1GyHIr3kSetj3Gj8pDUhfi2kGyfDrbmDLjSzak8OD6U
I6AVJ5JrNka69dp9JuKbrWNCmt+xntZzhIKm+JySli81dqWBM64uEbS978SF
0OVteH7csXAF4uandriXlw1KDVqPGxzlUR7McRvFCb7eK7XU8I7zhM0F0Mck
WENZriNzGUG+xjmWWrXR1te.jdk8+mi0v39eKDoIdXkgs3gTe782I1MvzLN3
mFJ9QyzO1CkVu7kUi25Dq1Rix1MiylfET8hQalFqFZ20fe4mxj0WzmmNYRwh
lPzjoqQkvLPINp6JUyA+fJcV6AMzZnrG48k8veUmydvukNMtn9zdvZc571SX
3rGWJ1yvEOmt8LP3SJwOJ4vYOpTvG8vYOTp1y.Ud2mh8Lf9qTxGp7Cm8Htur
GcHE+kXvrGUJ9K7qIw.YOG7Ucb6QObwy7Wk7b0u6uu7WGXz8N9bV6Y3peOI5
pCm0HSo5BdQdeeYQm0iMe5jmWNcwlpVUPJetb6ZDubopnkZd6Jh2Yr1aYqbM
eaTIgu5ADe0IYQ1AzhRghIuCzLTVDkTRiArXJkVwzgDhLI4zFtJBkIwbg2xB
GJKxjhWKHFPCJEHRObMtT5SsyIFHxcoT47.x0LoJOnvclA4aWUyNcH2ucOzy
y6HSAkstCm4C0CUMmz6RX3xVfkQcFctF5Lf95TxUnFvFBlRCk05g0dNWCuzC
XCuRI4kZ3RdoRpi5FtxWpjZ39.ZOoT2WP0tboZ6t1znJ2c6TJY4+z7Sxx+gv
uURa+kUTI6k7rZ5NqCtRIuOMv1SR4QLCi8jR4VZ35P.cJMzPMb480ozZU8.h
O9TqWbf5v+6r1NaTo1lmAxdr2YCHq69xdtyfmTPG+cm0LLgxtz5JQ28kAQpK
uqiUkSpvlMYkOTOvaxkRAA2vUuWJUyHGtlCFT2WvShcz+.1cn2W1i4Nydjoz
8FA4c03BJufA8h19CtW4OWmJikOT4Rj1jPlqxIn2nme9KEqVWYO7awCyG8Sk
a.37xnDKC0xOxSNvGVU7koau9xesrdXzpwed5lhwadYU47N7qd6CuEeQ+9a+
+0TKPXC
-----------end_max5_patcher-----------
</code></pre>

*/

