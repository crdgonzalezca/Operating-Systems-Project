#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include <fcntl.h>

/* ------------------------------------------------------*/

#define TABLE_SIZE 1000
#define PORT 3535
#define BACKLOOG 32
#define MAX_PROCESOS 1

sem_t *semaforo; //semaforo ingresar-borrar
sem_t *semaforo2; //semaforo ver-borrar
sem_t *semaforo3; //semaforo buscar-borrar
sem_t *semaforo4; //semaforo insertar-buscar
int count_clientes = 1;

typedef struct dogType{
	char nombre[32];
	char tipo[32];
	char raza[16];
	int edad;
	int estatura;
	int peso;
	char genero;
	int nextDog;
	int backDog;
}Dog;

void ingresar(int socket_desc);
void insertar(Dog dog_insertar);
void ver(int socket_desc);
void borrar(int socket_desc);
void buscar(int socket_desc);
int dispersion(char nombre[]);
int n_registros();
void historia_clinica(Dog auxDog, FILE *fd);
void update_log(char *accion, char *perro, char *ip);
void get_ip(int descriptor, char ip[20]);


/*Función que calcula el codigo hash del nombre del perro*/
int dispersion(char nombre[]){
	int hash = 7;
	int i  = 0;
	while(1) {
	    if(nombre[i] == '\0'){
	    	break;
	    }

	    if(nombre[i] >= 65 && nombre[i] <= 90)
	    	nombre[i] += 32;

	    hash = hash * 31 + nombre[i];
	    i++;
	}	

	if(hash < 0){
		hash *= -1;
	}
	return hash % TABLE_SIZE;
}

/*Funcion que toma como argumento el decriptor del socket, y hace un recv del cliente, que
es la estructura a insertar.*/
void ingresar(int socket_desc){
	int r;
	Dog *dog_recived = malloc(sizeof(Dog));

	r = recv(socket_desc, dog_recived, sizeof(Dog), 0);
	
	sem_wait(semaforo);
	insertar(*dog_recived);
	sem_post(semaforo);

	int n_regs = n_registros();

	char aux[7], ip[20];
	sprintf(aux, "%d", n_regs);
	get_ip(socket_desc, ip);
	update_log("Insercion", aux, ip);
}

/* Funcion para insertar en el archivo dataDog.dat, el perro se inserta al final del del archivo y ademas,
se actualizan los valores del archivo correspondiente de la hash. */
void insertar(Dog dog_insertar){
	int posPrimero = 0;
	int posUltimo = 0;
	int key = dispersion(dog_insertar.nombre);
	
	char path[16];
	sprintf(path, "%s%d%s", "./hash/", key, ".dat");

	FILE *data;
	data = fopen("dataDogs.dat", "r+");

	FILE *hash;
	hash = fopen(path, "r+");

	int result = fread(&posPrimero, sizeof(int), 1, hash);
	
	if(result == 1){
		Dog auxDog;

		fseek(hash, sizeof(int), SEEK_SET);
		fread(&posUltimo, sizeof(int), 1, hash); //Lee en el hash el ultimo guardado con esa colision
	
		fseek(data, sizeof(Dog) * posUltimo, SEEK_SET);
		fread(&auxDog, sizeof(Dog), 1, data); // lee el struct del ultimo guardado en esa colision
		
		fseek(data, 0, SEEK_END);
		auxDog.nextDog = ftell(data) / sizeof(Dog);

		fseek(data, sizeof(Dog) * posUltimo, SEEK_SET);
		fwrite(&auxDog, sizeof(Dog), 1, data); //actualiza el estruct leido con el nuevo apuntador
		
		dog_insertar.backDog = posUltimo;
		fseek(data, 0, SEEK_END);
		fwrite(&dog_insertar, sizeof(Dog), 1, data); //inserta el nuevo perro al final de dataDogs.dat

		posUltimo = ftell(data) / sizeof(Dog) - 1;
		fseek(hash, sizeof(int), SEEK_SET);
		fwrite(&posUltimo, sizeof(int), 1, hash); // actualiza el hash la poscion del ultimo guardado
		
	} else { // entra aca cuando el archivo hash no tiene ningun valor
		fseek(data, 0, SEEK_END);
		posPrimero = ftell(data) / sizeof(Dog);
		posUltimo = ftell(data) / sizeof(Dog);
		
		fseek(hash, 0, SEEK_SET);
		fwrite(&posPrimero, sizeof(int), 1, hash);
		fwrite(&posUltimo, sizeof(int), 1, hash);
		
		fseek(data, sizeof(Dog) * posUltimo, SEEK_SET);
		fwrite(&dog_insertar, sizeof(Dog), 1, data);
	}
	
	fclose(hash);
	fclose(data);
}

/*Funcion que retorna un entero, el cual es el numero de registros que hay en el archivo dataDogs.dat*/
int n_registros(){
	FILE *data;
	data = fopen("dataDogs.dat", "r");
	if(data == NULL){
		perror("No se ha podido abrir el archivo");
		exit(-1);
	}
	fseek(data, 0, SEEK_END);
	int numero_regs = ftell(data) / sizeof(Dog);
	fclose(data);
	return numero_regs;
}

/*Funcion que recibe un entero como argumento, el cual es el numero de registro a ver, y se retorna
la estructura del perro que se quiere ver*/
Dog traer_registro(int reg){
	int r;
	Dog aux;
	FILE *data;
	data = fopen("dataDogs.dat", "r");
	if(data == NULL){
		perror("No se ha podido abrir el archivo dataDogs.");
		exit(-1);
	}

	fseek(data, reg * sizeof(Dog), SEEK_SET);
	r = fread(&aux, sizeof(Dog), 1, data);
	if(r == -1){
		perror("Error al leer el archivo.");
	}
	fclose(data);
	return aux;
}


/*Esta funcion, primero le envia el cliente cuantos perros hay actualmente, despues de eso recive del
cliente el numero de registro que este quiere ver y luego se le envia al cliente la estructura del perro.*/
void ver(int socket_desc){
	/*Esta funcion debe recibir un registro del cliente y enviar la estructura
	del perro que desea ver*/
	Dog dog_ver;
	int r, n_regs, registro;
	n_regs = n_registros();
	r = send(socket_desc, &n_regs, sizeof(int), 0);
	if(r == -1){
		perror("Error al enviar el # de regs en ver.");
		close(socket_desc);
		exit(-1);
	}

	r = recv(socket_desc, &registro, sizeof(int), 0);
	if(r == -1){
		perror("Error al recibir el registro.");
		close(socket_desc);
		exit(-1);
	}
	
	FILE *historia;
	char path[35];
	sprintf(path, "%s%d%s", "./historias_clinicas/", registro, ".txt");
	
	historia = fopen(path, "r");
	dog_ver = traer_registro(registro - 1);
	r = send(socket_desc, &dog_ver, sizeof(Dog), 0);

	char linea[4096];
	// valida si el archivo existe
	if(historia == NULL){ // si el archivo no existe
		historia = fopen(path, "w+");		
		historia_clinica(dog_ver, historia);
		fputs("Histora: ", historia);
		send(socket_desc, "Historia: ", sizeof(linea), 0);
	} else {
		int i;
		for(i = 0; i < 8; i++){
			fgets(linea, 4096, historia);
		}
		while(fgets(linea, 4096, historia) != NULL){
			r = send(socket_desc, linea, sizeof(linea), 0);
		}
		
	}	
	linea[0] = '\0';
	r = send(socket_desc, linea, sizeof(linea), 0);
	fclose(historia);

	historia = fopen(path, "w+");
	historia_clinica(dog_ver, historia);
	do{
		r = recv(socket_desc, linea, sizeof(linea), 0);
		fputs(linea, historia);
	} while(linea[0] != '\0');

	fclose(historia);
	
	char aux[7], ip[20];
	sprintf(aux, "%d", registro);
	get_ip(socket_desc, ip);
	update_log("Lectura", aux, ip);

}

void historia_clinica(Dog auxDog, FILE *fd){
	
	fputs("*********************** HISTORIA CLÍNICA *************************\n", fd);
	fputs("Nombre del animal: ", fd);
	fputs(auxDog.nombre, fd);
	fputs("\nTipo de animal: ", fd);
	fputs(auxDog.tipo, fd);
	fputs("\nRaza del animal: ", fd);
	fputs(auxDog.raza, fd);
	fputs("\nEdad: ", fd);
	char aux[4];
	sprintf(aux, "%d", auxDog.edad);
	fputs(aux, fd);
	fputs("\nEstatura: ", fd);
	sprintf(aux, "%d", auxDog.estatura);
	fputs(aux, fd);
	fputs("\nPeso: ", fd);
	sprintf(aux, "%d", auxDog.peso);
	fputs(aux, fd);
	fputs("\nGénero: ", fd);
	char genero[6];
	if(auxDog.genero == 'H'){
		strcpy(genero, "Hembra");
	}else{
		strcpy(genero, "Macho");
	}
	fputs(genero, fd);
	fputs("\n", fd);
}

/* Funcion que va a reenlazar o hacer la accion correspondiente, para cambiar los apuntadores
relacionado con el registro a borrar */
void reenlazar(int reg_borrar){
	
	FILE *dogs;
	dogs = fopen("dataDogs.dat", "r+");
	if(dogs == NULL){
		printf("No se pudo abrir el archivo dataDogs.dat");
		exit(-1);
	}

	Dog dog;
	int auxBackDog;
	int auxNextDog;
	//Guardamos la direccion de la posicion anterior y siguiente del registro que vamos a borrar
	fseek(dogs, reg_borrar * sizeof(Dog), SEEK_SET);
	fread(&dog, sizeof(Dog), 1, dogs);
	auxNextDog= dog.nextDog;
	auxBackDog= dog.backDog;
 	
	Dog auxDog;


	// este if se va a ejecutar cuando la estructura que se va a eliminar es la unica estructura
	// que se encuentra en ese hash, y eso se sabe cuando los apuntadores son negativos
	// lo que se hace es dejar el archivo con 0 bytes
	if(auxBackDog == -1 && auxNextDog == -1){
		FILE *hash;
		int key = dispersion(dog.nombre);
		char path[16];
		sprintf(path,"%s%d%s","./hash/", key, ".dat");

		hash = fopen(path, "w+");
		fclose(hash);
	} else if(auxBackDog == -1){ // cuando el apuntador anterior es -1, significa que es la cabeza de la lista
		// lo que se hace es leer el archivo de la hash que coincide con la estructura a borrar
		// y actualizar la posicion del primero con la siguiente estructura y se actualiza la nueva estructura
		// que va a quedar como cabeza de lista, esto es poniendo un -1 en el apuntador backDog
		FILE *hash;
		int key = dispersion(dog.nombre);
		char path[16];
		sprintf(path,"%s%d%s","./hash/", key, ".dat");

		hash = fopen(path, "r+");

		fwrite(&auxNextDog, sizeof(int), 1, hash);
		fclose(hash);

		fseek(dogs, sizeof(Dog) * auxNextDog, SEEK_SET);
		fread(&auxDog, sizeof(Dog), 1, dogs);
		auxDog.backDog = -1;
		fseek(dogs, sizeof(Dog) * auxNextDog, SEEK_SET);
		fwrite(&auxDog, sizeof(Dog), 1, dogs);
	} else if(auxNextDog == -1) { // este caso es cuando el registro a borrar es el final de la lista
		// 	lo que se hace es ir a la estructura a la que apunta backDog actualizar
		// el archivo hash, la posicion ultima con la posion de backDog
		FILE *hash;
		int key = dispersion(dog.nombre);
		char path[16];
		sprintf(path,"%s%d%s","./hash/", key, ".dat");

		hash = fopen(path, "r+");

		fseek(hash, sizeof(int), SEEK_SET);
		fwrite(&auxBackDog, sizeof(int), 1, hash);
		fclose(hash);

		fseek(dogs, sizeof(Dog) * auxBackDog, SEEK_SET);
		fread(&auxDog, sizeof(Dog), 1, dogs);
		auxDog.nextDog = -1;
		fseek(dogs, sizeof(Dog) * auxBackDog, SEEK_SET);
		fwrite(&auxDog, sizeof(Dog), 1, dogs);				
	} else {
		fseek(dogs, sizeof(Dog) * auxBackDog, SEEK_SET);
		fread(&auxDog, sizeof(Dog), 1, dogs);
		auxDog.nextDog = auxNextDog;
		fseek(dogs, sizeof(Dog) * auxBackDog, SEEK_SET);
		fwrite(&auxDog, sizeof(Dog), 1, dogs);

		fseek(dogs, sizeof(Dog) * auxNextDog, SEEK_SET);
		fread(&auxDog, sizeof(Dog), 1, dogs);
		auxDog.backDog = auxBackDog;
		fseek(dogs, sizeof(Dog) * auxNextDog, SEEK_SET);
		fwrite(&auxDog, sizeof(Dog), 1, dogs);				
	}
	fclose(dogs);
}

/*Funcion que va a actualizar los archivos hash cuando los registros a los que apuntan cambian.*/
void update_hash(char nombre[], int primera, int ultima){
	FILE *hash;
	int key = dispersion(nombre);
	char path[16];
	sprintf(path,"%s%d%s","./hash/", key, ".dat");

	hash = fopen(path, "r+");

	int posicion;

	if(primera == 1){
		fseek(hash, 0, SEEK_SET);
		fread(&posicion, sizeof(int), 1, hash);
		posicion--;
		fseek(hash, 0, SEEK_SET);
		fwrite(&posicion, sizeof(int), 1, hash);		
	}

	if(ultima == 1){
		fseek(hash, sizeof(int), SEEK_SET);
		fread(&posicion, sizeof(int), 1, hash);
		posicion--;
		fseek(hash, sizeof(int), SEEK_SET);
		fwrite(&posicion, sizeof(int), 1, hash);
	}

	fclose(hash);
}

/*Funcion que copia cada registro una posicion antes en un archivo temporal, esta funcion complementa
toda la accion de borrado.*/
void copiar(int reg_borrar){	
	FILE *dogs;
	FILE *tmp;
	Dog dog;

	dogs = fopen("dataDogs.dat", "r+");

	if(dogs == NULL){
		printf("No se pudo abrir el archivo dataDogs.dat");
		exit(-1);
	}

	tmp = fopen("tmp.dat", "w+"); //archivo temporar donde guardaremos todas las estructuras menos la que queremos borrar
	if(tmp == NULL){
		printf("No se pudo abrir el archivo tmp.dat");
		exit(-1);
	}

	while((ftell(dogs) / sizeof(Dog)) != n_registros()){
		fread(&dog, sizeof(Dog), 1, dogs);
		int pos = ftell(dogs) / sizeof(Dog) - 1;
		if(pos > reg_borrar){

			if(dog.backDog == -1 && dog.nextDog == -1){
				fwrite(&dog, sizeof(Dog), 1, tmp);
				//update hash, le resta 1 a la primera y ultima posicion
				update_hash(dog.nombre, 1, 1);
			} else if(dog.backDog == -1){
				dog.nextDog -= 1;
				fwrite(&dog, sizeof(Dog), 1, tmp);
				//update hash la primera posicion, restarle 1
				update_hash(dog.nombre, 1, 0);
			} else if(dog.nextDog == -1){
				if(dog.backDog > reg_borrar){
					dog.backDog -= 1;
				}
				fwrite(&dog, sizeof(Dog), 1, tmp);
				//update hash la ultima posicion, restarle 1
				update_hash(dog.nombre, 0, 1);
			} else if(dog.backDog < reg_borrar){		
				dog.nextDog -= 1;
				fwrite(&dog, sizeof(Dog), 1, tmp);

				Dog auxDog;
				fseek(dogs, sizeof(Dog) * dog.backDog, SEEK_SET);
				fread(&auxDog, sizeof(Dog), 1, dogs);
				auxDog.nextDog -= 1;
				fseek(tmp, sizeof(Dog) * dog.backDog, SEEK_SET);
				fwrite(&auxDog, sizeof(Dog), 1, tmp);				
			} else {
				if(dog.nextDog > -1){
					--dog.nextDog;
					--dog.backDog;
					fwrite(&dog, sizeof(Dog), 1, tmp);
				}
			}
			fseek(dogs, sizeof(Dog) * (pos + 1), SEEK_SET);
			fseek(tmp, 0, SEEK_END);
		} else if(pos < reg_borrar){
			if(dog.nextDog > reg_borrar){
				dog.nextDog -= 1;
			}
			fwrite(&dog, sizeof(Dog), 1, tmp);
		}
	}

	fclose(dogs);
	fclose(tmp);
	remove("dataDogs.dat");
	rename("tmp.dat", "dataDogs.dat");		
}

/*Funcion que primero le envia al cliente el numero de registros que hay actualmente, luego,
recive del cliente el numero de registro que este quiere borrar y ahi se ejecuta las acciones para
borrar ese registro.*/

void borrar(int socket_desc){
	int r, n_regs, registro;
	n_regs = n_registros();
	r = send(socket_desc, &n_regs, sizeof(int), 0);
	if(r == -1){
		perror("Error al enviar el # de regs en ver.");
		close(socket_desc);
		exit(-1);
	}

	r = recv(socket_desc, &registro, sizeof(int), 0);
	if(r == -1){
		perror("Error al recibir el registro a borrar.");
		close(socket_desc);
		exit(-1);
	}

	sem_wait(semaforo);
	reenlazar(registro - 1);
	copiar(registro - 1);
	sem_post(semaforo);

	char aux[7], ip[20];
	sprintf(aux, "%d", registro);
	get_ip(socket_desc, ip);
	update_log("Borrado", aux, ip);
}


/*FUncion que, primero, recive el nombre de los perros que el cliente quiere buscar, 
luego hace una busqueda de todos los que coincidan con el nombre, y se los va enviando al cliente
uno por uno.*/
void buscar(int socket_desc){
	
	send(socket_desc, "T", sizeof(char), 0);

	char nombre[32];
	int r;
	r = recv(socket_desc, nombre, sizeof(nombre), 0);
	if(r == -1){
		perror("Error al recibir el nombre a buscar.");
		close(socket_desc);
		exit(-1);
	}

	FILE *data;
	FILE *hash;
	data = fopen("dataDogs.dat", "r");
	
	int key = dispersion(nombre);
	
	char path[16];
	sprintf(path, "%s%d%s", "./hash/", key, ".dat");
	hash = fopen(path, "r");	
	
	Dog *auxDog;
	auxDog = malloc(sizeof(Dog));

	int posPrimero = 0;
	int result = fread(&posPrimero, sizeof(int), 1, hash);

	int counter = 0;
	do{
		fseek(data, sizeof(Dog) * posPrimero, SEEK_SET);
		fread(auxDog, sizeof(Dog), 1, data);
		if(strcmp(nombre, auxDog->nombre) == 0){
			r = send(socket_desc, &posPrimero, sizeof(int), 0);
			if(r == -1){
				perror("Error al enviar la estructura.");
				close(socket_desc);
				exit(-1);
			}
			r = send(socket_desc, auxDog, sizeof(Dog), 0);
			if(r == -1){
				perror("Error al enviar la estructura.");
				close(socket_desc);
				exit(-1);
			}

			counter++;
		}

		posPrimero = auxDog->nextDog;
	}while(posPrimero > -1);

	r = send(socket_desc, &posPrimero, sizeof(int), 0);
	if(r == -1){
		perror("Error al enviar la bandera.");
		close(socket_desc);
		exit(-1);
	}

	r = send(socket_desc, auxDog, sizeof(Dog), 0);
	if(r == -1){
		perror("Error al enviar la bandera.");
		close(socket_desc);
		exit(-1);
	}

	r = send(socket_desc, &counter, sizeof(int), 0);
	if(r == -1){
		perror("Error al enviar el contador.");
		close(socket_desc);
		exit(-1);
	}

	free(auxDog);

	fclose(hash);
	fclose(data);

	char ip[20];
	get_ip(socket_desc, ip);
	update_log("Busqueda", nombre, ip);
}

/*Obtiene la fecha actual del sistema.*/
void get_date(char date[26]){
	time_t timer;
	struct tm* tm_info;
	time(&timer);
	tm_info = localtime(&timer);

	strftime(date, 26, "%Y-%m-%d %H:%M:%S ", tm_info);
}

/*Funcion que ira guardando la accion que ha realizado los clientes, en el archivo serverDogs.log*/
void update_log(char *accion, char *perro, char *ip){
	FILE *log;
	
	char date[26];
	get_date(date);


	log = fopen("serverDogs.log", "a+");
	if(log == NULL){
		log = fopen("serverDogs.log", "w+a");
		if(log == NULL){
			perror("No se ha podido abrir el archivo serverDogs.");
			exit(-1);
		}
	}
	fputs("Fecha: ", log);
	fputs(date, log);
	fputs("|", log);
	fputs("Cliente: ", log);
	fputs(ip, log);
	fputs(" | ", log);
	fputs(accion, log);
	fputs(": ", log);
	fputs(perro, log);
	fputs("\n", log);
	fclose(log);
}

/*Obtiene la ip del cliente asociada a un descriptor.*/
void get_ip(int descriptor, char ip[20]){
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    //Esa funcion retorna la ip del cliente asociada a ese descriptor
    int res = getpeername(descriptor, (struct sockaddr *)&addr, &addr_size);
    strcpy(ip, inet_ntoa(addr.sin_addr)); // convierte la ip dada en bytes a string
}

/*Esta funcion es la mas importante del programa, ya que es la funcion que va a ejecutar cada
hilo, y en la cual va a estar manejando las diferentes acciones que realice cada cliente. Accion 
que estara constantemente reciviendo del cliente.*/

pthread_mutex_t mutex;
pthread_mutex_t mutex2;
pthread_mutex_t mutex3;
pthread_mutex_t mutex4;


int pipe_server[2];
	int auxiliar; //Si es -1 es que el valor del semaforo fue 0.  
	int auxiliar2; //Si es 0 el mutex sí pudo bloquear

void *control_func(void *socket_desc){
	int opcion, r;
	int cliente_sock = *(int *)socket_desc;
	char testigo;
	while(1){
		r = recv(cliente_sock, &opcion, sizeof(int), 0);
		if(r == -1){
			perror("Error al recibir la operación.");
			close(cliente_sock);
			exit(-1);
		}

		switch(opcion){
			case 1:			   
			    //read(pipe_server[0], &testigo, 1);			
				//pthread_mutex_lock(&mutex);
				sem_wait(semaforo);
				//sem_wait(semaforo4);
				ingresar(cliente_sock);
				sem_post(semaforo);
				//sem_post(semaforo4);
				//pthread_mutex_unlock(&mutex);
				//write(pipe_server[1], "T", 1);
				break;
			case 2:
			    //read(pipe_server[0], &testigo, 1);			
				//pthread_mutex_lock(&mutex2);
				//sem_wait(semaforo2);
				ver(cliente_sock);
				//sem_post(semaforo2);
				//pthread_mutex_unlock(&mutex2);
				//write(pipe_server[1], "T", 1);
				break;
			case 3:
			    //read(pipe_server[0], &testigo, 1);	
			    //pthread_mutex_lock(&mutex);
				//pthread_mutex_lock(&mutex2);
				//pthread_mutex_lock(&mutex3);
				sem_wait(semaforo);
				//sem_wait(semaforo2);
				//sem_wait(semaforo3);		
				borrar(cliente_sock);
				sem_post(semaforo);
				//sem_post(semaforo2);
				//sem_post(semaforo3);
				//pthread_mutex_unlock(&mutex);
				//pthread_mutex_unlock(&mutex2);
				//pthread_mutex_unlock(&mutex3);
				//write(pipe_server[1], "T", 1);
				break;
			case 4:
			    //read(pipe_server[0], &testigo, 1);			
				//pthread_mutex_lock(&mutex3);
				//auxiliar2=pthread_mutex_trylock(&mutex4);
				//sem_wait(semaforo3);
				/*auxiliar=sem_trywait(semaforo4);
				buscar(cliente_sock);
				sem_post(semaforo3);
				if(auxiliar!=-1){
					sem_post(semaforo4);
				}
				*/
				//pthread_mutex_unlock(&mutex3);
				//if(auxiliar2==0){
				//	pthread_mutex_unlock(&mutex4);
				//}				
				//write(pipe_server[1], "T", 1);
				break;
			case 5:
				close(cliente_sock);
				break;
		}
		if(opcion == 5){
			break;
		}
	}
}

/*Se configura el socket del servidor, y los hilos para que ejecute de manera paralela los diferentes
clientes que entran en conexión*/
int main(){


	int serverfd, r, clientfd, i = 0;
	struct sockaddr_in server, client;
	size_t size = sizeof(struct sockaddr_in);
	size_t sizecli = 0;
	pthread_t thread[i]; 

	serverfd = socket(AF_INET, SOCK_STREAM, 0);
	if(serverfd == -1){
		perror("Error al crear el socket.");
		exit(-1);
	}

	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr = INADDR_ANY; // va a recibir cualquier direccion
	bzero(server.sin_zero, 0);

	r = bind(serverfd, (struct sockaddr *)&server, size);

	if(r == -1){
		perror("Error al hacer bind.");
		exit(-1);
	}
	r = listen(serverfd, BACKLOOG);

	if(r == -1){
		perror("Error al hacer listen");
		exit(-1);
	}
	sem_unlink("semaforo_ing_borrar");
	sem_unlink("semaforo_ver_borrar");
	sem_unlink("semaforo_bus_borrar");
	sem_unlink("semaforo_bus_ing");
	semaforo = sem_open("semaforo_ing_borrar", O_CREAT, 0700, MAX_PROCESOS); 
	semaforo2 = sem_open("semaforo_ver_borrar", O_CREAT, 0700, MAX_PROCESOS); 
	semaforo3 = sem_open("semaforo_bus_borrar", O_CREAT, 0700, MAX_PROCESOS); 
	semaforo4 = sem_open("semaforo_bus_ing", O_CREAT, 0700, MAX_PROCESOS); 


	
	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_init(&mutex2, NULL);
	pthread_mutex_init(&mutex3, NULL);
	pthread_mutex_init(&mutex4, NULL);

	pipe(pipe_server);
	write(pipe_server[1], "T", 1);

	printf("Servidor ejecutandose...\n");
	while(clientfd = accept(serverfd, (struct sockaddr *)&client, (socklen_t *)&sizecli)){
		count_clientes++;
		pthread_t thread_ind;
		int *new;
		new = malloc(1);
		*new = clientfd;
		r = pthread_create(&thread[i], NULL,(void *)control_func, (void *)new);
		if(r == -1){
			perror("Error al crear el hilo.");
			exit(-1);
		}
	}	

    sem_close(semaforo);
    sem_close(semaforo2);
    sem_close(semaforo3);
    sem_close(semaforo4);

	sem_unlink("semaforo_ing_borrar");
	sem_unlink("semaforo_ver_borrar");
	sem_unlink("semaforo_bus_borrar");
	sem_unlink("semaforo_bus_ing");

	pthread_mutex_destroy(&mutex);
	pthread_mutex_destroy(&mutex2);
	pthread_mutex_destroy(&mutex3);
	pthread_mutex_destroy(&mutex4);


	close(clientfd);
	close(serverfd);
}