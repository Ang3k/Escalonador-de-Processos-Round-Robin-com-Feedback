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

// Lógica Principal
int main() {
    srand(time(NULL)); 
    int processos_criados = 0;
    processo fila_processos[max_processos];

    for (int i = processos_criados ; processos_criados < max_processos ; i++){
        fila_processos[processos_criados] = criadora_processos();
        processos_criados++;
    }

    for (int i = 0; i < processos_criados; i++) {
    printf("PID: %d | Chegada: %d | Tempo de Serviço: %d | Prioridade: %d",
            fila_processos[i].pcb_info.PID,
            fila_processos[i].tempo_chegada,
            fila_processos[i].tempo_servico,
            fila_processos[i].pcb_info.prioridade);
    printf("\n");
}
        
    return 0;
}