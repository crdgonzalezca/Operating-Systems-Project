#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TABLE_SIZE 1000
#define MAX_REGISTROS 10000000

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


int dispersion(char nombre[]){
	
	int hash = 7;
	int i  = 0;


	while(1) {
	    if(nombre[i] == '\0' || nombre[i] == '\n'){
	    	break;
	    }

	    
	    if(nombre[i] >= 65 && nombre[i] <= 90){
	    	nombre[i] = nombre[i] + 32;
	    }
	    hash = hash * 31 + nombre[i];
	    i++;
	}	

	if(hash < 0){
		hash *= -1;
	}
	return hash % TABLE_SIZE;
}

void generate_hash(){
	FILE *ap;
	for(int i = 0; i < 1000; i++){
		char dat[] = ".dat";
		char path[16];
		sprintf(path,"%s%d%s","./hash/",i,dat);
		ap = fopen(path, "w+b");
		fclose(ap);
	}
}

int insertar(int posUltimo, Dog *dog, int registro, FILE *data){
	Dog aux;
	if(posUltimo == -1){
		fseek(data, sizeof(Dog) * registro, SEEK_SET);
		fwrite(dog, sizeof(Dog), 1, data);
		
	} else {
		fseek(data, sizeof(Dog) * posUltimo, SEEK_SET);
		
		fread(&aux, sizeof(Dog), 1, data);

		aux.nextDog = registro;
		fseek(data, sizeof(Dog) * posUltimo, SEEK_SET);
		fwrite(&aux, sizeof(Dog), 1, data);

		dog->backDog = posUltimo;
		fseek(data, sizeof(Dog) * registro, SEEK_SET);		
		fwrite(dog, sizeof(Dog), 1, data);
	}
}

Dog generate_dog(char nombre[]){
	Dog dogAux;

	char tipo[6];
	char raza[6];

	for(int i = 0; i < 6; i++){
		tipo[i] = 'a' + (rand() % 26);
	}

	for(int i = 0; i < 6; i++){
		raza[i] = 'a' + (rand() % 26);
	}

	int edad = rand() % 200;
	int estatura = rand() % 200;
	int peso = rand() % 100;

	char generos[2] = {'M', 'H'};

	char genero = generos[rand() % 2];
	
	strcpy(dogAux.nombre, nombre);
	strcpy(dogAux.tipo, tipo);
	strcpy(dogAux.raza, raza);
	dogAux.edad = edad;
	dogAux.estatura = estatura;
	dogAux.peso = peso;
	dogAux.nextDog = -1;
	dogAux.backDog = -1;
	dogAux.genero = genero;

	return dogAux;
}

void format_nombre(char nombre[], char ptr[]){
	int i = 0;
	char aux[32];
	while(1){
		if(nombre[i] == '\n'){
			ptr[i] = '\0';
			break;			
		}
		ptr[i] = nombre[i];
		i++;
	}
}

int main(){

	generate_hash();

	FILE *data;
	data = fopen("dataDogs.dat", "w+");
	char linea[32];

	for(int i = 0; i < MAX_REGISTROS;){
		
		FILE *file;
		file = fopen("nombres.txt", "r");
		if(file == NULL){
			perror("No se ha podido abrir el archivo");
			exit(-1);
		}
		
		while (fgets(linea, 32, file) != NULL && i < MAX_REGISTROS){	
			char nombre[32];
			format_nombre(linea, nombre);
			int hash = dispersion(nombre);

			char path[16];
			sprintf(path, "%s%d%s", "./hash/", hash, ".dat");
			
			int posPrimero;
			int posUltimo;

			FILE *ap;
			ap = fopen(path, "r+");	
			int result = fread(&posPrimero, sizeof(int), 1, ap);
		
			Dog *dogAux;
			dogAux = malloc(sizeof(Dog));

			*dogAux = generate_dog(nombre); 

			if(result == 1 ){
				fseek(ap, sizeof(int), SEEK_SET);				
				fread(&posUltimo, sizeof(int), 1, ap);				
				
				insertar(posUltimo, dogAux, i, data);

				posUltimo = i;
				fseek(ap, sizeof(int), SEEK_SET);
				fwrite(&posUltimo, sizeof(int), 1, ap);
				
			} else {
				posPrimero = i;
				posUltimo = i;
				fwrite(&posPrimero, sizeof(int), 1, ap);
				fwrite(&posUltimo, sizeof(int), 1, ap);
				
				insertar(-1, dogAux, i, data);

			}

			free(dogAux);
			fclose(ap);
			

			i++;	
			
	    }	
	    fclose(file);

	}

	fclose(data);
	printf("tiempo: %.5le\n", (double)clock() / CLOCKS_PER_SEC);
	
	/*
	FILE *data;
	data = fopen("dataDogs.dat", "r+");

	int posPrimero = 282;
	Dog aux;
	do{
		fseek(data, sizeof(Dog) * posPrimero, SEEK_SET);
		fread(&aux, sizeof(Dog), 1, data);
		printf("Reg: %d Nomre:%s\n", posPrimero, aux.nombre);
		printf("back: %d next:%d\n", aux.backDog, aux.nextDog);

		posPrimero = aux.nextDog;


	}while(posPrimero != -1);
*/
/*
			int hash = dispersion("cristian");

			char path[16];
			sprintf(path, "%s%d%s", "./hash/", hash, ".dat");
			
			int posPrimero;
			int posUltimo;

			FILE *ap;
			ap = fopen(path, "r+");	
			fread(&posPrimero, sizeof(int), 1, ap);
			fread(&posUltimo, sizeof(int), 1, ap);

			printf("posPrimero%d\n", posPrimero);
			printf("posUltimo%d\n", posUltimo);

	FILE *data;
	data = fopen("dataDogs.dat", "r+");

	Dog aux;
	int pos = 1739;
	do{
		fseek(data, sizeof(Dog) * pos, SEEK_SET);
		fread(&aux, sizeof(Dog), 1, data);
		printf("nombre %s back %d next %d\n", aux.nombre, aux.backDog, aux.nextDog);
		pos = aux.backDog;
	}while(aux.backDog != -1);*/
	
	return 0;
}