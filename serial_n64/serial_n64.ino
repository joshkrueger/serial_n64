/**
* Send commands to Nintendo 64
* by Alvaro Garcia (maxpowel@gmail.com)
*/

/**
 * Based on Gamecube controller to Nintendo 64 adapter
 * by Andrew Brown
 */

/**
 * To use, hook up the following to the Arduino UNO:
 * Digital I/O 8: N64 serial line
 * All appropriate grounding and power lines
 * Serial connection to the source (ej: computer)
 *
 * Also note: the N64 supplies a 3.3 volt line, but I don't plug that into
 * anything.  The arduino can't run off of that many volts, it needs more, so
 * it's powered externally. Additionally, the arduino has its own 3.3 volt
 * supply that I use to power the Gamecube controller. Therefore, only two lines
 * from the N64 are used.
 */
/*
 Copyright (c) 2009 Andrew Brown

 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation
 files (the "Software"), to deal in the Software without
 restriction, including without limitation the rights to use,
 copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following
 conditions:

 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.
 */

//#include <Console.h>

#include <SPI.h>
#include <Ethernet.h>

byte mac[] = {
  0x90, 0xA2, 0xDA, 0x00, 0xE3, 0x29
};
//IPAddress ip(192, 168, 1, 177);

EthernetServer server(80);

#define N64_PIN 8
#define N64_HIGH DDRB &= ~0x01
#define N64_LOW DDRB |= 0x01
#define N64_QUERY (PINB & 0x01)


char n64_raw_dump[281]; // maximum recv is 1+2+32 bytes + 1 bit
// n64_raw_dump does /not/ include the command byte. That gets pushed into
// n64_command:
unsigned char n64_command;

// bytes to send to the 64
// maximum we'll need to send is 33, 32 for a read request and 1 CRC byte
unsigned char n64_buffer[33];

void get_n64_command();

#include "crc_table.h"

void serverProcess(){
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          // output the value of each analog input pin
          for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
            int sensorReading = analogRead(analogChannel);
            client.print("analog input ");
            client.print(analogChannel);
            client.print(" is ");
            client.print(sensorReading);
            client.println("<br />");
          }
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

void setup()
{
  Serial.begin(115200);
  //Bridge.begin();
  //Console.begin(); 
  // Communication with the N64 on this pin
  digitalWrite(N64_PIN, LOW);
  pinMode(N64_PIN, INPUT);

  
  /*if(Serial){
    Serial.println("USA! USA!");
  } */ 
  //while(!Serial);
  //if(Serial){
  //  Serial.println("USA USA");
  //}
    // start the Ethernet connection and the server:
  /*Ethernet.begin(mac);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
  */
}




/**
 * Complete copy and paste of gc_send, but with the N64
 * pin being manipulated instead.
 * I copy and pasted because I didn't want to risk the assembly
 * output being altered by passing some kind of parameter in
 * (read: I'm lazy... it probably would have worked)
 * PD: gc_send was removed in this version because is not used ;)
 */
void n64_send(unsigned char *buffer, char length, bool wide_stop)
{
  asm volatile (";Starting N64 Send Routine");
  // Send these bytes
  char bits;

  bool bit;

  // This routine is very carefully timed by examining the assembly output.
  // Do not change any statements, it could throw the timings off
  //
  // We get 16 cycles per microsecond, which should be plenty, but we need to
  // be conservative. Most assembly ops take 1 cycle, but a few take 2
  //
  // I use manually constructed for-loops out of gotos so I have more control
  // over the outputted assembly. I can insert nops where it was impossible
  // with a for loop

  asm volatile (";Starting outer for loop");
outer_loop:
  {
    asm volatile (";Starting inner for loop");
    bits = 8;
inner_loop:
    {
      // Starting a bit, set the line low
      asm volatile (";Setting line to low");
      N64_LOW; // 1 op, 2 cycles

      asm volatile (";branching");
      if (*buffer >> 7) {
        asm volatile (";Bit is a 1");
        // 1 bit
        // remain low for 1us, then go high for 3us
        // nop block 1
        asm volatile ("nop\nnop\nnop\nnop\nnop\n");

        asm volatile (";Setting line to high");
        N64_HIGH;

        // nop block 2
        // we'll wait only 2us to sync up with both conditions
        // at the bottom of the if statement
        asm volatile ("nop\nnop\nnop\nnop\nnop\n"
                      "nop\nnop\nnop\nnop\nnop\n"
                      "nop\nnop\nnop\nnop\nnop\n"
                      "nop\nnop\nnop\nnop\nnop\n"
                      "nop\nnop\nnop\nnop\nnop\n"
                      "nop\nnop\nnop\nnop\nnop\n"
                     );

      } else {
        asm volatile (";Bit is a 0");
        // 0 bit
        // remain low for 3us, then go high for 1us
        // nop block 3
        asm volatile ("nop\nnop\nnop\nnop\nnop\n"
                      "nop\nnop\nnop\nnop\nnop\n"
                      "nop\nnop\nnop\nnop\nnop\n"
                      "nop\nnop\nnop\nnop\nnop\n"
                      "nop\nnop\nnop\nnop\nnop\n"
                      "nop\nnop\nnop\nnop\nnop\n"
                      "nop\nnop\nnop\nnop\nnop\n"
                      "nop\n");

        asm volatile (";Setting line to high");
        N64_HIGH;

        // wait for 1us
        asm volatile ("; end of conditional branch, need to wait 1us more before next bit");

      }
      // end of the if, the line is high and needs to remain
      // high for exactly 16 more cycles, regardless of the previous
      // branch path

      asm volatile (";finishing inner loop body");
      --bits;
      if (bits != 0) {
        // nop block 4
        // this block is why a for loop was impossible
        asm volatile ("nop\nnop\nnop\nnop\nnop\n"
                      "nop\nnop\nnop\nnop\n");
        // rotate bits
        asm volatile (";rotating out bits");
        *buffer <<= 1;

        goto inner_loop;
      } // fall out of inner loop
    }
    asm volatile (";continuing outer loop");
    // In this case: the inner loop exits and the outer loop iterates,
    // there are /exactly/ 16 cycles taken up by the necessary operations.
    // So no nops are needed here (that was lucky!)
    --length;
    if (length != 0) {
      ++buffer;
      goto outer_loop;
    } // fall out of outer loop
  }

  // send a single stop (1) bit
  // nop block 5
  asm volatile ("nop\nnop\nnop\nnop\n");
  N64_LOW;
  // wait 1 us, 16 cycles, then raise the line
  // take another 3 off for the wide_stop check
  // 16-2-3=11
  // nop block 6
  asm volatile ("nop\nnop\nnop\nnop\nnop\n"
                "nop\nnop\nnop\nnop\nnop\n"
                "nop\n");
  if (wide_stop) {
    asm volatile (";another 1us for extra wide stop bit\n"
                  "nop\nnop\nnop\nnop\nnop\n"
                  "nop\nnop\nnop\nnop\nnop\n"
                  "nop\nnop\nnop\nnop\n");
  }

  N64_HIGH;

}



bool rumble = false;

void loop()
{

  int i;
  unsigned char data, addr;
  int buttonState = 0;
  char sbuf;

  memset(n64_buffer, 0, sizeof(n64_buffer));

  /* Receive the command from external device
   * The command will be send exactly as received
   */
  //Serial.println("wtf");
  // serverProcess();
  if (Serial.available()) {

    sbuf = Serial.read();
    //Serial.println("woo");
    
    if(sbuf == 's'){
      n64_buffer[0] = n64_buffer[0] | 0x10;
      Serial.println("START");
    }
    
    if(sbuf == 'a'){
      n64_buffer[0] = n64_buffer[0] | 0x80;
      Serial.println("A");
    }

    if(sbuf == 'b'){
      n64_buffer[0] = n64_buffer[0] | 0x40;
      Serial.println("B");
    }

    if(sbuf == 'u'){
      n64_buffer[0] = n64_buffer[0] | 0x08;
      Serial.println("UP");
    }

        if(sbuf == 'd'){
      n64_buffer[0] = n64_buffer[0] | 0x04;
      Serial.println("DOWN");
    }

        if(sbuf == 'l'){
      n64_buffer[0] = n64_buffer[0] | 0x02;
      Serial.println("LEFT");
    }

        if(sbuf == 'r'){
      n64_buffer[0] = n64_buffer[0] | 0x01;
      Serial.println("RIGHT");
    }
    
    //n64_buffer[0] = Serial.read();

    //while (Serial.available() == 0) {}
    //n64_buffer[1] = Serial.read();

    //while (Serial.available() == 0) {}
    //n64_buffer[2] = Serial.read();
    //Serial.write(n64_buffer[2]);

    //while (Serial.available() == 0) {}
    //n64_buffer[3] = Serial.read();
    //Serial.write(n64_buffer[3]);
  }
  //Serial.println("wtf2");
  // Wait for incomming 64 command
  // this will block until the N64 sends us a command
  noInterrupts();
  //Serial.println("wtf2.1");
  get_n64_command();

  // 0x00 is identify command
  // 0x01 is status
  // 0x02 is read
  // 0x03 is write
  //Serial.println("wtf3");
  switch (n64_command)
  {
    case 0x00:
    case 0xFF:
      // identify
      // mutilate the n64_buffer array with our status
      // we return 0x050001 to indicate we have a rumble pack
      // or 0x050002 to indicate the expansion slot is empty
      //
      // 0xFF I've seen sent from Mario 64 and Shadows of the Empire.
      // I don't know why it's different, but the controllers seem to
      // send a set of status bytes afterwards the same as 0x00, and
      // it won't work without it.
      n64_buffer[0] = 0x05;
      n64_buffer[1] = 0x00;
      n64_buffer[2] = 0x01;

      n64_send(n64_buffer, 3, 0);
      //Serial.println("wtf 0x00");
      //Serial.println("It was 0x00: an identify command");
      break;
    case 0x01:
      // Send to n64 the received data

      n64_send(n64_buffer, 4, 0);
      //Serial.println("wtf 0x01");
      //Serial.println("It was 0x01: the query command");
      break;
    case 0x02:
      // A read. If the address is 0x8000, return 32 bytes of 0x80 bytes,
      // and a CRC byte.  this tells the system our attached controller
      // pack is a rumble pack

      // Assume it's a read for 0x8000, which is the only thing it should
      // be requesting anyways
      memset(n64_buffer, 0x80, 32);
      n64_buffer[32] = 0xB8; // CRC

      n64_send(n64_buffer, 33, 1);
      //Serial.println("wtf 0x02");
      //Serial.println("It was 0x02: the read command");
      break;
    case 0x03:
      // A write. we at least need to respond with a single CRC byte.  If
      // the write was to address 0xC000 and the data was 0x01, turn on
      // rumble! All other write addresses are ignored. (but we still
      // need to return a CRC)

      // decode the first data byte (fourth overall byte), bits indexed
      // at 24 through 31
      data = 0;
      data |= (n64_raw_dump[16] != 0) << 7;
      data |= (n64_raw_dump[17] != 0) << 6;
      data |= (n64_raw_dump[18] != 0) << 5;
      data |= (n64_raw_dump[19] != 0) << 4;
      data |= (n64_raw_dump[20] != 0) << 3;
      data |= (n64_raw_dump[21] != 0) << 2;
      data |= (n64_raw_dump[22] != 0) << 1;
      data |= (n64_raw_dump[23] != 0);

      // get crc byte, invert it, as per the protocol for
      // having a memory card attached
      n64_buffer[0] = crc_repeating_table[data] ^ 0xFF;

      // send it
      n64_send(n64_buffer, 1, 1);

      // end of time critical code
      // was the address the rumble latch at 0xC000?
      // decode the first half of the address, bits
      // 8 through 15
      addr = 0;
      addr |= (n64_raw_dump[0] != 0) << 7;
      addr |= (n64_raw_dump[1] != 0) << 6;
      addr |= (n64_raw_dump[2] != 0) << 5;
      addr |= (n64_raw_dump[3] != 0) << 4;
      addr |= (n64_raw_dump[4] != 0) << 3;
      addr |= (n64_raw_dump[5] != 0) << 2;
      addr |= (n64_raw_dump[6] != 0) << 1;
      addr |= (n64_raw_dump[7] != 0);

      if (addr == 0xC0) {
        rumble = (data != 0);
      }

      //Serial.println("It was 0x03: the write command");
      //Serial.print("Addr was 0x");
      //Serial.print(addr, HEX);
      //Serial.print(" and data was 0x");
      //Serial.println(data, HEX);
      //Serial.println("wtf 0x03");
      break;

  }

  interrupts();

  // DEBUG: print it
  //print_gc_status();
  //Serial.println("wtf4");

}

/**
  * Waits for an incomming signal on the N64 pin and reads the command,
  * and if necessary, any trailing bytes.
  * 0x00 is an identify request
  * 0x01 is a status request
  * 0x02 is a controller pack read
  * 0x03 is a controller pack write
  *
  * for 0x02 and 0x03, additional data is passed in after the command byte,
  * which is also read by this function.
  *
  * All data is raw dumped to the n64_raw_dump array, 1 bit per byte, except
  * for the command byte, which is placed all packed into n64_command
  */
void get_n64_command()
{

  int bitcount;
  char *bitbin = n64_raw_dump;
  int idle_wait;

func_top:
  n64_command = 0;

  bitcount = 8;

  // wait to make sure the line is idle before
  // we begin listening
  for (idle_wait = 32; idle_wait > 0; --idle_wait) {
    if (!N64_QUERY) {
      idle_wait = 32;
    }
  }

read_loop:
  // wait for the line to go low
  while (N64_QUERY) {}

  // wait approx 2us and poll the line
  asm volatile (
    "nop\nnop\nnop\nnop\nnop\n"
    "nop\nnop\nnop\nnop\nnop\n"
    "nop\nnop\nnop\nnop\nnop\n"
    "nop\nnop\nnop\nnop\nnop\n"
    "nop\nnop\nnop\nnop\nnop\n"
    "nop\nnop\nnop\nnop\nnop\n"
  );
  if (N64_QUERY)
    n64_command |= 0x01;

  --bitcount;
  if (bitcount == 0)
    goto read_more;

  n64_command <<= 1;

  // wait for line to go high again
  // I don't want this to execute if the loop is exiting, so
  // I couldn't use a traditional for-loop
  while (!N64_QUERY) {}
  goto read_loop;

read_more:
  switch (n64_command)
  {
    case (0x03):
      // write command
      // we expect a 2 byte address and 32 bytes of data
      bitcount = 272 + 1; // 34 bytes * 8 bits per byte
      //Serial.println("command is 0x03, write");
      break;
    case (0x02):
      // read command 0x02
      // we expect a 2 byte address
      bitcount = 16 + 1;
      //Serial.println("command is 0x02, read");
      break;
    case (0x00):
    case (0x01):
    default:
      // get the last (stop) bit
      bitcount = 1;
      break;
      //default:
      //    Serial.println(n64_command, HEX);
      //    goto func_top;
  }

  // make sure the line is high. Hopefully we didn't already
  // miss the high-to-low transition
  while (!N64_QUERY) {}
read_loop2:
  // wait for the line to go low
  while (N64_QUERY) {}

  // wait approx 2us and poll the line
  asm volatile (
    "nop\nnop\nnop\nnop\nnop\n"
    "nop\nnop\nnop\nnop\nnop\n"
    "nop\nnop\nnop\nnop\nnop\n"
    "nop\nnop\nnop\nnop\nnop\n"
    "nop\nnop\nnop\nnop\nnop\n"
    "nop\nnop\nnop\nnop\nnop\n"
  );
  *bitbin = N64_QUERY;
  ++bitbin;
  --bitcount;
  if (bitcount == 0)
    return;

  // wait for line to go high again
  while (!N64_QUERY) {}
  goto read_loop2;
}
