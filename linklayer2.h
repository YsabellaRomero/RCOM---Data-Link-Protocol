/*Non-Canonical Input Processing*/

#include <strings.h>


/*#define BAUDRATE B38400*/
#define _POSIX_SOURCE 1 //POSIX compliant source
#define FALSE 0
#define TRUE 1


#define FLAG 0x07E	//slide 26: "All frames are delimited by flags (01111110)
#define A 0x03
#define C_SET 0x03	//comandos enviados pelo emissor e respostas enviadas pelo recetor
#define C_UA 0x07

#define C00 0x00
#define C10 0x20
#define RR0 0x01
#define RR1 0x21
#define REJ0 0x05
#define REJ1 0x25
#define C_DISC 0x0B

#define ESCAPE 0x7D
#define ESCAPE_FLAG 0x5E
#define ESCAPE_ESCAPE 0x5D

volatile int STOP=FALSE;
struct termios oldtio, newtio;

typedef struct Statistics{
  int retransmissions;
  int rejs_sent;
  int timeouts;
  int I_Frame_Received;
} Statistics;


void sendPacket();
void sendPacketTime();
void sendPacket();
unsigned char receivePacket(int fd);
unsigned char BCC2_calc (unsigned char *arr, int size);
int destuffing(unsigned char *vet, int size);
int removetrailer(unsigned char *buffer, char ctrl_aux, int size);
int stuffing(char * buf, unsigned char * Newmsg, int size, int sizeBuf);
unsigned char getExpectingREJ(unsigned char var);
void copyToPacketToSend(unsigned char * sourcePacket, int length);
void complementC();
unsigned char getCmdExpectingTwo(unsigned char expecting1, unsigned char expecting2);
unsigned char getExpectingRR(unsigned char var);
void TT_iReceived();
void Prepare(unsigned char * array, unsigned char C);
