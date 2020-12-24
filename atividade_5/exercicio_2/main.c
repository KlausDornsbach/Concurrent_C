#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>

int produzir(int value);    //< definida em helper.c
void consumir(int produto); //< definida em helper.c
void *produtor_func(void *arg);
void *consumidor_func(void *arg);

int n_produtores;
int indice_produtor, indice_consumidor, tamanho_buffer;
int* buffer;
sem_t semaforo_enche;
sem_t semaforo_esvazia;

pthread_mutex_t mutex_consome;
pthread_mutex_t mutex_produz;

//Você deve fazer as alterações necessárias nesta função e na função
//consumidor_func para que usem semáforos para coordenar a produção
//e consumo de elementos do buffer.
void *produtor_func(void *arg) {
    //arg contem o número de itens a serem produzidos
    int max = *((int*)arg);
    for (int i = 0; i <= max; ++i) {
    sem_wait(&semaforo_enche);
        int produto;
        if (i == max){
            produto = -1;
        }else{
           produto = produzir(i); //produz um elemento normal
        }
        pthread_mutex_lock(&mutex_produz);
        indice_produtor = (indice_produtor + 1) % tamanho_buffer; //calcula posição próximo elemento
        buffer[indice_produtor] = produto; //adiciona o elemento produzido à lista
        pthread_mutex_unlock(&mutex_produz);
        sem_post(&semaforo_esvazia);
    }
    return NULL;
}
void *consumidor_func(void *arg) {
    int val;
    while (1) {
        sem_getvalue(&semaforo_enche, &val);
        sem_wait(&semaforo_esvazia);
        pthread_mutex_lock(&mutex_consome);
        indice_consumidor = (indice_consumidor + 1) % tamanho_buffer; //Calcula o próximo item a consumir
        int produto = buffer[indice_consumidor]; //obtém o item da lista
        pthread_mutex_unlock(&mutex_consome);
        //Podemos receber um produto normal ou um produto especial
        if (produto >= 0){
            consumir(produto); //Consome o item obtido.
            sem_post(&semaforo_enche);
        }else{
            sem_post(&semaforo_enche);
            break; //produto < 0 é um sinal de que o consumidor deve parar
        }
        sem_getvalue(&semaforo_enche, &val);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        printf("Uso: %s tamanho_buffer itens_produzidos n_produtores n_consumidores \n", argv[0]);
        return 0;
    }

    tamanho_buffer = atoi(argv[1]);
    int itens = atoi(argv[2]);
    int n_produtores = atoi(argv[3]);
    int n_consumidores = atoi(argv[4]);
    printf("itens=%d, n_produtores=%d, n_consumidores=%d\n",
	   itens, n_produtores, n_consumidores);

    //Iniciando buffer
    indice_produtor = 0;
    indice_consumidor = 0;
    buffer = malloc(sizeof(int) * tamanho_buffer);

    pthread_mutex_init(&mutex_produz, NULL);
    pthread_mutex_init(&mutex_consome, NULL);
    //pthread_mutex_init(&mutex_produz_acabaram, NULL);
    // Crie threads e o que mais for necessário para que n_produtores
    // threads criem cada uma n_itens produtos e o n_consumidores os
    // consumam.

    pthread_t prod[n_produtores];
    pthread_t cons[n_consumidores];
    sem_init(&semaforo_esvazia, 0, 0); //inicializo o semaforo em 0 e seto segundo argumento para 0 pois ele será
    sem_init(&semaforo_enche, 0, tamanho_buffer); //compartilhado entre threads
    for(int i = 0; i<n_produtores; i++){// crio threads
        prod[i]= i;
        pthread_create(&prod[i], NULL, produtor_func, &itens);
    }
    for(int i = 0; i<n_consumidores; i++){
        cons[i] = i;
        pthread_create(&cons[i], NULL, consumidor_func, &itens);
    }

    for(int i = 0; i<n_produtores; i++){//dou join nas threads
        pthread_join(prod[i], NULL);
    }
    for(int i = 0; i<n_consumidores; i++){
        pthread_join(cons[i], NULL);
    }
    sem_destroy(&semaforo_esvazia);
    sem_destroy(&semaforo_enche);

    pthread_mutex_destroy(&mutex_consome);
    pthread_mutex_destroy(&mutex_produz);
    //pthread_mutex_destroy(&mutex_produz_acabaram);
    //Libera memória do buffer
    free(buffer);

    return 0;
}
