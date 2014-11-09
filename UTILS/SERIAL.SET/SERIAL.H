/*    Serial.h    */


void closeserial(void);
void interrupt com_int (void);
int getccb(void);
void closeserial(void);
void initserial (void);
void i_disable (void);
void i_enable (int pnum);
void resvects (void);
void setvects (void);
void comon (void);
int SerialOut (char x);
int setserial (int Port,int Speed,int Parity,int Bits,int StopBit);

#define TRUE    1
#define FALSE   0
#define SBUFSIZ 0x2000

#define COM1BASE        0x3F8   /* base port for com1                   */
#define COM2BASE        0x2F8   /* base port for com2                   */

/*                                                                      */
/*      registers for serial port (not all used)                        */
/*                                                                      */

#define TX      0       /*      Transmit register       */
#define RX      0       /*      receive register        */
#define IER     1       /*      Interrupt Enable        */
#define IIR     2       /*      Interrupt ID            */
#define LCR     3       /*      Line control            */
#define MCR     4       /*      Modem control           */
#define LSR     5       /*      Line Status             */
#define MSR     6       /*      Modem Status            */
 #define DLL    0       /*      Divisor Latch Low       */
#define DLH     1       /*      Divisor latch high      */

/*                                                                      */
/*      Status values                                                   */
/*                                                                      */

#define RCVRDY  0x01    /*      Data ready flag         */
#define OVRERR  0x02    /*      Overrun error           */
#define PRTYERR 0x04    /*      Parity error            */
#define FRMERR  0x08    /*      Framing error           */
#define BRKINT  0x10    /*      Break interrupt         */
#define XMTRDY  0x20    /*      Transmit register empty */
#define XMTRSR  0x40    /*      Tx shift register empty */

/*                                                      */
/*      Status values for modem register                */
/*                                                      */

#define CTS     0x10
#define DSR     0x20
#define RI      0x40
#define CD      0x80

#define DTR     0x01
#define RTS     0x02
#define OUT2    0x08

#define IMR     0x21
#define ICR     0x20
#define EOI     0x20
#define RX      0
#define IIR     2
#define RX_MASK 7
#define RX_ID   4

#define IRQ3    0xf7
#define IRQ4    0xef

#define MCR     4
#define IER     1
#define MC_INT  8
#define RX_INT  1
#define RX_MASK 7
#define RX_ID   4

#define BUFOVFL 1       /* buffer overflowed    */

#define COM1 1
#define COM2 2

#define NO_PAR 0
#define EV_PAR 1
#define OD_PAR 2
