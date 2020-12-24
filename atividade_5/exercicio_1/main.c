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
sem_t semaforo_enche;
sem_t semaforo_esvazia;

int indice_produtor, indice_consumidor, tamanho_buffer;
int* buffer;

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
            produto = -1;          //envia produto sinlizando FIM
        }else{
           produto = produzir(i); //produz um elemento normal
        }
      indice_produtor = (indice_produtor + 1) % tamanho_buffer; //calcula posição próximo elemento
        buffer[indice_produtor] = produto; //adiciona o elemento produzido à lista
        sem_post(&semaforo_esvazia);
    }
    return NULL;
}

void *consumidor_func(void *arg) {
    while (1) {
    sem_wait(&semaforo_esvazia);
        indice_consumidor = (indice_consumidor + 1) % tamanho_buffer; //Calcula o próximo item a consumir
        int produto = buffer[indice_consumidor]; //obtém o item da lista
        //Podemos receber um produto normal ou um produto especial
        if (produto >= 0){
            consumir(produto); //Consome o item obtido.
            sem_post(&semaforo_enche);
        }else{
            break; //produto < 0 é um sinal de que o consumidor deve parar
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s tamanho_buffer itens_produzidos\n", argv[0]);
        return 0;
    }

    tamanho_buffer = atoi(argv[1]);
    int n_itens = atoi(argv[2]);
    printf("n_itens: %d\n", n_itens);

    //Iniciando buffer
    indice_produtor = 0;
    indice_consumidor = 0;
    buffer = malloc(sizeof(int) * tamanho_buffer);

    // Crie threads e o que mais for necessário para que uma produtor crie
    // n_itens produtos e o consumidor os consuma
    // ....
    pthread_t prod, cons;
    sem_init(&semaforo_esvazia, 0, 0); //inicializo o semaforo em 0 e seto segundo argumento para 0 pois ele será
    sem_init(&semaforo_enche, 0, tamanho_buffer); //compartilhado entre threads
    pthread_create(&prod, NULL, produtor_func, &n_itens);
    pthread_create(&cons, NULL, consumidor_func, &n_itens);

    pthread_join(prod, NULL);
    pthread_join(cons, NULL);

    sem_destroy(&semaforo_esvazia);
    sem_destroy(&semaforo_enche);
    free(buffer);

    return 0;
}
