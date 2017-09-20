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
#include <unistd.h>


/* ---- ṔRESIONE CUALQUIER TECLA PARA CONTINUAR ---- */
static struct termios old, new;

/* Initialize new terminal i/o settings */
void initTermios(int echo){
  tcgetattr(0, &old); /* grab old terminal i/o settings */
  new = old; /* make new settings same as old settings */
  new.c_lflag &= ~ICANON; /* disable buffered i/o */
  new.c_lflag &= echo ? ECHO : ~ECHO; /* set echo mode */
  tcsetattr(0, TCSANOW, &new); /* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void resetTermios(void) {
  tcsetattr(0, TCSANOW, &old);
}

/* Read 1 character - echo defines echo mode */
char getch_(int echo) {
  char ch;
  initTermios(echo);
  ch = getchar();
  resetTermios();
  return ch;
}

/* Read 1 character without echo */
char getch(void) {
  return getch_(0);
}

/* Read 1 character with echo */
char getche(void) {
  return getch_(1);
}

/* ------------------------------------------------------*/

#define TABLE_SIZE 1000
#define PORT 3580
int clientfd;
char *clientip;

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

void menu();
void ingresar();
void insertar(Dog dog_insertar);
void ver();
void borrar();
void buscar();
int n_registros();
void menu_ver_registro(int registro);
void historia_clinica(int registro);
void print_dog(int registro, Dog *dog);
void open_historia(char path[]);

/*Funcion para imprimir el MENU PRINCIPAL */
void menu(){
	printf("\n----------- MENU PRINCIPAL -----------\n");
	printf("| *Por favor seleccione una opción*  |\n");
	printf("| 1. Ingresar registro.              |\n");
	printf("| 2. Ver registro.                   |\n");
	printf("| 3. Borrar registro.                |\n");
	printf("| 4. Buscar registro.                |\n");
	printf("| 5. Salir.                          |\n");
	printf("--------------------------------------\n");
}

/* Funcion para leer los datos del perro a ingresar*/
void ingresar(){
	char nombre[32];
	char tipo[32];
	char raza[16];
	int edad;
	int estatura;
	int peso;
	char genero;

	printf("A continuación inserte los datos del nuevo registro:\n");

	printf("Nombre: ");
	scanf(" %s", nombre);
	printf("Tipo de animal: ");
	scanf(" %s", tipo);
	printf("Raza: ");
	scanf(" %s", raza);
	printf("Edad: ");
	scanf(" %d", &edad);		
	printf("Estatura (centimetros): ");
	scanf(" %d", &estatura);
	printf("Peso: ");
	scanf(" %d", &peso);	
	printf("Genero (H/M): ");
	while(1){
		scanf(" %c", &genero);
		if(genero == 'H' || genero == 'M')
			break;
		else
			printf("El genero debe ser el caracter 'H' o 'M': ");
		
	}

	Dog dog;
	strcpy(dog.nombre, nombre);
	strcpy(dog.tipo, tipo);
	strcpy(dog.raza, raza);

	dog.edad = edad;
	dog.estatura = estatura;
	dog.peso = peso;
	dog.genero = genero;
	dog.nextDog = -1;
	dog.backDog = -1;
	
	insertar(dog);
	char flag;
	recv(clientfd, &flag, sizeof(char), 0);


	printf("\n");
}

/*Funcion para enviale al servidor la estructura del perro que ha sido ingresada. */
void insertar(Dog dog_insertar){
	//El cliente envia la estructura del perro que quiere insertar)
	int r = send(clientfd, &dog_insertar, sizeof(Dog), 0);
	if(r == -1){
		perror("No se envio la estructura completa.");
	}
}

/*Funcion para recibir el numero de perros que hay actualmente. */
int n_registros(){
	int registros;
	int r = recv(clientfd, &registros, sizeof(int), 0);
	if(r != sizeof(int)){
		perror("No se recibieron todos los datos.");
		close(clientfd);
		exit(-1);
	}
	return registros;
}

/*Menu para las opciones de ver el registro. */
void menu_ver_registro(int registro){
	printf("\n-------- MENÚ VER REGISTRO #%d -------\n", registro);
	printf("|  *Por favor seleccione una opción*     |\n");
	printf("|  1. Ver historia clínica.              |\n");
	printf("|  2. Volver al menú principal.          |\n");
	printf("------------------------------------------\n");
}

/*Funcion que abre la historia del perro a ver. */
void open_historia(char path[]){
	char nano[40];
	sprintf(nano, "%s%s", "nano ", path);
	system(nano);
}

void historia_clinica(int registro){
	Dog auxDog;
	char texto[4096];
	int r = recv(clientfd, &auxDog, sizeof(Dog), 0);
	if(r != sizeof(Dog)){
		perror("No se enviaron todos los datos.");
	}

	FILE *historia;
	
	char path[35];
	sprintf(path, "%d%s", registro + 1, ".txt");
	
	historia = fopen(path, "w+");
	if(historia == NULL){
		perror("No se ha podido abrir la historia clinica.");
		exit(-1);
	}
	fputs("*********************** HISTORIA CLÍNICA *************************\n", historia);
	fputs("Nombre del animal: ", historia);
	fputs(auxDog.nombre, historia);
	fputs("\nTipo de animal: ", historia);
	fputs(auxDog.tipo, historia);
	fputs("\nRaza del animal: ", historia);
	fputs(auxDog.raza, historia);
	fputs("\nEdad: ", historia);
	char aux[4];
	sprintf(aux, "%d", auxDog.edad);
	fputs(aux, historia);
	fputs("\nEstatura: ", historia);
	sprintf(aux, "%d", auxDog.estatura);
	fputs(aux, historia);
	fputs("\nPeso: ", historia);
	sprintf(aux, "%d", auxDog.peso);
	fputs(aux, historia);
	fputs("\nGénero: ", historia);
	char genero[6];
	if(auxDog.genero == 'H'){
		strcpy(genero, "Hembra");
	}else{
		strcpy(genero, "Macho");
	}
	fputs(genero, historia);
	fputs("\n", historia);
	do {
		r = recv(clientfd, texto, sizeof(texto), 0);
		if(r != sizeof(texto)){
			perror("No se enviaron todos los datos.");
		}
		fputs(texto, historia);
	}while(texto[0] != '\0');
	fclose(historia);
	
	open_historia(path);

	historia = fopen(path, "r");
	int i;
	for(i = 0; i < 8; i++){
		fgets(texto, 4096, historia);
	}

	while(fgets(texto, 4096, historia) != NULL){
		r = send(clientfd, texto, sizeof(texto), 0);
	}
	texto[0] = '\0';	
	r = send(clientfd, texto, sizeof(texto), 0);

	fclose(historia);

	remove(path);
}
/*Funcion que pide al cliente un numero de registro valido, luego le envia ese numero de registro
al servidor y luego recibe la estructura del perro para abrirla en el archivo.*/
void ver(){	

	int numero_regs = n_registros();
	int registro, r;
	printf("Número de registros presentes: %d\n", numero_regs);
	printf("Número de registro que quiere ver: ");
	while(1){
		scanf(" %d", &registro);
		printf("\n");

		if(registro > numero_regs || registro <= 0){
			printf("El número de registro ingresado no existe\n");
			printf("Por favor ingrese un nuevo número de registro: ");
		} else{
			break;
		}
	}
	menu_ver_registro(registro);
	int opcion = 0;
	while(1){
		scanf(" %d", &opcion);
		if(opcion != 1 && opcion != 2){
			printf("Por favor escriba una opción valida: ");
		}else{
			break;
		}
	}

	switch(opcion){
		case 1:
			r = send(clientfd, &registro, sizeof(int), 0);
			if(r != sizeof(int)){
				perror("No se enviaron todos los datos.");
			}		
			
			historia_clinica(registro - 1);
			break;
		case 2:
			printf("\n");
			break;
	}

}

/*Funcion que le pide al cliente un numero de registro valido a borrar, luego se lo envía al
servidor.*/
void borrar(){
	int borrado;

	//   MENU
	int numero_regs = n_registros();
	printf("Numero de registros presentes: %i\n", numero_regs);
	printf("Numero de registro que quiere borrar: ");
	while(1){
		scanf(" %d", &borrado);
		printf("\n");

		if(borrado > numero_regs || borrado <= 0){
			printf("El número de registro ingresado no existe\n");
			printf("Por favor ingrese un nuevo número de registro: ");
		} else{
			break;
		}
	}

	int r = send(clientfd, &borrado, sizeof(int), 0);
	if(r != sizeof(int)){
		perror("No se enviaron todos los datos.");
	}
	printf("\n Espere un momento... \n");

	char flag;
	recv(clientfd, &flag, sizeof(char), 0);
}

/*Funcion que imprime en consola los datos del perro que se quiere imprimir.*/
void print_dog(int registro, Dog *dog){
	char genero[6];
	if(dog->genero == 'H')
		strcpy(genero, "Hembra");
	else
		strcpy(genero, "Macho");
	printf("|    %d       %s     %s   %s     %d        %d         %d       %s   \n", 
		++registro, dog->nombre, dog->tipo, dog->raza, dog->edad, dog->estatura, dog->peso, genero);
}

/*Funcion que le envia el nombre del perro a buscar, luego estara recibiendo uno por uno los perros
encontrados con ese nombre.*/
void buscar(){
	char flag;
	recv(clientfd, &flag, sizeof(char), 0);

	Dog dogaux;
	char nombre[32];
	int registro, r;
	printf("\nBuscar por el nombre: ");
	scanf(" %s", nombre);

	r = send(clientfd, nombre, sizeof(nombre), 0);
	if(r != sizeof(nombre)){
		perror("No se enviaron todos los datos.");
	}

	printf("\n--- #REGISTRO --- NOMBRE --- TIPO --- RAZA --- EDAD --- ESTATURA --- PESO --- GENERO ---\n");

	while(1){
		r = recv(clientfd, &registro, sizeof(int), 0);
		if(r != sizeof(int)){
			perror("No se retcibieron todos los datos.");
			close(clientfd);
			exit(-1);
		}
		r = recv(clientfd, &dogaux, sizeof(Dog), 0);
		if(r != sizeof(Dog)){
			perror("No se retcibieron todos los datos.");
			close(clientfd);
			exit(-1);
		}

		if(registro == -1){
			break;
		}else{
			print_dog(registro, &dogaux);
		}	
	}

	int contador;
	r = recv(clientfd, &contador, sizeof(int), 0);
	if(r != sizeof(int)){
		perror("No se recibieron todos los datos.");
		close(clientfd);
		exit(-1);
	}

	if(contador == 0)
		printf("No se han encontrado registros con la palabra %s\n", nombre);
	else{
		printf("--- #REGISTRO --- NOMBRE --- TIPO --- RAZA --- EDAD --- ESTATURA --- PESO --- GENERO ---\n");
		printf("\n");
		printf("Se encontró un total de %d registros que coinciden con '%s'.\n",contador, nombre);
	}
	printf("\n");

}

/*Funcion que configura el socket del cliente.*/
void conf_socket(char *ip){
	int r;
	struct sockaddr_in client;
	socklen_t size;
	clientip = ip;
	/*crea un socket para la comunicacion y retorna el descriptor del socket
	AF_INET - dominio de comunicacion, selecciona el protocolo, en ese caso es IPV4
	SOCK_STREAM - Para hacer el transporte por TCP
	*/
	clientfd = socket(AF_INET, SOCK_STREAM, 0);

	if(clientfd == -1){
		perror("Error al crear el socket.");
		exit(-1);
	}
	//configuracion del socket del cliente
	client.sin_family = AF_INET;
	client.sin_port = htons(PORT); //Hace cambio de endian PORT:3535
	client.sin_addr.s_addr = inet_addr(ip);// inet_addr convierte un string de la ip a binario
	bzero(client.sin_zero, 8); //llena de ceros ese campo de la estructura

	size = sizeof(client);
	/*Conecta el socket identificado con clientfd a la direccion especificada en client*/
	r = connect(clientfd, (struct sockaddr *)&client, size);

	if(r == -1){
		perror("Error conectando al servidor. Número maximo de clientes alcanzado");
		exit(-1);
	}
}

int main(int argc, char *argv[]){
	conf_socket(argv[1]);

	int opcion, r;
	char cualquier_tecla;
	do{
		opcion = 0;
		menu();
		scanf(" %d", &opcion);
		send(clientfd, &opcion, sizeof(int), 0);
		
		switch (opcion){
			case 1:
				ingresar();
				printf("Ingreso exitoso, Presione cualquier tecla para continuar... \n");
				getchar();
				cualquier_tecla = getch();
				break;
			case 2:
				ver();
				printf("Operación exitosa, Presione cualquier tecla para continuar... \n");
				getchar();
				cualquier_tecla = getch();
				break;
			case 3:
				borrar();
				printf("Borrado exitoso, Presione cualquier tecla para continuar... \n");
				getchar();
				cualquier_tecla = getch();
				break;
			case 4:
				buscar();
				printf("Busqueda exitosa, Presione cualquier tecla para continuar... \n");
				getchar();
				cualquier_tecla = getch();
				break;
			case 5:
				close(clientfd);
				getchar();
				printf("Presione cualquier tecla para continuar... \n");
				cualquier_tecla = getch();
				break;
		}
	}while(opcion != 5);
}
