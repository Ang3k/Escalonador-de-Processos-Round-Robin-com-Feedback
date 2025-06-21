# Simulador de Escalonador de Processos

Este projeto é uma implementação em C de um simulador de escalonador de processos, desenvolvido para a disciplina de Arquitetura de Computadores e Sistemas Operacionais. O objetivo é simular a alocação de CPU e o gerenciamento de filas de processos e de I/O.

## Algoritmo Implementado

O simulador implementa o algoritmo **Round Robin com Feedback**, utilizando múltiplas filas de prioridade para gerenciar os processos. As regras principais são:

* **Filas de Prontos**: O sistema utiliza uma fila de **alta prioridade** e uma de **baixa prioridade**.
* **Chegada de Processos**: Processos novos entram na fila de alta prioridade.
* **Preempção**: Um processo em execução é preemptado se seu tempo de uso da CPU excede o *quantum* definido, sendo então movido para a fila de baixa prioridade.
* **Operações de I/O**: Os processos podem solicitar operações de I/O para três tipos de dispositivos: Disco, Fita Magnética e Impressora. Ao fazer isso, são movidos para a fila do respectivo dispositivo.
* **Retorno de I/O**: O retorno para as filas de prontos depende do dispositivo:

  * **Disco**: Retorna para a fila de baixa prioridade.
  * **Fita Magnética e Impressora**: Retornam para a fila de alta prioridade.

## Estrutura dos Arquivos

* **`main.c`**: Contém a lógica principal da simulação, o controle do tempo e o gerenciamento dos ciclos de execução.
* **`fila.c`**: Implementa as funções para o gerenciamento das filas (enfileirar, desenfileirar, etc.).
* **`fila.h`**: Arquivo de cabeçalho que define as estruturas de dados (`processo`, `fila`) e os protótipos das funções utilizadas no projeto.

## Como Compilar e Executar

1. Para compilar o projeto, utilize o seguinte comando no terminal:

   ```bash
   gcc main.c fila.c -o main
   ```

2. Para executar o simulador após a compilação:

   ```bash
   ./main
   ```

---
