/*
    Sketch designed to receive and send the responses to simulate the Pokemon Snap Station smart card reader
    Copyright (C) March 2015 Steve Muller - lilmul123@gmail.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

 */
 
 // internal variables
 char incoming[40];
 int numOfCredits = 1;
 int numOfBytesRead = 0;
 int resetLoop = 0;
 String incomingStr = "0";
 
void setup() {
  // initialize serial
  Serial.begin(9600);
  Serial.setTimeout(2000); // two seconds per timeout should allow resync within 10 seconds
}

void loop() {
        
  numOfBytesRead = Serial.readBytesUntil(0x03,incoming,40); // wait for 60011273 or 6002170772
  incoming[numOfBytesRead] = '\0';
  incomingStr = incoming;
  
  // The card reader connected to the Pokemon Snap kiosk is a Gemplus GCR410 (also sold as a Gemalto GemPC410).
  // The transport layer is TLP224, which transmits the hexidecimal packet encoded as readable ASCII characters.
  // The GemCore V1.21-Based Reader Reference Manual details the protocol and is worth a read.
  // http://support.gemalto.com/fileadmin/user_upload/user_guide/GemPC410/GemCore_v1.21_Reference_Manual.pdf
  
  // The cards which store the "credits" the machine uses are simple GPM103 memory cards which contain a counter for
  // the number of available credits.  There's no complex authentication or validation of the card itself, as the cards
  // are "read only" pre-paid cards and only allow for debiting credits.
  
  // Messages follow a simple format:
  // 60         = ACK (ACK to previous message)
  //   02       = LN (Length of this packet's data)
  //     0000   = The packet itself
  //         02 = LRC (Checksum, XOR LN + packet together)
  
  // 60       = ACK
  //   01     = LN
  //     17   = COMMAND (Card status - main card)
  //       76 = LRC
  if(incomingStr == "60011776") // status is requested
    {
    // NOTE: The blank space after each string is ETX, *not* a space.
    // 60                   = ACK
    //   07                 = LN
    //     00               = S  (Sequence number)
    //       06             = STAT (Card #0, 5V, Card powered, Card inserted, T=0)
    //         07           = TYPE (Card type - GPM103)
    //           00         = CNF1
    //             00       = CNF2
    //               00     = CNF3
    //                 00   = CNF4
    //                   66 = LRC
    Serial.write("60070006070000000066"); // card is inserted
    
    // 60       = ACK
    //   01     = LN
    //     12   = COMMAND (Power up card)
    //       73 = LRC
    Serial.readBytesUntil(0x03,incoming,40); // wait for 60011273
    
    // 60                   = ACK
    //   07                 = LN
    //     00               = S (Sequence number)
    //       3B0000000000   = ATR (Answer To Reset, this is the default: 0x3B0000000000)
    //                   5C = LRC
    Serial.write("6007003B00000000005C"); // send ack response
    
    // 60                 = ACK
    //   06               = LN
    //     13             = COMMAND (Read data from card)
    //       00           = CLA (Class byte)
    //         B0         = INS (Instruction - Read bytes)
    //           00       = A1 (Address MSB)
    //             02     = A2 (Address LSB
    //               06   = LN (Return LN bytes)
    //                 C1 = LRC
    Serial.readBytesUntil(0x03,incoming,40); // wait for 60061300B0000206C1
    
    // 60                       = ACK
    //   09                     = LN
    //     00                   = S (Sequence number)
    //       46010017A830       = <data> (6 bytes of serial number)
    //                   9000   = SW1 & SW2 (No further qualification - command successful)
    //                       31 = LRC
    Serial.write("60090046010017A830900031"); // write card serial number
    
    // 60                 = ACK
    //   06               = LN
    //     13             = COMMAND (Read data from card)
    //       00           = CLA
    //         B2         = INS (Instruction - Read counter value from GPM103 card)
    //           05       = A1 (Record number)
    //             08     = A2 (Reference control)
    //               02   = LN (Return LN bytes of counter, must always be 0x02)
    //                 C8 = LRC
    Serial.readBytesUntil(0x03,incoming,40); // wait for 60061300B2050802C8
    
    // if a credit is available...
    if(numOfCredits)
      // 60               = ACK
      //   05             = LN
      //     00           = S (Sequence number)
      //       0001       = <data> (Counter value - 0x0001 credit)
      //           9000   = SW1 & SW2 (No further qualification - command successful)
      //               F4 = LRC
      Serial.write("60050000019000F4"); // write "card has one credit" message
    // if a credit is unavailable...
    else
      // 60               = ACK
      //   05             = LN
      //     00           = S (Sequence number)
      //       0000       = <data> (Counter value - 0x0000 credits)
      //           9000   = SW1 & SW2 (No further qualification - command successful)
      //               F5 = LRC
      Serial.write("60050000009000F5"); // write "card has no credits" message
      
    // 60                 = ACK
    //   06               = LN
    //     13             = COMMAND (Read data from card)
    //       00           = CLA
    //         B2         = INS (Instruction - Read counter value of GPM103 card)
    //           05       = A1 (Record number)
    //             08     = A2 (Reference control)
    //               02   = LN (Return LN bytes of counter, must always be 0x02)
    //                 C8 = LRC
    Serial.readBytesUntil(0x03,incoming,40); // wait for 60061300B2050802C8
    
    // if a credit is available...
    if(numOfCredits)
      // 60               = ACK
      //   05             = LN
      //     00           = S (Sequence number)
      //       0001       = <data> (Counter value - 0x0001 credit)
      //           9000   = SW1 & SW2 (No further qualification - command successful)
      //               F4 = LRC
      Serial.write("60050000019000F4"); // write "card has one credit" message again
    // if a credit is unavailable...
    else
      // 60               = ACK
      //   05             = LN
      //     00           = S (Sequence number)
      //       0000       = <data> (Counter value - 0x0000 credits)
      //           9000   = SW1 & SW2 (No further qualification - command successful)
      //               F5 = LRC
      Serial.write("60050000009000F5"); // write "card has no credits" message again
    
    // this is where we check to see if the Snap Station is trying to accept the credit
    numOfBytesRead = Serial.readBytesUntil(0x03,incoming,40); // wait for 6002170772 or 60081400D20508020000A1
    incoming[numOfBytesRead] = '\0';
    incomingStr = incoming;
    
    // "Snap Station wishes to accept credit" message received
    // 60                        = ACK
    //   08                      = LN
    //     14                    = COMMAND (Write data to card)
    //       00                  = CLA
    //         D2                = INS (Instruction - Write counter value of GPM103 card)
    //           05              = A1 (Record number)
    //             08            = A2 (Reference control)
    //               02          = LN (Set LN bytes of counter, must always be 0x02)
    //                 0000      = <data> (Counter vaue - 0x0000 credits)   
    //                     A1    = LRC
    if(incomingStr == "60081400D20508020000A1")
    {
      // 60           = ACK
      //   03         = LN
      //     00       = S (Sequence number)
      //       9000   = SW1 & SW2 (No further qualification - command successful)
      //           F3 = LRC
      Serial.write("6003009000F3"); // write "credit has been deposited" message
      numOfCredits = 0; // credit has been deposited, need to send "empty card inserted" string
      
      // 60         = ACK
      //   02       = LN
      //     17     = COMMAND (define card type and presence detection)
      //       07   = T (Card type - GPM103)
      //         72 = LRC
      Serial.readBytesUntil(0x03,incoming,40); // wait for 6002170772
    }
    
    // This belongs in the above block, is response to previous command.
    // 60       = ACK
    //   01     = LN
    //     00   = S (Sequence number)
    //       61 = LRC
    Serial.write("60010061"); // write final ack message
    
    // if the credit was deposited, send that it is empty ten times, and then set the number of credits back to 1
    if(!numOfCredits)
    {
       if(resetLoop++ >= 10) 
       {
        numOfCredits = 1; 
        resetLoop = 0;
       }
    }
    // 60         = ACK
    //   02       = LN
    //     17     = COMMAND (define card type and presence detection)
    //       07   = T (Card type - GPM103)
    //         72 = LRC
    } else if(incomingStr == "6002170772") { // message end/heartbeat was received
      // 60       = ACK
      //   01     = LN
      //     00   = S (Sequence number)
      //       61 = LRC
      Serial.write("60010061"); // write ack message
  }
}
