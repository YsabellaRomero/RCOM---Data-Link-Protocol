#include "linklayer.h"
#include "linklayer2.h"

int fd;														//ficheiro a ler
unsigned char C_FIELD_aux = C00;
int DONE;
int LENGTH;
int attempts = 0;

unsigned char * CopiedPacket;

unsigned char SET[5];
unsigned char UA[5];
unsigned char DISC[5];
unsigned char generic[5];
linkLayer new;
Statistics statistics;

////////////////////////////////// sendPacket //////////////////////////////////

void sendPacket()
{
		DONE = FALSE;
		int time = 0;

		printf("time out fornecido %d\n", new.timeOut);

		if(attempts > 0)
			(statistics.timeouts)++;

		if( new.role == TRANSMITTER )
		{
			if(attempts == new.numTries)
			{
				printf("TIMEOUT: Failed 3 times, exiting...\n");
				exit(-1);
			}
			printf("Attempt number %d\n", attempts+1);

			int BytesSent = 0;

			while( BytesSent != LENGTH)
			{
				BytesSent = write(fd, CopiedPacket, LENGTH);
			}

			attempts++;

			if(!DONE)
			{
				alarm(new.timeOut);
			}
		}

			else if( new.role == RECEIVER )
			{
				int BytesSent = 0;

				while( BytesSent != 5 )
				{
					BytesSent = write(fd, CopiedPacket, 5);
				}
			}

			else
			{
				printf("ERROR\n");
				exit(-1);
			}

}

//////////////////////////////Prepare Command///////////////////////////////////

void Prepare(unsigned char * array, unsigned char C)
{
	array[0] = FLAG;
	array[1] = A;
	array[2] = C;
	array[3] = array[1]^array[2];
	array[4] = FLAG;
}

///////////////////////////////////// receivePacket //////////////////////////////////

unsigned char receivePacket(int fd)
{
	unsigned char car, res, Cverif;
	int state=0;

	while( state != 5 )
	{
    res = read(fd,&car,1);
    if( res < 0)
    {
      printf("Read falhou.\n");
      return FALSE;
  	}

		switch(state)
		{
			case 0: //expecting flag
				if(car == FLAG)
					state = 1;
				break;

			case 1: //expecting A
				if(car == A)
					state = 2;

				else if(car != FLAG)
				{
					state = 0;
				}

				break;

			case 2: //expecting Cverif
				Cverif = car;
				if(car == C_UA || car == C_DISC || car == C_SET)
					state = 3;

				else if(car != FLAG)
				{
					state = 1;
				}

				else
					state = 0;

				break;

			case 3: //expecting BCC
				if(car == (A^Cverif))
					state = 4;

				else
				{
					state = 0;
				}

				break;

			case 4: //expecting FLAG
				if(car == FLAG)
					state = 5;

				else
				{
					state = 0;
				}

				break;
		}
	}

	DONE = TRUE;
	alarm(0);
	printf("Success!\n");
	attempts = 0;

	return Cverif;
}

//////////////////////////////////// llopen ////////////////////////////////////

/*int llopen(int porta, TRANSMITER | RECEIVER)
porta: COM1, COM", ...
flag: TRANSMITER / RECEIVER*/

int llopen(linkLayer connectionParameters)
{
	signal(SIGALRM, sendPacket);	//Instala rotina que atende interrupção

	printf("%s\n", connectionParameters.serialPort);
	fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);

	if( fd < 0 )
	{
		printf("Error at opening connection!\n");
		return NOT_DEFINED;
	}

	if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
	}

	bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    //set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
  	newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");

		CopiedPacket = (unsigned char *) malloc(sizeof(unsigned char));

	new = connectionParameters;

	statistics.retransmissions = 0;
	statistics.rejs_sent = 0;
	statistics.timeouts = 0;
	statistics.I_Frame_Received = 0;

	switch(connectionParameters.role)
	{
		case TRANSMITTER:
			Prepare(SET, C_SET);

			copyToPacketToSend(SET, 5);

			sendPacket();
			printf("C_SET enviado\n");

			if(receivePacket(fd) == C_UA)
				printf("C_UA recebido\n");
			break;

		case RECEIVER:
			if(receivePacket(fd) == C_SET){
				printf("C_SET recebido\n");
				Prepare(UA, C_UA);

				copyToPacketToSend(UA, 5);

				sendPacket();
				printf("C_UA enviado\n");
			break;
			}

	}

	printf("ligação estabelecida\n");

	return TRUE;
}

//////////////////////////////////////////////////// llread //////////////////////////////////////

int llread(char *packet)
{

	int size=0, n, res;
	unsigned char *auxbuff = (unsigned char *) malloc(size*sizeof(char));
	unsigned char ctrl_aux;
	unsigned char c;
	int state = 0;
	int i;

  while (state != 5)
  {
		res = read(fd, &c, 1);

    if(res < 0)
		{
			perror("llread");
			return FALSE;
		}

    switch (state)
    {
    //recebe flag
    case 0:
      if (c == FLAG)
        state = 1;
      break;
    //recebe A
    case 1:
      if (c == A)
        state = 2;
      else
      {
        if (c == FLAG)
          state = 1;
        else
          state = 0;
      }
      break;
    //recebe C
    case 2:

      if (c == C00 || c == C10)
      {
				ctrl_aux= c;
        state = 3;
      }

      else
      {
        if (c == FLAG)
          state = 1;
        else
          state = 0;
      }
      break;
    //recebe BCC
    case 3:
      if (c == (A ^ ctrl_aux))
			{
				  state = 4;
					i = 0;
			}
      else
        state = 0;										//enviar REJ e retornar erro
      break;
    //recebe até segunda FLAG
    case 4:
		++size;
		auxbuff = (unsigned char *) realloc(auxbuff, size*sizeof(unsigned char));
		auxbuff[i] = c;	//grava todos os char até ao fim no buffer

		if(c == FLAG){

			n = removetrailer(auxbuff, ctrl_aux, size);

			if(n != -1){
					for (i = 0; i < n; i++) {
						packet[i] = auxbuff[i];
					}
					state = 5;
			}

			else{
				printf("pacote corrompido, REJ enviado\n");
				return 0;
			}

		}
		i++;
		break;
	}
  }
	return n;
}


int destuffing(unsigned char *vet, int size){
	int n = 0;
	for(int i = 0; i < size; i++){
		if(vet[i] == ESCAPE){
			if(vet[i+1] == ESCAPE_FLAG){						//mudar o escape para FLAG e retirar a escapeFLAG
				vet[i] = FLAG;
				for(int j = i+1; j<=size ; j++){
					vet[j]=vet[j+1];
				}
				n++;
				//vet = (char *) realloc(vet, nsize);
			}
			if(vet[i+1] == ESCAPE_ESCAPE){					//retirar a escapeEscape
				for(int j = i+1; j<=size ; j++){
					vet[j]=vet[j+1];
				}
				n++;
				//vet = (char *) realloc(vet, nsize);
			}
		}
	}
	return n;}


int removetrailer(unsigned char *buffer, char ctrl_aux, int size)
{
	int reducedSize = 0;
	int flags;

	unsigned char bcc2vet = buffer[size-2];

	flags = destuffing(buffer, size-2);							//destuffing

	reducedSize = size - flags - 2;

	buffer = (unsigned char *) realloc(buffer, reducedSize);

		unsigned char bcc2cal = BCC2_calc(buffer, reducedSize);

	if( bcc2cal == bcc2vet){

		if(ctrl_aux == C00)
		{
			C_FIELD_aux = RR1;
			Prepare(generic, C_FIELD_aux);
			copyToPacketToSend(generic, 5);
			sendPacket();
		}
		else
		{		//C == C10
			C_FIELD_aux = RR0;
			Prepare(generic, C_FIELD_aux);

			copyToPacketToSend(generic, 5);
			sendPacket();
		}
		(statistics.I_Frame_Received)++;
		return reducedSize;
	}

	else
	{
		if(ctrl_aux == C00){
			C_FIELD_aux = REJ1;								//enviar REJ e retornar erro;
			Prepare(generic, C_FIELD_aux);
			copyToPacketToSend(generic, 5);
			sendPacket();
		}
		else{		//C == C10
			C_FIELD_aux = REJ0;
			Prepare(generic, C_FIELD_aux);
			copyToPacketToSend(generic, 5);
			sendPacket();
		}

		(statistics.rejs_sent)++;
		return -1;
	}
}
/////////////////////////////////// llclose ////////////////////////////////////

int llclose(int showStatistics)
{
		switch(new.role)
		{
			case TRANSMITTER:
				Prepare(generic, C_DISC);
				copyToPacketToSend(generic, 5);
				sendPacket();
				printf("C_DISC enviado\n");

				if(receivePacket(fd) == C_DISC)
					printf("C_DISC recebido\n");

					Prepare(UA, C_UA);
					copyToPacketToSend(UA, 5);
					sendPacket();

					printf("///////////write terminado///////////\n");

					if( tcsetattr(fd, TCSANOW, &oldtio) == -1)
					{
						perror("tcsetattr");
						exit(-1);
					}
				break;

			case RECEIVER:
				if(receivePacket(fd) == C_DISC){
					printf("C_DISC recebido\n");

					Prepare(generic, C_DISC);
					copyToPacketToSend(generic, 5);
					sendPacket();
					printf("C_DISC enviado\n");

					if(receivePacket(fd) == C_UA)
						printf("C_UAC recebido\n");

						printf("///////////read terminado///////////\n");

						if( tcsetattr(fd, TCSANOW, &oldtio) == -1)
						{
							perror("tcsetattr");
							exit(-1);
						}
				break;
				}

		}

		printf("Information about statistics:\n");
		printf("Number of timeouts = %d\n", statistics.timeouts);
		printf("Number of retransmission = %d\n", statistics.retransmissions);
		printf("Number of sent rejs = %d\n", statistics.rejs_sent);
		printf("Number of I frames received = %d\n", statistics.I_Frame_Received);

		return 1;
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// llwrite ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int llwrite(char * buf, int size){

	int sizeNewMsg = size + 6;
	int BytesSent = 0;
	int retransmission = -1; 		//numero de rejs recebidos

	unsigned char *newMsg = (unsigned char *)malloc(sizeof(unsigned char) * (size+6));

	unsigned char BCC2;
	int sizeBCC2 = 1;
	BCC2 = BCC2_calc(buf, size);


	newMsg[0] = FLAG;
	newMsg[1] = A;
	newMsg[2] = C_FIELD_aux;
	newMsg[3] = newMsg[1]^newMsg[2];

	sizeNewMsg = stuffing(buf, newMsg, size, sizeNewMsg);

	newMsg[sizeNewMsg-2] = BCC2;
	newMsg[sizeNewMsg-1] = FLAG;

	unsigned char readChar;

	copyToPacketToSend(newMsg, sizeNewMsg);

	do
	{
			retransmission++;

			if( retransmission > MAX_RETRANSMISSIONS_DEFAULT )
			{
				statistics.retransmissions = MAX_RETRANSMISSIONS_DEFAULT;
					printf("Max number of retransmissions reached\n");
					return -1;
			}

			sendPacket();
			readChar = getCmdExpectingTwo(getExpectingREJ(newMsg[2]), getExpectingRR(newMsg[2]));

	} while( readChar == getExpectingREJ(newMsg[2]) );

	(statistics.retransmissions)+=retransmission;

	complementC();

	free(newMsg);

	return sizeNewMsg;

}

//CÁCULO DE BCC2
unsigned char BCC2_calc(unsigned char *arr, int size)
{
    unsigned char result = arr[0];

    for(int i = 0; i < size; i++)
        result ^= arr[i];

    return result;
}


//FUNÇÃO STUFFING PARA NEW MESSAGE
int stuffing(char * buf, unsigned char * Newmsg, int size, int sizeBuf)
{
	int aux=4;

	for(int i = 0; i < size; i++)
	{
		if( buf[i] == FLAG )
		{
			sizeBuf++;
			Newmsg = (unsigned char *)realloc(Newmsg, sizeBuf);
			Newmsg[aux] = ESCAPE;
			Newmsg[aux+1] = ESCAPE_FLAG;

			aux = aux + 2;
		}

		else if( buf[i] == ESCAPE )
		{
			sizeBuf++;
			Newmsg = (unsigned char *)realloc(Newmsg, sizeBuf);
			Newmsg[aux] = ESCAPE;
			Newmsg[aux+1] = ESCAPE_ESCAPE;

			aux = aux + 2;
		}

		else
		{
			Newmsg[aux] = buf[i];
			aux++;
		}

	}

	return sizeBuf;
}

//REJ ESPERADO
unsigned char getExpectingREJ(unsigned char var)	//FALTA VERSAO PARA RR
{
  if( var == C00 )
    return REJ1;

	else
    return REJ0;
}

//RR ESPERADO
unsigned char getExpectingRR(unsigned char var)
{
  if(var == C00)
    return RR1;

	else
    return RR0;
}


//Complement
void complementC()
{
  if(C_FIELD_aux == C00)
    C_FIELD_aux = C10;

  else
    C_FIELD_aux = C00;
}

//Expected
unsigned char getCmdExpectingTwo(unsigned char expecting1, unsigned char expecting2)
{
	unsigned char readChar, res;
	unsigned char matchExpected = 0;
	unsigned char packet_A, packet_C;

	int state = 0;

	while (state != 5)
	{       /* loop for input */

			res = read(fd, &readChar, 1);

  		if(res < 0)
			{
	      printf("Read falhou.\n");
	      exit(-1);
	  	}

  		switch (state)
			{
				case 0:
  				if (readChar == FLAG)
  					state = 1; // this is the beginning of a packet
  				break;

  			case 1:
  				packet_A = readChar; // we received the byte A, here. Storing.

  				if(readChar == A)
  					state = 2;

					else if(readChar != FLAG)
  					state = 0;
  				break;

  			case 2:
  				packet_C = readChar; // we received the byte C, here. Storing.

					if(readChar == expecting1 || readChar == expecting2)
					{
						matchExpected = readChar;//Save the match to return later
  					state=3;
					}

					else if(readChar != FLAG)
						state = 1;

						else
							state=0;
  					break;

  			case 3:
  				if(readChar == (packet_A ^ packet_C)) //expecting the bcc1
  					state=4;

  				else
  					state=0;
  				break;

  			case 4:
  				if(readChar == FLAG)
  					state = 5;

  				else
  					state=0;
  				break;
  		}
    }

    DONE = TRUE;
		alarm(0);
		attempts = 0;

	return matchExpected;
}

//COPIED PACKET
void copyToPacketToSend(unsigned char * sourcePacket, int length)
{
	LENGTH = length;
	CopiedPacket = (unsigned char *) realloc(CopiedPacket,sizeof(unsigned char) * LENGTH);

  for(int i = 0; i < length; i++){
    CopiedPacket[i] = sourcePacket[i];
  }
}
