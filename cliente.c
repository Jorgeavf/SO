#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/stat.h>
#include <signal.h>

int main(int argc, char* argv[]){	
	char buf[512];
	char buf1[512];
	char buf2[512];
	int r = 0;
	int fd = open("Historico.txt", O_RDWR | O_CREAT | O_APPEND, 0666);
	int n;
	char c[40];
	sprintf(c, "fifoC%d", getpid());
	mkfifo(c, 0666);
	if(argc == 1){
		while((n = read(fd, buf, sizeof(buf))) > 0){
			write(1, buf, n);
		}
	}
	else if((strcmp(argv[1], "help")) == 0){
		strcpy(buf, argv[0]);
		strcpy(buf1, argv[0]);
		strcat(buf1, "proc-file input-filename output-filename priority transformation-id-1 transformation-id-2 ...\n");
		strcat(buf, " status\n");
		write(1, buf, strlen(buf));
		write(1, buf1, strlen(buf1));
	}
	else if((strcmp(argv[1], "proc-file")) == 0 || (strcmp(argv[1], "status")) == 0){ // enviar todos os argv atraves do fifo
		strcpy(buf2, c);
		for(int i = 1; i < argc; i++){
			strcat(buf2, " ");
			strcat(buf2, argv[i]);
		}
		char len[512];
		sprintf(len, "%04ld", strlen(buf2));
		strcat(len, buf2);
		int fifo = open("fifoS", O_WRONLY);
		write(fifo, len, sizeof(len));
		close(fifo);
		int fifoC;
		int ok = 1;
		while(ok && (fifoC = open(c, O_RDONLY))){
			while((n = read(fifoC, buf, sizeof(buf))) > 0){
				buf[n] = '\0';
				if(strcmp(buf, "acabou?") == 0){
					ok = 0;
					break;
				}
				else{
					write(1, buf, n);
				}
			}
		}
	}
	else{
		write(1, "Erro nos argumentos\n", 20);
		r = -1;
	}
	for(int i = 0; i < argc; i++){
		write(fd, argv[i], strlen(argv[i]));
		write(fd, " ", 1);
	}
	write(fd, "\n", 1);
	return r;
}
