#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <windows.h>
#include <semaphore.h>
#include <pthread.h>

#define AFORO_MAX 12
#define N_CIRCUITOS 4
#define N_VISITANTES 25

HANDLE sem_escritura;
HANDLE sem_parque;
HANDLE sem_salida;


typedef struct
{
	int id;
	int tiempo_circuito;
	int circuito; //(N_CIRCUITOS)

}TPARAMHILO;


int aleatorio(int min, int max) 
{
   if(min == max) 
   {
      return min;
   }
   else if (min > max) 
   {
      return -1;
   }
   return ((rand() % (max - min + 1)) + min);
}


DWORD f_visitantes(LPVOID ptr)
{
	TPARAMHILO visitante;
	visitante = *(TPARAMHILO*)ptr;


	WaitForSingleObject(sem_escritura, INFINITE);
	printf("El visitante %d esta en la puerta, va al circuito %d y va a estar %d minutos\n", visitante.id, visitante.circuito, visitante.tiempo_circuito);
	ReleaseSemaphore(sem_escritura, 1, NULL);

	WaitForSingleObject(sem_parque, INFINITE);
	WaitForSingleObject(sem_escritura, INFINITE);
	printf("El visitante %d ha entrado al parque\n", visitante.id);
	ReleaseSemaphore(sem_escritura, 1, NULL);

	WaitForSingleObject(sem_escritura, INFINITE);
	printf("El visitante %d ha entrado al circuito %d\n", visitante.id, visitante.circuito);
	ReleaseSemaphore(sem_escritura, 1, NULL);
	sleep(visitante.tiempo_circuito);

	WaitForSingleObject(sem_escritura, INFINITE);
	printf("El visitante %d va a salir\n", visitante.id);
	ReleaseSemaphore(sem_escritura, 1, NULL);
	WaitForSingleObject(sem_salida, INFINITE);
	ReleaseSemaphore(sem_salida, 1, NULL);
	WaitForSingleObject(sem_escritura, INFINITE);
	printf("El visitante %d ha salido del parque\n", visitante.id);
	ReleaseSemaphore(sem_escritura, 1, NULL);

	return EXIT_SUCCESS;
}

DWORD f_vigilante(LPVOID ptr)
{
	sleep(3);
	WaitForSingleObject(sem_escritura, INFINITE);
	printf("El vigilante abre para la tercera parte del aforo\n");
	ReleaseSemaphore(sem_escritura, 1 , NULL);
	ReleaseSemaphore(sem_parque, AFORO_MAX/3, NULL);
	sleep(3);
	WaitForSingleObject(sem_escritura, INFINITE);
	printf("El vigilante permite aforo completo\n");
	ReleaseSemaphore(sem_escritura, 1, NULL);
	ReleaseSemaphore(sem_parque, 2*AFORO_MAX/3, NULL);


	return EXIT_SUCCESS;
}


int main(int argc, char** argv)
{
	TPARAMHILO param[N_VISITANTES];
	int i = 0;
	HANDLE hilo_visitante[N_VISITANTES];
	HANDLE hilo_vigilante;
	DWORD wait, close;

	//CREACION DE LOS SEMAFOROS
	sem_escritura = CreateSemaphore(NULL, 1, 1, NULL);
	if (sem_escritura == NULL)
	{
		printf("ERROR %d en la creacion del semaforo sem_escritura\n", GetLastError());
		return EXIT_FAILURE;
	}

	sem_parque = CreateSemaphore(NULL, 0, AFORO_MAX, NULL);
	if (sem_parque == NULL)
	{
		printf("ERROR %d en la creacion del semaforo sem_parque\n", GetLastError());
		return EXIT_FAILURE;
	}

	sem_salida = CreateSemaphore(NULL, 1, 1, NULL);
	if (sem_salida == NULL)
	{
		printf("ERROR %d al crear el semaforo sem_salida\n", GetLastError());
		return EXIT_FAILURE;
	}

	//CREACION DE LOS HILOS
	for (i = 0; i < N_VISITANTES; i++)
	{
		param[i].id = i+1;
		param[i].tiempo_circuito = aleatorio(3,4);
		param[i].circuito = aleatorio(1,4);
		hilo_visitante[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)f_visitantes, &param[i], 0, NULL);
		if (hilo_visitante[i] == NULL)
		{
			WaitForSingleObject(sem_escritura, INFINITE);
			printf("ERROR %d en la creacion del hilo hilo_visitante[%d]\n", GetLastError(), i);
			ReleaseSemaphore(sem_escritura, 1, NULL);
			return EXIT_FAILURE;
		}
	}

	hilo_vigilante = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)f_vigilante, NULL, 0, NULL);
	if (hilo_vigilante == NULL)
	{
		WaitForSingleObject(sem_escritura, INFINITE);
		printf("ERROR %d en la creacion del hilo hilo_vigilante\n", GetLastError());
		ReleaseSemaphore(sem_escritura, 1, NULL);
		return EXIT_FAILURE;
	}


	//ESPERA DE LOS HILOS
	wait = WaitForMultipleObjects(N_VISITANTES, hilo_visitante, TRUE, INFINITE);
	if (wait == WAIT_FAILED)
	{
		WaitForSingleObject(sem_escritura, INFINITE);
		printf("ERROR %d en la espera de los hilos hilo_visitante\n", GetLastError());
		ReleaseSemaphore(sem_escritura, 1, NULL);
		return EXIT_FAILURE;
	}

	wait = WaitForSingleObject(hilo_vigilante, INFINITE);
	if (wait == WAIT_FAILED)
	{
		WaitForSingleObject(sem_escritura, INFINITE);
		printf("ERROR %d en la espera del hilo hilo_vigilante\n", GetLastError());
		ReleaseSemaphore(sem_escritura, 1, NULL);
		return EXIT_FAILURE;
	}

	WaitForSingleObject(sem_escritura,INFINITE);
    printf("TODOS LOS VISITANTES HAN SALIDO\n");
    ReleaseSemaphore(sem_escritura, 1, NULL);

	//CIERRE DE LOS HILOS
	for (i = 0; i < N_VISITANTES; i++)
	{
		close = CloseHandle(hilo_visitante[i]);
		if (close == FALSE)
		{
			printf("ERROR %d en el cierre del hilo hilo_visitante[%d]\n", GetLastError(), i);
			return EXIT_FAILURE;
		}
	}

	close = CloseHandle(hilo_vigilante);
	if (close == FALSE)
	{
		printf("ERROR %d en el cierre del hilo hilo_vigilante\n", GetLastError());
		return EXIT_FAILURE;
	}

	//CIERRE DE LOS SEMAFOROS
	CloseHandle(sem_escritura);
	CloseHandle(sem_parque);
	CloseHandle(sem_salida);

	return EXIT_SUCCESS;
}