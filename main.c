#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> 
#include <windows.h>

// Definindo variáveis especiais
#define max_processos 10
#define quantum 3
int contador_global = 0;
int tempo_chegada_acumulado = 0;

// Estrutura do Bloco de Controle de Processo (PCB)
typedef struct {
    int PID;
    int PPID;
    int status; // status: 0=Pronto, 1=Executando, 2=Bloqueado(I/O), 3=Terminado
} pcb;

// Estrutura do Processo
typedef struct {
    pcb pcb_info; 
    int tempo_servico; // Quanto tempo total de CPU ele precisa
    int tempo_chegada; // Quando ele entra no sistema
    int tempo_CPU_usado; // Quanto tempo de CPU já usou
} processo;

// Estrutura da Fila
typedef struct {
    int inicio;
    int fim;
    int tamanho;
    processo fila_processos[max_processos];
} fila;

// Protótipos das funções
processo criadora_processos();
void inicializar_fila(fila* f);
void enfileirar(fila* f, processo p);
processo desenfileirar(fila* f);
void printa_fila(fila* f);
void printa_lista_de_processos(processo lista[], int num_processos);
int rodar_processo(processo* p, int tempo_atual);
void CPU_executa(fila* alta, fila* baixa, int* finalizados, int tempo_atual);


// --- Implementação das Funções ---

processo criadora_processos(){
    processo novo_processo;

    novo_processo.pcb_info.PID = contador_global++;
    novo_processo.pcb_info.PPID = 1; // Processo pai simbólico
    novo_processo.pcb_info.status = 0; // 0 = Pronto

    // Gera um tempo de serviço entre 1 e 10
    novo_processo.tempo_servico = (rand() % 10) + 1; 
    novo_processo.tempo_CPU_usado = 0;

    // Gera um tempo de chegada com intervalo para não chegarem todos juntos
    tempo_chegada_acumulado += (rand() % 4 + 1); 
    novo_processo.tempo_chegada = tempo_chegada_acumulado; 
    
    return novo_processo;
}

void inicializar_fila(fila* f){
    f->inicio = 0;
    f->fim = 0;
    f->tamanho = 0;
}

void enfileirar(fila* f, processo p){
    if (f->tamanho >= max_processos){
        printf("ERRO: Fila cheia!\n");
        return;
    } 
    f->fila_processos[f->fim] = p;
    f->fim = (f->fim + 1) % max_processos;
    f->tamanho++;
}

processo desenfileirar(fila* f){
    if (f->tamanho == 0){
        processo processo_vazio = {0}; 
        processo_vazio.pcb_info.PID = -1;
        return processo_vazio;
    }
    processo p = f->fila_processos[f->inicio]; 
    f->inicio = (f->inicio + 1) % max_processos; 
    f->tamanho--;
    return p;
}

void printa_fila(fila* f){
    if (f->tamanho == 0) {
        printf("[vazia]");
        return;
    }
    int i = f->inicio;
    int count = 0;
    while(count < f->tamanho) {
        printf("[PID:%d] ", f->fila_processos[i].pcb_info.PID);
        i = (i + 1) % max_processos;
        count++;
    }
}

void printa_lista_de_processos(processo lista[], int num_processos) {
    printf("--- Lista de Processos a serem Simulados ---\n");
    for (int i = 0; i < num_processos; i++) {
        printf("PID: %d | Chegada: %d | Tempo de Servico: %d\n",
               lista[i].pcb_info.PID,
               lista[i].tempo_chegada,
               lista[i].tempo_servico);
    }
    printf("---------------------------------------------\n\n");
}

int rodar_processo(processo* p, int tempo_atual) {
    printf("Tempo %d: CPU <- PID %d (Servico restante: %d)\n", tempo_atual, p->pcb_info.PID, p->tempo_servico);
    p->pcb_info.status = 1; // 1 = Executando
    
    int tempo_executado = 0;
    // O processo roda por no máximo 'quantum' tempo, ou até terminar.
    while (tempo_executado < quantum && p->tempo_servico > 0) {
        p->tempo_servico--;
        p->tempo_CPU_usado++;
        tempo_executado++;
    }

    if (p->tempo_servico <= 0) {
        p->pcb_info.status = 3; // 3 = Terminado
        printf("Tempo %d: PID %d TERMINOU.\n", tempo_atual + tempo_executado, p->pcb_info.PID);
        return 1; // Retorna 1 se o processo terminou
    } else {
        p->pcb_info.status = 0; // 0 = Pronto (de volta para a fila)
        printf("Tempo %d: PID %d sofreu PREEMPCAO. Falta %d de servico.\n", tempo_atual + tempo_executado, p->pcb_info.PID, p->tempo_servico);
        return 0; // Retorna 0 se não terminou (foi preemptado)
    }
}

void CPU_executa(fila* alta, fila* baixa, int* finalizados, int tempo_atual) {
    processo p_executando;

    // 1. Verifica se há processo na fila de ALTA prioridade
    if (alta->tamanho > 0) {
        p_executando = desenfileirar(alta);
        int resultado = rodar_processo(&p_executando, tempo_atual);

        if (resultado == 1) { // Processo terminou
            (*finalizados)++;
        } else { // Processo sofreu preempção, vai para a fila de BAIXA prioridade
            enfileirar(baixa, p_executando);
        }
    }
    // 2. Se não, verifica a fila de BAIXA prioridade
    else if (baixa->tamanho > 0) {
        p_executando = desenfileirar(baixa);
        int resultado = rodar_processo(&p_executando, tempo_atual);

        if (resultado == 1) { // Processo terminou
            (*finalizados)++;
        } else { // Sofreu preempção, volta para o FIM da própria fila de BAIXA prioridade
            enfileirar(baixa, p_executando);
        }
    }
    // 3. Se ambas as filas estão vazias, a CPU fica ociosa
    else {
        printf("Tempo %d: CPU Ociosa...\n", tempo_atual);
    }
}


// --- Lógica Principal ---
int main() {
    // Inicialização das filas
    fila prioridade_alta, prioridade_baixa;
    inicializar_fila(&prioridade_alta);
    inicializar_fila(&prioridade_baixa);
    
    // As filas de I/O não estão sendo usadas nesta versão simplificada
    // fila fila_io_disco, fila_io_fita, fila_io_impressora;
    // inicializar_fila(&fila_io_disco);
    // inicializar_fila(&fila_io_fita);
    // inicializar_fila(&fila_io_impressora);

    // Criação prévia de todos os processos
    processo todos_os_processos[max_processos]; 
    for (int i = 0; i < max_processos; i++) {
        todos_os_processos[i] = criadora_processos();
    }
    printa_lista_de_processos(todos_os_processos, max_processos);

    // Variáveis de controle da simulação
    int tempo_simulacao = 0;
    int processos_finalizados = 0;

    printf("\n--- INICIO DA SIMULACAO (Quantum = %d) ---\n", quantum);
    // O loop principal roda enquanto houver processos a serem finalizados.
    while (processos_finalizados < max_processos) {
        // ETAPA 1: Chegada de Novos Processos
        // Verifica se algum processo chegou no tempo atual para entrar na fila de alta prioridade.
        for (int i = 0; i < max_processos; i++) {
            if (todos_os_processos[i].tempo_chegada == tempo_simulacao) {
                printf("Tempo %d: Chegou o PID %d.\n", tempo_simulacao, todos_os_processos[i].pcb_info.PID);
                enfileirar(&prioridade_alta, todos_os_processos[i]);
            }
        }

        // ETAPA 2: Execução da CPU
        // O escalonador decide qual processo executar (se houver algum).
        CPU_executa(&prioridade_alta, &prioridade_baixa, &processos_finalizados, tempo_simulacao);
        
        // ETAPA 3: Avanço do Tempo e Exibição do Estado
        printf("Estado no fim do tempo %d: Fila Alta: ", tempo_simulacao);
        printa_fila(&prioridade_alta);
        printf(" | Fila Baixa: ");
        printa_fila(&prioridade_baixa);
        printf("\n---------------------------------------------\n");
        
        tempo_simulacao++;

        getchar(); 
        Sleep(1000); // Pausa por 1 segundo (Windows)
    }

    printf("\n--- FIM DA SIMULACAO ---\n");
    printf("Simulacao finalizada em %d unidades de tempo.\n", tempo_simulacao);
    printf("Total de processos finalizados: %d\n", processos_finalizados);

    return 0;
}