#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define TAMANHO_PAGINA 1024
#define NUMERO_QUADROS 4
#define NUMERO_PAGINAS 10
#define ENDERECO_MAXIMO (NUMERO_PAGINAS * TAMANHO_PAGINA - 1)

typedef struct {
    int id_quadro_fisico;
    bool presente;
    bool modificado;
    int tempo_carregamento;
} EntradaTabelaPaginas;

typedef struct {
    int id_pagina_virtual;
    char dados[TAMANHO_PAGINA];
} QuadroMemoriaFisica;

typedef struct {
    char dados[TAMANHO_PAGINA];
} PaginaArmazenamento;

EntradaTabelaPaginas tabela_paginas[NUMERO_PAGINAS];
QuadroMemoriaFisica quadros_memoria[NUMERO_QUADROS];
PaginaArmazenamento armazenamento_disco[NUMERO_PAGINAS];
int total_falhas_pagina = 0;
int tempo_global = 0;
int proximo_quadro_fifo = 0;

int obter_id_pagina(int endereco_virtual) {
    return endereco_virtual / TAMANHO_PAGINA;
}

void mostrar_estado_atual();
bool acessar_memoria(int endereco_virtual, char operacao, char valor);


void escrever_no_disco(int id_pagina, char *dados_a_escrever) {
    printf("  [SWAP OUT] Escrevendo dados da Página %d de volta para o Disco...\n", id_pagina);
    memcpy(armazenamento_disco[id_pagina].dados, dados_a_escrever, TAMANHO_PAGINA);
    printf("  [SWAP OUT CONCLUÍDO] Dados da P%d salvos no Disco.\n", id_pagina);
}

void carregar_do_disco(int id_pagina, char *dados_a_carregar_em) {
    printf("  [SWAP IN] Carregando dados da Página %d do Disco para o Quadro...\n", id_pagina);
    memcpy(dados_a_carregar_em, armazenamento_disco[id_pagina].dados, TAMANHO_PAGINA);
    printf("  [SWAP IN CONCLUÍDO] Dados da P%d carregados.\n", id_pagina);
}

void inicializar_sistema() {
    printf("Inicializando Memória Virtual...\n");
    for (int i = 0; i < NUMERO_PAGINAS; i++) {
        tabela_paginas[i].id_quadro_fisico = -1;
        tabela_paginas[i].presente = false;
        tabela_paginas[i].modificado = false;
        tabela_paginas[i].tempo_carregamento = 0;

        for(int j = 0; j < TAMANHO_PAGINA; j++) {
            armazenamento_disco[i].dados[j] = (char)('A' + (i % 26));
        }
    }

    for (int i = 0; i < NUMERO_QUADROS; i++) {
        quadros_memoria[i].id_pagina_virtual = -1;
        memset(quadros_memoria[i].dados, 0, TAMANHO_PAGINA);
    }
    printf("Sistema Inicializado. Quadros disponíveis: %d\n\n", NUMERO_QUADROS);
}

void mostrar_estado_atual() {
    printf("\n--- ESTADO ATUAL ---\n");
    printf("Memória (Quadros): ");
    for (int i = 0; i < NUMERO_QUADROS; i++) {
        int id_pag = quadros_memoria[i].id_pagina_virtual;
        if (id_pag != -1) {
            bool modificado = tabela_paginas[id_pag].modificado;
            int tempo_c = tabela_paginas[id_pag].tempo_carregamento;
            printf("[Q%d: P%d (FIFO: %d, Modificado: %s)] ", i, id_pag, tempo_c, modificado ? "S" : "N");
        } else {
            printf("[Q%d: Vazio] ", i);
        }
    }
    printf("\nPróxima Vítima FIFO: Quadro %d\n", proximo_quadro_fifo);
    printf("Falhas de Página Totais: %d\n", total_falhas_pagina);
    printf("--------------------\n");
}

void tratar_falha_pagina(int id_pagina, char operacao, char valor, int offset) {
    int quadro_vitima = proximo_quadro_fifo;
    int id_pagina_vitima = quadros_memoria[quadro_vitima].id_pagina_virtual;

    printf("  [FIFO] Quadro %d selecionado como VÍTIMA (Continha P%d).\n", quadro_vitima, id_pagina_vitima);

    if (id_pagina_vitima != -1) {
        tabela_paginas[id_pagina_vitima].presente = false;
        tabela_paginas[id_pagina_vitima].id_quadro_fisico = -1;

        if (tabela_paginas[id_pagina_vitima].modificado) {
            escrever_no_disco(id_pagina_vitima, quadros_memoria[quadro_vitima].dados);
            tabela_paginas[id_pagina_vitima].modificado = false;
        } else {
            printf("  [SWAP OUT] Página %d estava 'Limpa', descartada.\n", id_pagina_vitima);
        }
    }

    carregar_do_disco(id_pagina, quadros_memoria[quadro_vitima].dados);

    quadros_memoria[quadro_vitima].id_pagina_virtual = id_pagina;
    
    EntradaTabelaPaginas *entrada_pte = &tabela_paginas[id_pagina];
    entrada_pte->id_quadro_fisico = quadro_vitima;
    entrada_pte->presente = true;
    entrada_pte->tempo_carregamento = ++tempo_global;
    entrada_pte->modificado = false;

    int posicao_no_quadro = offset;
    
    if (operacao == 'W') {
        quadros_memoria[quadro_vitima].dados[posicao_no_quadro] = valor;
        entrada_pte->modificado = true;
        printf("  Operação 'W' aplicada. Quadro %d marcado como 'Modificado'.\n", quadro_vitima);
    } else {
         printf("  Operação 'R' aplicada. Valor lido: '%c'.\n", quadros_memoria[quadro_vitima].dados[posicao_no_quadro]);
    }

    proximo_quadro_fifo = (proximo_quadro_fifo + 1) % NUMERO_QUADROS;
}

bool acessar_memoria(int endereco_virtual, char operacao, char valor) {
    if (endereco_virtual < 0 || endereco_virtual > ENDERECO_MAXIMO) {
        printf("ERRO: Endereço virtual %d fora do intervalo.\n", endereco_virtual);
        return false;
    }

    int id_pagina = obter_id_pagina(endereco_virtual);
    int offset = endereco_virtual % TAMANHO_PAGINA;
    EntradaTabelaPaginas *entrada_pte = &tabela_paginas[id_pagina];

    printf("Acesso: Endereço %d (Página %d, Offset %d) | Operação: %c\n",
           endereco_virtual, id_pagina, offset, operacao);

    if (entrada_pte->presente) {
        int id_quadro = entrada_pte->id_quadro_fisico;
        printf("  [HIT] Página %d encontrada no Quadro %d.\n", id_pagina, id_quadro);

        if (operacao == 'W') {
            quadros_memoria[id_quadro].dados[offset] = valor;
            entrada_pte->modificado = true;
            printf("  Dados escritos. Quadro %d marcado como 'Modificado'.\n", id_quadro);
        } else {
            printf("  Dados lidos: '%c'\n", quadros_memoria[id_quadro].dados[offset]);
        }
        return true;

    } else {
        printf("  [FALHA DE PÁGINA] Página %d não está na memória.\n", id_pagina);
        total_falhas_pagina++;
        tratar_falha_pagina(id_pagina, operacao, valor, offset);
        return false;
    }
}

int main() {
    inicializar_sistema();

    printf("--- SIMULAÇÃO DE ACESSO À MEMÓRIA ---\n");
    
    acessar_memoria(0, 'R', 0);    
    acessar_memoria(1024, 'R', 0); 
    acessar_memoria(2048, 'R', 0); 
    acessar_memoria(3072, 'R', 0); 
    mostrar_estado_atual();

    acessar_memoria(1024, 'W', 'X'); 
    mostrar_estado_atual();
    
    acessar_memoria(4096, 'R', 0); 
    mostrar_estado_atual();
    
    acessar_memoria(5120, 'R', 0); 
    mostrar_estado_atual();

    acessar_memoria(4096, 'W', 'Z'); 
    mostrar_estado_atual();

    acessar_memoria(6144, 'R', 0);
    mostrar_estado_atual();
    
    printf("\n--- SIMULAÇÃO FINALIZADA ---\n");
    printf("Total de Falhas de Página: %d\n", total_falhas_pagina);
    
    return 0;
}