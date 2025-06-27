#ifndef FILA_H
#define FILA_H
#define max_processos 10     

typedef struct {
    int PID;
    int PPID;
    int status;              /* 0-pronto | 1-rodando | 2-I/O | 3-terminou */
} pcb;

typedef struct {
    pcb  pcb_info;
    int  tempo_servico;
    int  tempo_chegada;
    int  tempo_CPU_usado;

    int  num_total_IO;
    int  proximo_IO;
    int  tipo_IO[3];
    int  gatilhos_IO[3];
    int  tempo_restante_IO;
    int  tempo_inicio_IO;     /* instante em que o dispositivo começa a atendê-lo */
} processo;

typedef struct {
    int inicio;
    int fim;
    int tamanho;
    processo fila_processos[max_processos];
} fila;

// Protótipos (fila.c)
void inicializar_fila(fila* f);
void enfileirar(fila* f, processo p);
processo desenfileirar(fila* f);
void printa_fila(fila* f);

#endif /* FILA_H */