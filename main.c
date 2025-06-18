#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Definindo variáveis especiais
#define max_processos 10
#define quantum 3
int contador_global = 0;
int tempo_chegada = 0;

// Criando estruturas para o processo
typedef struct {
    int prioridade;
    int PID;
    int PPID;
    int status;
} pcb;

typedef struct {
    pcb pcb_info; 
    int tempo_servico;
    int tempo_chegada;
    int tempo_CPU;
    // falta arrumar um jeito de implementar as operações de E/S e seus tempos
} processo;

typedef struct {
    int inicio;
    int fim;
    int tamanho;
    processo fila_processos[max_processos];
} fila;

// Função criadora de processos aleatoriamente
processo criadora_processos(){
    processo novo_processo;

    // Informações da PCB
    novo_processo.pcb_info.prioridade = rand() % 2; // 0 para baixa prioridade, 1 para alta prioridade?
    novo_processo.pcb_info.PID = contador_global++;
    novo_processo.pcb_info.PPID = contador_global + 100;
    novo_processo.pcb_info.status = 0; // 0 para pronto?

    // Informações do processo
    novo_processo.tempo_servico = rand() % 10;
    tempo_chegada += (rand() % 4 + 2); 
    novo_processo.tempo_chegada = tempo_chegada; 
    
    return novo_processo;
}

// Função para inicializar uma fila vazia
void inicializar_fila(fila* f){
    f->inicio = 0;
    f->fim = 0;
    f->tamanho = 0;
}

// Enfileirar processos em uma fila
void enfileirar(fila* f, processo p){
    if (f->tamanho >= max_processos){
        printf("ERRO: Tamanho máximo atingido pelo sistema!");
        return;
    } 
    f->fila_processos[f->fim] = p;
    f->fim = (f->fim + 1) % max_processos;
    f->tamanho++;
}

void printa_fila(fila* f){
    printf("FILA PID: ");
    for (int i = 0 ; i < f->tamanho ; i++){
        printf(" %d ", f->fila_processos[i].pcb_info.PID);
    }
    printf("\n");
}


void printa_lista_de_processos(processo lista[], int num_processos) {
    printf("--- Lista de Processos Criados ---\n");
    for (int i = 0; i < num_processos; i++) {
        printf("PID: %d | Chegada: %d | Tempo de Servico: %d | Prioridade: %d\n",
               lista[i].pcb_info.PID,
               lista[i].tempo_chegada,
               lista[i].tempo_servico,
               lista[i].pcb_info.prioridade);
    }
    printf("----------------------------------\n\n");
}

// Lógica Principal
int main() {
    fila prioridade_baixa;
    inicializar_fila(&prioridade_baixa);
    fila prioridade_alta;
    inicializar_fila(&prioridade_alta);
    fila fila_io_disco;
    inicializar_fila(&fila_io_disco);
    fila fila_io_fita;
    inicializar_fila(&fila_io_fita);
    fila fila_io_impressora;
    inicializar_fila(&fila_io_impressora);

    srand(time(NULL)); 
    int processos_criados = 0;
    processo todos_os_processos[max_processos]; 

    // Processos sendo previamente criados
    for (int i = 0 ; i < max_processos ; i++){ // Loop mais conciso
        todos_os_processos[i] = criadora_processos();
        processos_criados++;
    }

    // Chamando a função modularizada para printar a lista de processos
    printa_lista_de_processos(todos_os_processos, processos_criados);

    // Processos sendo simulados como se estivessem chegando
    printf("--- Simulacao de Chegada de Processos ---\n");
    for (int tempo_simulacao = 0; tempo_simulacao < 50; tempo_simulacao++) {
        for (int i = 0; i < max_processos; i++) {
            if (todos_os_processos[i].tempo_chegada == tempo_simulacao) {
                printf("Tempo %d: Processo PID %d chegou. Prioridade: %d\n", tempo_simulacao, todos_os_processos[i].pcb_info.PID, todos_os_processos[i].pcb_info.prioridade);
                if (todos_os_processos[i].pcb_info.prioridade == 1) enfileirar(&prioridade_alta, todos_os_processos[i]);
                if (todos_os_processos[i].pcb_info.prioridade == 0) enfileirar(&prioridade_baixa, todos_os_processos[i]);
                }
            }
        }
    
    printf("-----------------------------------------\n");

    // Filas após a inserção simples nas filas de prioridade
    printf("Fila de Prioridade ALTA: ");
    printa_fila(&prioridade_alta);
    printf("Fila de Prioridade BAIXA: ");
    printa_fila(&prioridade_baixa);
    printf("----------------------------------------\n");

    return 0;
}