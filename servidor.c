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

char* tranfs[7];

typedef struct List{
	int number;
	int pending;
	char* fifoC;
	char* inst[512];
	pid_t pid;
	struct List* prox;
}*List;

int count(List task){
	int i;
	for(i = 0; task->inst[i] != NULL; i++);
		return i;
}

List enqueue(List queue, List task){
	List new_task = malloc(sizeof(struct List));
	new_task->number = task->number;
	new_task->pending = task->pending;
	new_task->fifoC = strdup(task->fifoC);
	int i;
	for(i = 0; task->inst[i] != NULL; i++){
		new_task->inst[i] = strdup(task->inst[i]);
	}
	new_task->inst[i] = NULL;
	new_task->pid = task-> pid;
	if(queue == NULL){
		new_task->prox = NULL;
		return new_task;
	}
	else{
		List iterator = queue;
		if(strcmp(iterator->inst[3], new_task->inst[3]) < 0){
			new_task->prox = iterator;
			return new_task;
		}
		for(; iterator->prox != NULL && strcmp(iterator->prox->inst[3], new_task->inst[3]) >= 0; iterator = iterator->prox);
			new_task->prox = iterator->prox;
		iterator->prox = new_task;
		return queue;
	}
}

int readchar(int fd, char *buf){
	static char inter_buf[512];
	static int pos = 0;
	static int read_bytes = 0;

	if(pos == read_bytes){
		read_bytes = read(fd,inter_buf,sizeof(inter_buf));
		if(!read_bytes) return 0;
		pos = 0;
	}
	*buf = inter_buf[pos];
	pos++;
	return 1;
}

int readline(int fd, char buf[], int size){
	int pos = 0;
	while(pos < size && readchar(fd, buf + pos)){
		if(buf[pos] == '\n' ){
			pos++;
			break;
		}
		if(buf[pos] == '\0'){
			break;
		}
		pos++;
	}
	return pos;
}

int close_pipe(int pd[2]){
	close(pd[0]);
	close(pd[1]);
	return pipe(pd);
}

int executar(List task, char* path, int instsize, List queue, int val[], int cur_val[]){
	if(strcmp("proc-file", task->inst[0]) == 0){
		int pd[4];
		pipe(pd);
		pipe(pd + 2);
		int i = 4;
		int fifo = open(task->fifoC, O_WRONLY);
		write(fifo, "Processing\n", 11);
		pid_t pid;
		if(instsize != 5){
			if((pid = fork()) == 0){
				char* pa = strcat(path, task->inst[i]);
				int fd = open(task->inst[1], O_RDONLY);
				dup2(fd, 0);
				close(fd);
				close(pd[0]);
				dup2(pd[1], 1);
				close(pd[1]);
				execlp(pa, task->inst[i], NULL);
				perror(task->inst[i]);
			}
			wait(NULL);
			for(i = 5; i < instsize - 1; i++){
				int a = i % 2;
				int b = 1 - a;
				if((pid = fork()) == 0){
					char* pa = strcat(path, task->inst[i]);
					close(pd[b * 2 + 1]);
					dup2(pd[b * 2], 0);
					close(pd[b * 2]);
					close(pd[a * 2]);
					dup2(pd[a * 2 + 1], 1);
					close(pd[a * 2 + 1]);
					execlp(pa, task->inst[i], NULL);
					perror(task->inst[i]);
				}
				close_pipe(pd + b * 2);
				wait(NULL);
			}
			int b = 1 - (i % 2);
			if((pid = fork()) == 0){
				char* pa = strcat(path, task->inst[i]);
				int out = open(task->inst[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
				dup2(pd[b * 2], 0);
				close(pd[b * 2]);
				close(pd[b * 2 + 1]);
				dup2(out, 1);
				close(out);
				execlp(pa, task->inst[i], NULL);
				perror(task->inst[i]);
			}
			close_pipe(pd + b * 2);
			wait(NULL);
			write(fifo, "Concluded\n", 10);
			write(fifo, "acabou?", 7);
			close(fifo);
		}
		else{
			if((pid = fork()) == 0){
				int fd = open(task->inst[1], O_RDONLY);
				int out = open(task->inst[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
				dup2(fd, 0);
				close(fd);
				dup2(out, 1);
				close(out);
				execlp(strcat(path, task->inst[i]), task->inst[i], NULL);
				perror(task->inst[i]);
			}
			wait(NULL);
			write(fifo, "Concluded\n", 10);
			write(fifo, "acabou?", 7);
			close(fifo);
		}
		char dead_pid[8], msg[16] = {0};
		sprintf(dead_pid, "%d", getpid());
		sprintf(msg, "%04ld%s", strlen(dead_pid), dead_pid);
		int fifoS = open("fifoS", O_WRONLY);
		write(fifoS, msg, 16);
		close(fifoS);
	}
	else if(strcmp("status", task->inst[0]) == 0){
		int fifo = open(task->fifoC, O_WRONLY);
		char buf[512] = {0};
		for(List iterator = queue; iterator != NULL; iterator = iterator->prox){
			if(!iterator->pending){
				write(fifo, "task #", 6);
				sprintf(buf, "%d:", iterator->number);
				write(fifo, buf, strlen(buf));
				for(int i = 0; iterator->inst[i] != NULL; i++){
					write(fifo, " ", 1);
					write(fifo, iterator->inst[i], strlen(iterator->inst[i]));
				}
				write(fifo, "\n", 1);
			}
		}
		for(int i = 0; i < 7; i++){
			sprintf(buf, "transf %s: %d/%d (running/max)\n", tranfs[i], cur_val[i], val[i]);
			write(fifo, buf, strlen(buf));
		}
		write(fifo, "acabou?", 7);
		close(fifo);
	}
	else{
		write(1, "Argumentos errados\n", 19);
	}
	return 0;
}

int analisar(char *buf, int val[]){
    char* info = strtok(buf, " \n");
    if((strcmp(info, "nop") == 0)){
    	info = strtok(NULL, " \n");
    	if(info != NULL){
    		for(int i = 0; i < strlen(info); i++){
    			if(!(isdigit(info[i]))){
    				write(1, "Argumentos errados em nop\n", 26);
    				return -1;
    			}
    		}
    		val[0] = atoi(info);
    	}
    	else {
    		write(1, "Argumentos errados em nop\n", 26);
    		return -1;
    	}
    }   
    else if((strcmp(info, "bcompress") == 0)){
    	info = strtok(NULL, " \n");
    	if(info != NULL){
    		for(int i = 0; i < strlen(info); i++){
    			if(!(isdigit(info[i]))){
    				write(1, "Argumentos errados em bcompress\n", 32);
    				return -1;
    			}
    		}
    		val[1] = atoi(info);
    	}
    	else {
    		write(1, "Argumentos errados em bcompress\n", 32);
    		return -1;
    	}
    }
    else if((strcmp(info, "bdecompress") == 0)){
    	info = strtok(NULL, " \n");
    	if(info != NULL){
    		for(int i = 0; i < strlen(info); i++){
    			if(!(isdigit(info[i]))){
    				write(1, "Argumentos errados em bdecompress\n", 34);
    				return -1;
    			}
    		}
    		val[2] = atoi(info);
    	}
    	else {
    		write(1, "Argumentos errados em bdecompress\n", 34);
    		return -1;
    	}
    }
    else if((strcmp(info, "gcompress") == 0)){
    	info = strtok(NULL, " \n");
    	if(info != NULL){
    		for(int i = 0; i < strlen(info); i++){
    			if(!(isdigit(info[i]))){
    				write(1, "Argumentos errados em gcompress\n", 32);
    				return -1;
    			}
    		}
    		val[3] = atoi(info);
    	}
    	else {
    		write(1, "Argumentos errados em gcompress\n", 32);
    		return -1;
    	}
    }
    else if((strcmp(info, "gdecompress") == 0)){
    	info = strtok(NULL, " \n");
    	if(info != NULL){
    		for(int i = 0; i < strlen(info); i++){
    			if(!(isdigit(info[i]))){
    				write(1, "Argumentos errados em gdecompress\n", 34);
    				return -1;
    			}
    		}
    		val[4] = atoi(info);
    	}
    	else {
    		write(1, "Argumentos errados em gdecompress\n", 34);
    		return -1;
    	}
    }
    else if((strcmp(info, "encrypt") == 0)){
    	info = strtok(NULL, " \n");
    	if(info != NULL){
    		for(int i = 0; i < strlen(info); i++){
    			if(!(isdigit(info[i]))){
    				write(1, "Argumentos errados em encrypt\n", 30);
    				return -1;
    			}
    		}
    		val[5] = atoi(info);
    	}
    	else {
    		write(1, "Argumentos errados em encrypt\n", 30);
    		return -1;
    	}
    }
    else if ((strcmp(info, "decrypt") == 0)){
    	info = strtok(NULL, " \n");
    	if(info != NULL){
    		for(int i = 0; i < strlen(info); i++){
    			if(!(isdigit(info[i]))){
    				write(1, "Argumentos errados em decrypt\n", 30);
    				return -1;
    			}
    		}
    		val[6] = atoi(info);
    	}
    	else {
    		write(1, "Argumentos errados em decrypt\n", 30);
    		return -1;
    	}
    }
    else{
    	write(1, "Erro no ficheiro de instruções\n", 31);
    	return -1;
    }
    return 0;
}

int verifica(List task, int val[], int cur_val[]){
	int new_val[7] = {0};
	for(int i = 0; task->inst[i] != NULL; i++){
		for(int j = 0; j < 7; j++){
			if(strcmp(task->inst[i], tranfs[j]) == 0){
				new_val[j]++;
				break;
			}
		}
	}
	for(int i = 0; i < 7; i++){
		if(cur_val[i] + new_val[i] > val[i]) return 1;
	}
	for(int i = 0; i < 7; i++){
		cur_val[i] += new_val[i];
	}
	return 0;
}

List remove_pid(List queue, int pid, int val[], int cur_val[], char* path){
	List iterator = queue;
	int i;
	pid_t new_pid;
	if(iterator->pid == pid){
		for(i = 0; iterator->inst[i] != NULL; i++){
			for(int j = 0; j < 7; j++){
				if(strcmp(iterator->inst[i], tranfs[j]) == 0){
					cur_val[j]--;
					break;
				}
			}
		}
		for(List iter = queue->prox; iter != NULL; iter = iter->prox){
			if(iter->pending && !verifica(iter, val, cur_val)){
				iter->pending = 0;
				i = count(iter);
				if((new_pid = fork()) == 0){
					executar(iter, path, i, NULL, val, cur_val);
					_exit(0);
				}
				iter->pid = new_pid;
			}
		}
		queue = queue->prox;
		free(iterator);
		return queue;
	}
	for(; iterator->prox != NULL && iterator->prox->pid != pid; iterator = iterator->prox);
	for(int i = 0; iterator->prox->inst[i] != NULL; i++){
		for(int j = 0; j < 7; j++){
			if(strcmp(iterator->prox->inst[i], tranfs[j]) == 0){
				cur_val[j]--;
				break;
			}
		}
	}
	for(List iter = queue; iter != NULL; iter = iter->prox){
		if(iter->pending && !verifica(iter, val, cur_val)){
			iter->pending = 0;
			i = count(iter);
			if((new_pid = fork()) == 0){
				executar(iter, path, i, NULL, val, cur_val);
				_exit(0);
			}
			iter->pid = new_pid;
		}
	}
	List freed = iterator->prox;
	iterator->prox = freed->prox;
	free(freed);
	return queue;
}

int main(int argc, char* argv[]){
	tranfs[0] = "nop";
	tranfs[1] = "bcompress";
	tranfs[2] = "bdecompress";
	tranfs[3] = "gcompress";
	tranfs[4] = "gdecompress";
	tranfs[5] = "encrypt";
	tranfs[6] = "decrypt";
	mkfifo("fifoS", 0666);
	int n, fifo, i;
	int task_number = 1;
	int cur_val[7] = {0};
	int val[7];
	for(int i = 0; i < 7; i++){
		val[i] = -1;
	}
	char buf[512] = {0};
	int fd = open(argv[1], O_RDONLY);
	if(fd == -1){
		write(1, "Erro na abertura do ficheiro\n", 29);
		return -1;
	}
	while((n = readline(fd, buf, sizeof(buf))) > 0){
		if(analisar(buf, val) == -1){
			write(1, "Server Shutdown\n", 16);
			return -1; 
		}
	}
	close(fd);
	List queue = NULL;
	List new_task = NULL;
	while((fifo = open("fifoS", O_RDONLY))){
		char to_read[4] = {0};
		while((n = read(fifo, to_read, sizeof(to_read))) > 0){
			if((n = read(fifo, buf, atoi(to_read))) > 0){
    			buf[n] = '\0';
				if(buf[0] == 'f'){
					new_task = malloc(sizeof(struct List));
					new_task->prox = NULL;
					new_task->fifoC = strtok(buf, " ");
					i = 0;
					do{
						new_task->inst[i] = strtok(NULL, " ");
					}
					while(new_task->inst[i++]);
					if(strcmp(new_task->inst[0], "proc-file") == 0){
						int fifoC = open(new_task->fifoC, O_WRONLY);
						write(fifoC, "Pending\n", 8);
						close(fifoC);
						new_task->number = task_number++;
						new_task->pending = verifica(new_task, val, cur_val);
					}
					else{
						new_task->number = 0;
						new_task->pending = 0;
					}
					pid_t pid = -1;
					if(!new_task->pending && (pid = fork()) == 0){
						executar(new_task, argv[2], i - 1, queue, val, cur_val);
						_exit(0);
					}
					new_task->pid = pid;
					new_task->prox = NULL;
					if(new_task->number > 0) queue = enqueue(queue, new_task);
					free(new_task);
				}
				else{
					int dead_pid;
					dead_pid = atoi(buf);
					queue = remove_pid(queue, dead_pid, val, cur_val, argv[2]);
				}
			}
		}
	}
	return 0;
}