/*
A command line web client that allows you to send http requests, download files and 
print the HTTP response.
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>

#define OPTSTRING "u:scio:"
#define ERROR 1
#define REQUIRED_ARGC 3
#define HOST_POS 1
#define PORT_POS 2
#define PROTOCOL "tcp"
#define BUFLEN 1024
#define PORT 80
#define SHORTSIZE 100

char hostname[SHORTSIZE];
char path[SHORTSIZE];
char requestToSend[BUFLEN] = {0};
int fileLength;
char headerStr[BUFLEN];


bool printINF = false;
bool printREQ = false;
bool printRSP = false;
bool makeFile = true;

char *outputFileName = "output";
char *firstPart;
int sizeread;

struct hostent *hinfo;
struct protoent *protoinfo;
char buffer[BUFLEN];
int sd, ret;

FILE *ptr;

// method to print the header
void printHeader()
{
    char *fullHeader, *line;
    fullHeader = strdup(headerStr);

    while ((line = strsep(&fullHeader, "\n")) != NULL)
    {
        printf("RSP: %s\n", line);
    }
}

// turn the url to lower case
char *lowercase(char str[])
{
    int j = 0;
    char ch;
    for(int i = 0; str[i]; i++){
    str[i] = tolower(str[i]);
    }
    return str;
}

// method to parse url
void parseUrl(char url1[])
{
    url1 = lowercase(url1);
    sscanf(url1, "http://%99[^/]/%99[^\n]", hostname, path);
}

// method that prints the INF necessary
void infOutput()
{
    printf("INF: hostname = \"%s\"\n", hostname);
    printf("INF: web_filename = \"%s\"\n", path);
    printf("INF: output_filename = %s", outputFileName );
}

// error printing
int errexit(char *format, char *arg)
{
    fprintf(stderr, format, arg);
    fprintf(stderr, "\n");
    exit(ERROR);
}

// makes the HTTP request from the parsed data
void makeHTTP()
{
    sprintf(requestToSend, "GET /%s HTTP/1.1\r\nHost: %s\r\nUser-Agent: CWRU CSDS 325 Client 1.0\r\n\r\n", path, hostname);
}

//prints the Request HTTP
void printHTTP(){
    char *fullHTTP, *line;
    fullHTTP = strdup(requestToSend);
    printf("\n");
    int i = 0;
    while(i < 3)
    {   
        line = strsep(&fullHTTP, "\n");
        printf("REQ: %s\n", line);
        i++;
    }
}

//code taken from socket file provided
void connectToSocket()
{
    struct sockaddr_in sin;
    /* lookup the hostname */
    hinfo = gethostbyname(hostname);
    if (hinfo == NULL)
        errexit("cannot find name: %s", hostname);

    /* set endpoint information */
    memset((char *)&sin, 0x0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT);
    memcpy((char *)&sin.sin_addr, hinfo->h_addr, hinfo->h_length);

    if ((protoinfo = getprotobyname(PROTOCOL)) == NULL)
        errexit("cannot find protocol information for %s", PROTOCOL);

    /* allocate a socket */
    /*would be SOCK_DGRAM for UDP */
    sd = socket(PF_INET, SOCK_STREAM, protoinfo->p_proto);
    if (sd < 0)
        errexit("cannot create socket", NULL);

    /* connect the socket */
    if (connect(sd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        errexit("cannot connect", NULL);
}

//parses the header from the file data under it
void parseHeaderRecieved()
{
    firstPart = strstr(buffer, "\r\n\r\n");
    //moves past the empty carriage returns
    while (firstPart[0] == '\n' || firstPart[0] == '\r' ) 
        memmove(firstPart, firstPart+1, strlen(firstPart));

    int sizeOfHeader = strlen(buffer) - strlen(firstPart);
    //only copies the size of the header
    strncpy(headerStr, buffer, sizeOfHeader);
    sizeread = ret - strlen(headerStr);

    //deletes 3 null characters added during parsing
    int deleteCounter = 0; 
    for(int i = 0; i < sizeread - 1; i++)
	{
		if( firstPart[i] == '\0' && firstPart[i+1] == '\0' && deleteCounter < 4)
		{
			for(int j = i; j < sizeread  ; j++)
			{
				firstPart[j] = firstPart[j + 1]; 
			}
            deleteCounter++;
			sizeread--;
			i--;	
		} 
	}
}

//parses content length from the header
void getContentLength()
{
    char *contentLength = strstr(buffer, "Content-Length:");
    sscanf(contentLength, "Content-Length: %d", &fileLength);
}

//parses and creates the request
void request()
{
    connectToSocket();
    // attempt to send a request to the socket
    if (write(sd, requestToSend, strlen(requestToSend)) < 0)
        errexit("cannot send", NULL);

    memset(buffer, 0x0, BUFLEN);
    ret = read(sd, buffer, BUFLEN - 1);
    if (ret < 0)
        errexit("reading error", NULL);

    parseHeaderRecieved();
    getContentLength();

    ret = fwrite(firstPart, sizeof(char), sizeread, ptr);
    if (ret < 0)
        errexit("writing error", NULL);

    while (sizeread < fileLength)
    {
        memset(buffer, 0, BUFLEN);
        int readsize = read(sd, buffer, BUFLEN);
        if (readsize == 0)
        {
            break; //breaks in case there is no more characters to read
        }
        sizeread += readsize;
        ret = fwrite(buffer, sizeof(char), readsize, ptr);
        if (ret < 0)
            errexit("writing error", NULL);
    }
}

void closeSocket()
{
    /* close & exit */
    close(sd);
    exit(0);
}

void save(char *path)
{
    outputFileName = path;
    if ((ptr = fopen(path, "wb")) == NULL)
    {
        errexit("cannot open file", NULL);
    }
    
}

int main(int argc, char **argv)
{
    char option;
    while ((option = getopt(argc, argv, OPTSTRING)) != EOF)
    {
        switch (option)
        {
        case 'u':
            parseUrl(optarg);
            makeHTTP();
            break;
        case 'o':
            save(optarg);
            break;

        case 'i':
            printINF = true;
            break;

        case 's':
            printRSP = true;
            break;

        case 'c':
            printREQ = true;
            break;
        
        case '?':

        default:
            
            break;
        }
    }
    makeHTTP();
    request();
    if(printINF) infOutput();
    if(printREQ) printHTTP();
    if(printRSP) printHeader();
    closeSocket();
}
