#ifndef FILA_H
#define FILA_H

// Definindo constantes importantes
#define max_processos 10

// Estruturas (as mesmas que você já tem)
typedef struct {
    int PID;
    int PPID;
    int status;
} pcb;

typedef struct {
    pcb pcb_info;
    int tempo_servico;
    int tempo_chegada;
    int tempo_CPU_usado;

    int num_total_IO;
    int proximo_IO;
    int tipo_IO[3];
    int gatilhos_IO[3];
    int tempo_restante_IO;
} processo;

typedef struct {
    int inicio;
    int fim;
    int tamanho;
    processo fila_processos[max_processos];
} fila;

// Protótipos das funções da fila
void inicializar_fila(fila* f);
void enfileirar(fila* f, processo p);
processo desenfileirar(fila* f);
void printa_fila(fila* f);

#endif // FILA_H