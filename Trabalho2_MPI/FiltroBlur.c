#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mpi.h>

#define MAX_BUFFER 50

typedef struct PGM
{
  char type[3];
  unsigned char **data;
  unsigned int width;
  unsigned int height;
  unsigned int grayvalue;
  double time;
} PGM;

typedef struct {
    PGM *Average;
    PGM *Median;
} ResultadosFiltro;

PGM *pgm;
PGM *pgmParaleloAverage;
PGM *pgmParaleloMedian;
PGM *pgmSequencial;

int inicializa_PGM(PGM *pgm_a){
  // Aloca as linhas da matriz dinamicamente;
  pgm_a->data = malloc(pgm_a->height * sizeof(unsigned char *));

  // Aloca as colunas da matiz dinamicamente;
  for (int i = 0; i < pgm_a->height; i++)
  {
    pgm_a->data[i] = malloc(pgm_a->width * sizeof(unsigned char));
  }
}

void removerEspacos(char *str)
{
    int i = 0, j = 0;

    while (str[i] != '\0') {
        if (!isspace((unsigned char)str[i])) {
            str[j++] = str[i];
        }
        i++;
    }

    str[j] = '\0';
}

void lerString(char buffer[]){
    int erro;
    char c;

    do{
        erro=0;

        if (fgets(buffer, MAX_BUFFER, stdin) == NULL) {
            printf("\n\nFim da execucao [Ctrl+Z].\n");
            exit(1);
        }

        if(strlen(buffer)== MAX_BUFFER && buffer[strlen(buffer)-1] != '\n'){
            printf("\n\nERRO: Entrada muito grande. Digite novamente: ");
            erro=1;
            while ((c = getchar()) != '\n' && c != EOF);
        }

        removerEspacos(buffer);
        buffer[strcspn(buffer, "\n")] = '\0';

        if (buffer[0] == '\0')
        {
            printf("\n\nERRO: Entrada vazia. Digite novamente: \n");
            erro=1;
        }
    }while(erro);

}

void lerNumero(char buffer[]){
    int erro;
    do{
        erro=0;
        if (fgets(buffer, MAX_BUFFER, stdin) != NULL) {

            removerEspacos(buffer);

            char *fim;
            long valor = strtol(buffer, &fim, 10);

            if (fim == buffer) {
                printf("\nNenhum numero foi digitado.\n");
                erro=1;
            }
            else if (*fim != '\n' && *fim != '\0') {
                printf("\nA entrada nao e um inteiro valido.\n");
                erro=1;
            }
            else if (valor < 2 || valor > 20) {
                printf("\nNumero fora do intervalo delimitado [2-20].\n");
                erro=1;
            }
        }else{
            printf("\n\nFim da execucao [Ctrl+Z].\n");
            exit(1);
        }
    }while(erro);
}

int nomeArquivoValido(const char *nome) {
    int i, len;
    if (nome == NULL)
        return 0;

    len = strlen(nome);

    // Tamanho mínimo: "a.pgm"
    if (len < 5)
        return 0;

    // Verifica extensão ".pgm"
    if (strcmp(nome + len - 4, ".pgm") != 0)
        return 0;

    // O nome não pode terminar com espaço ou ponto
    if (nome[len - 5] == ' ' || nome[len - 5] == '.')
        return 0;

    // Verifica caracteres inválidos
    for (i = 0; i < len - 4; i++) {
        unsigned char c = nome[i];

        // Caracteres de controle
        if (c < 32)
            return 0;

        switch (c) {
            case '<':
            case '>':
            case ':':
            case '"':
            case '/':
            case '\\':
            case '|':
            case '?':
            case '*':
                return 0;
        }
    }

    return 1;
}

void ignoreComments(FILE *fp)
{
  int ch;
  char line[100];
  // Ignora qualquer linha vazia
  while ((ch = fgetc(fp)) != EOF && isspace(ch))
    ;
  // Ignora recursivamente comentarios na imagem PGM
  if (ch == '#')
  {
    fgets(line, sizeof(line), fp);
    ignoreComments(fp);
  }
  else
  {
    fseek(fp, -1, SEEK_CUR);
  }
}

int openPGM(PGM *pgm, char fname[])
{
  FILE *pgmfile = fopen(fname, "rb");
  if (pgmfile == NULL)
  {
      printf("\nArquivo inexistente. Digite o nome novamente: \n");
      return 0;
  }

  ignoreComments(pgmfile);
  fscanf(pgmfile, "%s", pgm->type);
  ignoreComments(pgmfile);

  fscanf(pgmfile, "%d %d", &(pgm->width),
         &(pgm->height));
  ignoreComments(pgmfile);

  fscanf(pgmfile, "%d", &(pgm->grayvalue));
  ignoreComments(pgmfile);

  pgm->data = malloc(pgm->height * sizeof(unsigned char *));
  for (int i = 0; i < pgm->height; i++)
  {
    pgm->data[i] = malloc(pgm->width *
                          sizeof(unsigned char));
    fread(pgm->data[i], pgm->width * sizeof(unsigned char), 1, pgmfile);
  }

  fclose(pgmfile);
  return 1;
}

void printImageDetails(PGM *pgm, char filename[])
{
  FILE *pgmfile = fopen(filename, "rb");

  // searches the occurrence of '.'
  char *ext = strrchr(filename, '.');

  if (!ext)
    printf("\nSem extensao no nome '%s'", filename);
  else // portion after .
    printf("\nFormato do arquivo    : %s\n", ext + 1);

  printf("Tipo de arquivo PGM   : %s\n", pgm->type);
  printf("Compimento da imagem  : %d px\n", pgm->width);
  printf("Altura da imagem      : %d px\n", pgm->height);
  printf("Maior valor de cinza  : %d\n", pgm->grayvalue);

  fclose(pgmfile);
}
void saveImage(PGM *pgm, char fname[])
{
  FILE *fp = fopen(fname, "wb");

  fprintf(fp, "%s\n", pgm->type);
  fprintf(fp, "%d %d\n", pgm->width, pgm->height);
  fprintf(fp, "%d\n", pgm->grayvalue);

  for (int i = 0; i < pgm->height; i++)
  {
    for (int j = 0; j < pgm->width; j++)
    {
      fprintf(fp, "%c", pgm->data[i][j]);
    }
  }

  fclose(fp);
}

void liberarPGM(PGM *pgm)
{
    int i;

    if(pgm == NULL)
        return;

    if(pgm->data != NULL) {
        for(i = 0; i < pgm->height; i++)
            free(pgm->data[i]);

        free(pgm->data);
    }

    free(pgm);
}

void *filtroMPIAverage(int inicio, int fim, int tamFiltro, int id, int visao_detalhada){
  int i, j, k, l, divisor, soma, media;

  tamFiltro = tamFiltro/2;

  if(visao_detalhada){
      printf("Processo %i: Executando da linha (%i-%i).",id, inicio, fim);
      fflush(stdout);
  }

  for(i=inicio; i<=fim; i++)
  {
    for (j = 0; j < pgm->width; j++)
    {
      divisor = 0;
      soma = 0;
      for (k = -tamFiltro; k <= tamFiltro; k++)
      {
        for (l = -tamFiltro; l <= tamFiltro; l++)
        {
          if (i + k > 0 && j + l > 0 && i + k < pgm->height && j + l < pgm->width)
          {
            divisor++;
            soma = soma + (int)pgm->data[i + k][j + l];
          }
        }
      }
      media = soma / divisor;
      pgmParaleloAverage->data[i-inicio][j] = (char)media;
      //printf("\nPonto (%d,%d): Soma:%d | Media:%d ", i, j, soma, media);
    }
  }
  if(visao_detalhada){
    printf("Processo %i: Fim da execucao da linha (%i-%i).",id, inicio, fim);
    fflush(stdout);
  }
}

int cmp(const void *a, const void *b)
{
  return (*(int *)a - *(int *)b);
}

void *filtroMPIMedian(int inicio, int fim, int tamFiltro, int id, int visao_detalhada){
  int i, j, k, l, qtd_elem, mediana;

  if(tamFiltro%2 == 0)
      tamFiltro++;

  int *vetorAuxiliar = (int *)malloc((tamFiltro * tamFiltro) * sizeof(int));

  tamFiltro = tamFiltro/2;

  if(visao_detalhada){
      printf("Processo %i: Executando da linha (%i-%i).",id, inicio, fim);
      fflush(stdout);
  }

  for(i=inicio; i<=fim; i++)
  {
    for (j = 0; j < pgm->width; j++)
    {
      qtd_elem = 0;
      for (k = -tamFiltro; k <= tamFiltro; k++)
      {
        for (l = -tamFiltro; l <= tamFiltro; l++)
        {
          if (i + k > 0 && j + l > 0 && i + k < pgm->height && j + l < pgm->width)
          {
            vetorAuxiliar[qtd_elem] = (int)pgm->data[i + k][j + l];
            qtd_elem++;
          }
        }
      }
      qsort(vetorAuxiliar, qtd_elem, sizeof(int), cmp);
      mediana = vetorAuxiliar[qtd_elem / 2];
      pgmParaleloMedian->data[i-inicio][j] = (char)mediana;
      //printf("\nPonto (%d,%d): Soma:%d | Media:%d ", i, j, soma, media);
    }
  }
  free(vetorAuxiliar);

  if(visao_detalhada){
    printf("Processo %i: Fim da execucao da linha (%i-%i).",id, inicio, fim);
    fflush(stdout);
  }
}

ResultadosFiltro filtroParalelo(PGM *pgm, int tamanho_filtro, int np, int visao_detalhada)
{
  int i, j, n_int, n_resto, inicio, fim, variavel_auxiliar, qtd_linhas_original, qtd_colunas_original, tag=0;
  clock_t time_ini, time_fim, time_construcao, time_average, time_median;

  ResultadosFiltro Paralelo;

  MPI_Status st;

  time_ini = clock();

  // Aloca um PGM dinamicamente
  pgmParaleloAverage = malloc(sizeof(PGM));
  pgmParaleloMedian = malloc(sizeof(PGM));


  // Copia o PGM original
  strcpy(pgmParaleloAverage->type, pgm->type);
  pgmParaleloAverage->height = pgm->height;
  pgmParaleloAverage->width = pgm->width;
  pgmParaleloAverage->grayvalue = pgm->grayvalue;

  inicializa_PGM(pgmParaleloAverage);

  // Copia o PGM original
  strcpy(pgmParaleloMedian->type, pgm->type);
  pgmParaleloMedian->height = pgm->height;
  pgmParaleloMedian->width = pgm->width;
  pgmParaleloMedian->grayvalue = pgm->grayvalue;

  inicializa_PGM(pgmParaleloMedian);

  // Define a parte inteira e os restos da divisão de trabalho das threads
  n_int = pgm->height/np;
  n_resto = pgm->height % np;

  // Aux_inicio determina o início do intervalo de cálculo de uma thread
    inicio=0;
    fim=0;
    if (n_resto > 0){
        fim = n_int;
        n_resto--;
    }else{
        fim=n_int-1;
    }

    //printf("\nGerente: N_int:%d | n_resto:%d | inicio:%d | fim:%d| aux:%d", n_int, n_resto, inicio, fim, variavel_auxiliar);

    variavel_auxiliar = fim;

    for(i=1; i<np; i++){
		// Esse if/esle controla se existe um resto a ser adicionado ao intervalo das threads
		if (n_resto > 0){
			inicio = fim+1;
			fim=inicio+n_int;
			n_resto--;
		}else{
            inicio=fim+1;
            fim=inicio+n_int-1;
        }

        qtd_linhas_original = pgm->height;
        qtd_colunas_original = pgm->width;

        if(visao_detalhada){
            printf(
                "\nEnviando para o processo %d:"
                "\n  Linhas originais : %d"
                "\n  Colunas originais: %d"
                "\n  Inicio           : %d"
                "\n  Fim              : %d"
                "\n  Tamanho filtro   : %d"
                "\n  Visao Detalhada  : %d\n",
                i,
                qtd_linhas_original,
                qtd_colunas_original,
                inicio,
                fim,
                tamanho_filtro,
                visao_detalhada
            );
            fflush(stdout);
        }

        MPI_Send(&qtd_linhas_original, 1, MPI_INT, i, tag, MPI_COMM_WORLD);
        MPI_Send(&qtd_colunas_original, 1, MPI_INT, i, tag, MPI_COMM_WORLD);
        MPI_Send(&inicio, 1, MPI_INT, i, tag, MPI_COMM_WORLD);
        MPI_Send(&fim, 1, MPI_INT, i, tag, MPI_COMM_WORLD);
        MPI_Send(&tamanho_filtro, 1, MPI_INT, i, tag, MPI_COMM_WORLD);
        MPI_Send(&visao_detalhada, 1, MPI_INT, i, tag, MPI_COMM_WORLD);

        if(visao_detalhada){
            printf("\nGerente: Carregando Processo %d...\n", i);
            fflush(stdout);
        }

        for(j=0;j<qtd_linhas_original;j++){
            MPI_Send(pgm->data[j], qtd_colunas_original, MPI_UNSIGNED_CHAR, i, tag, MPI_COMM_WORLD);
        }

        if(visao_detalhada){
            printf("\nGerente: Processo %d carregado\n", i);
            fflush(stdout);
        }
  }

  time_fim = clock();

  time_construcao = ((double)(time_fim - time_ini))/CLOCKS_PER_SEC;

  if(visao_detalhada){
      printf("\n\nInicio da filtragem paralela por Media.");
      fflush(stdout);
  }

  time_ini = clock();

  filtroMPIAverage(0,variavel_auxiliar, tamanho_filtro, 0, visao_detalhada);

  for(i=1;i<np;i++){
    MPI_Recv(&inicio, 1, MPI_INT, i, tag, MPI_COMM_WORLD, &st);
    MPI_Recv(&fim, 1, MPI_INT, i, tag, MPI_COMM_WORLD, &st);

    if(visao_detalhada){
        printf("\n\nGerente: Recebendo PgmParaleloAverage (%d-%d) do Processo %d...", inicio, fim,i);
        fflush(stdout);
    }

    for(j=inicio;j<=fim;j++){
        MPI_Recv(pgmParaleloAverage->data[j], qtd_colunas_original, MPI_UNSIGNED_CHAR, i, tag, MPI_COMM_WORLD, &st);
    }

    if(visao_detalhada){
        printf("\n\nGerente: Recebido o PgmParaleloAverage (%d-%d) do Processo %d!", inicio, fim,i);
        fflush(stdout);
    }
  }
  time_fim = clock();

  pgmParaleloAverage->time = ((double)(time_fim - time_ini + time_construcao))/CLOCKS_PER_SEC;

  Paralelo.Average = pgmParaleloAverage;

  if(visao_detalhada){
      printf("\n\nFim da filtragem paralela por Media.\n");
      fflush(stdout);
  }

  MPI_Barrier(MPI_COMM_WORLD);

  if(visao_detalhada){
      printf("\n\nInicio da filtragem paralela por Mediana.");
      fflush(stdout);
  }

  time_ini = clock();

  filtroMPIMedian(0,variavel_auxiliar, tamanho_filtro, 0, visao_detalhada);

  for(i=1;i<np;i++){
    MPI_Recv(&inicio, 1, MPI_INT, i, tag, MPI_COMM_WORLD, &st);
    MPI_Recv(&fim, 1, MPI_INT, i, tag, MPI_COMM_WORLD, &st);

    if(visao_detalhada){
        printf("\n\nGerente: Recebendo PgmParaleloMedian (%d-%d) do Processo %d...", inicio, fim,i);
        fflush(stdout);
    }

    for(j=inicio;j<=fim;j++){
        MPI_Recv(pgmParaleloMedian->data[j], qtd_colunas_original, MPI_UNSIGNED_CHAR, i, tag, MPI_COMM_WORLD, &st);
    }

    if(visao_detalhada){
        printf("\n\nGerente: Recebido o PgmParaleloMedian(%d-%d) do Processo %d!", inicio, fim,i);
        fflush(stdout);
    }
  }

  time_fim = clock();

  if(visao_detalhada){
      printf("\n\nFim da filtragem paralela por Mediana.");
      fflush(stdout);
  }

  pgmParaleloMedian->time = ((double)(time_fim - time_ini + time_construcao))/CLOCKS_PER_SEC;

  Paralelo.Median = pgmParaleloMedian;

  return Paralelo;
}

PGM *filtroSequencialAverage(PGM *pgm, int filt_size)
{
  clock_t time_ini, time_fim;

  time_ini = clock();

  pgmSequencial = malloc(sizeof(PGM));
  int s = filt_size / 2;

  strcpy(pgmSequencial->type, pgm->type);
  pgmSequencial->height = pgm->height;
  pgmSequencial->width = pgm->width;
  pgmSequencial->grayvalue = pgm->grayvalue;
  pgmSequencial->data = malloc(pgm->height * sizeof(unsigned char *));
  for (int i = 0; i < pgm->height; i++)
  {
    pgmSequencial->data[i] = malloc(pgm->width *
                           sizeof(unsigned char));
  }

  for (int i = 0; i < pgm->height; i++)
  {
    for (int j = 0; j < pgm->width; j++)
    {
      int d = 0;
      int e = 0;
      for (int k = -s; k <= s; k++)
      {
        for (int l = -s; l <= s; l++)
        {
          if (i + k > 0 && j + l > 0 && i + k < pgm->height && j + l < pgm->width)
          {
            d++;
            e = e + (int)pgm->data[i + k][j + l];
          }
        }
      }
      e = e / d;
      pgmSequencial->data[i][j] = (char)e;
    }
  }

  time_fim = clock();

  pgmSequencial->time = ((double)(time_fim - time_ini))/ CLOCKS_PER_SEC;

  return pgmSequencial;
}

// Funcao base do problema original. Alteracao apenas no calculo do tempo
PGM *filtroSequencialMedian(PGM *pgm, int filt_size)
{
  clock_t time_ini, time_fim;

  time_ini = clock();

  if(filt_size%2 == 0)
      filt_size++;

  pgmSequencial = malloc(sizeof(PGM));
  int s = filt_size / 2;
  int sq = filt_size * filt_size;
  int *a = (int *)malloc(sq * sizeof(int));

  strcpy(pgmSequencial->type, pgm->type);
  pgmSequencial->height = pgm->height;
  pgmSequencial->width = pgm->width;
  pgmSequencial->grayvalue = pgm->grayvalue;
  pgmSequencial->data = malloc(pgm->height * sizeof(unsigned char *));

  for (int i = 0; i < pgm->height; i++)
  {
    pgmSequencial->data[i] = malloc(pgm->width *
                           sizeof(unsigned char));
  }
  for (int i = 0; i < pgm->height; i++)
  {
    for (int j = 0; j < pgm->width; j++)
    {
      int d = 0;
      int e = 0;
      int m = 0;
      for (int k = -s; k <= s; k++)
      {
        for (int l = -s; l <= s; l++)
        {
          if (i + k > 0 && j + l > 0 && i + k < pgm->height && j + l < pgm->width)
          {
            a[m] = (int)pgm->data[i + k][j + l];
            m++;
          }
        }
      }
      qsort(a, m, sizeof(int), cmp);
      int p = a[m / 2];
      // e = e / d;
      pgmSequencial->data[i][j] = (char)p;

    }

  }
  free(a);

  time_fim = clock();

  pgmSequencial->time = ((double)(time_fim - time_ini))/ CLOCKS_PER_SEC;

  return pgmSequencial;
}

void comparaMatriz(PGM *pgm1, PGM *pgm2){
  int i, j, igual;

  igual=1;

  for(i=0;i<pgm1->height;i++){
    for(j=0;j<pgm1->width;j++){
      if(pgm1->data[i][j] != pgm2->data[i][j]){
        igual=0;
      }
    }
  }
  if(igual){
    printf("\nTodos os elementos das matrizes Sequencial e Paralela sao iguais.\n");
    fflush(stdout);
  }else{
    printf("\nExistem divergencias entre as matrizes Sequencial e Paralela.\n");
    fflush(stdout);
  }

}

int main(int argc, char *argv[])
{
    int i, j, np, id, inicio, fim,janelaFiltro, qtd_linhas_original, qtd_colunas_original, tag=0;
    char nome_arquivo[MAX_BUFFER], buffer[MAX_BUFFER], visaoDetalhada;
    int loop=0;

    FILE *arquivo_saida;

    ResultadosFiltro sequencial;
    ResultadosFiltro paralelo;

    MPI_Status st;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &np);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);


    pgm = malloc(sizeof(PGM));

    do{

        if(id==0){
            srand(time(NULL));

            printf("\n----------------------------------------------------------------------------");
            printf("\nParalelizando a aplicacao de filtro  blur por media e mediana em imagens PGM\n");
            printf("----------------------------------------------------------------------------\n");

            do{
                printf("\nInsira o arquivo PGM com extensao ('nome_arquivo'.pgm): ");
                fflush(stdout);
                lerString(buffer);

                while(!nomeArquivoValido(buffer)){
                    printf("\nNome do arquivo invalido! Digite novamente ('nome_arquivo'.pgm): ");
                    fflush(stdout);

                    lerString(buffer);
                }

                strcpy(nome_arquivo, buffer);

            }while(!openPGM(pgm, nome_arquivo));

            printImageDetails(pgm, nome_arquivo);

            printf("\nInsira o tamanho da janela de filtro em valores inteiros [2-20]: ");
            fflush(stdout);

            lerNumero(buffer);

            janelaFiltro = atoi(buffer);

            printf("\n\nVisao detalhada da execucao? [y/n]: ");
            fflush(stdout);
            lerString(buffer);

            while (strlen(buffer) != 1 || (tolower((unsigned char)buffer[0]) != 'y' && tolower((unsigned char)buffer[0]) != 'n'))
            {
                printf("\nResposta invalida! Responda 'y' ou 'n' [y/n]: ");
                fflush(stdout);

                lerString(buffer);
            }

            if(tolower(buffer[0]) == 'y'){
                visaoDetalhada = 1;
            }else{
                visaoDetalhada = 0;
            }

            // Roda a execução paralela
            paralelo = filtroParalelo(pgm, janelaFiltro, np, visaoDetalhada);

            printf("\n\nGerente: Fim da filtragem paralela.\n");
            fflush(stdout);

            saveImage(paralelo.Average, "FiltroParaleloAverage.pgm");
            saveImage(paralelo.Median, "FiltroParaleloMedian.pgm");

            MPI_Barrier(MPI_COMM_WORLD);

            // Roda a execução sequencial
            sequencial.Average = filtroSequencialAverage(pgm, janelaFiltro);
            saveImage(sequencial.Average, "FiltroSequencialAverage.pgm");

            sequencial.Median = filtroSequencialMedian(pgm, janelaFiltro);
            saveImage(sequencial.Median, "FiltroSequencialMedian.pgm");

            printf("\nA imagem foi Filtrada.\n");

            printf("\nComparacao Paralelo Sequencial -> Average.");
            fflush(stdin);
            comparaMatriz(paralelo.Average, sequencial.Average);

            printf("\nComparacao Paralelo Sequencial -> Median.");
            fflush(stdin);
            comparaMatriz(paralelo.Median, sequencial.Median);


            arquivo_saida = fopen("log.txt", "a");

            if (arquivo_saida == NULL) {
                printf("Erro ao abrir o arquivo.\n");
                return 1;
            }


            fprintf(arquivo_saida, "\nExecucao %d:", loop);
            fprintf(arquivo_saida, "\nTempo de execucao Filtro Sequencial Average: %f", sequencial.Average->time);
            fprintf(arquivo_saida, "\nTempo de execucao Filtro Sequencial Median: %f", sequencial.Median->time);
            fprintf(arquivo_saida, "\nTempo de execucao Filtro Paralelo Average: %f", paralelo.Average->time);
            fprintf(arquivo_saida, "\nTempo de execucao Filtro Paralelo Median: %f\n", paralelo.Median->time);

            fclose(arquivo_saida);

            printf("\nTempo de execucao Filtro Sequencial Average: %f", sequencial.Average->time);
            printf("\nTempo de execucao Filtro Sequencial Median: %f", sequencial.Median->time);
            printf("\nTempo de execucao Filtro Paralelo Average: %f", paralelo.Average->time);
            printf("\nTempo de execucao Filtro Paralelo Median: %f", paralelo.Median->time);

            printf("\n\nDeseja filtrar a imagem novamente? [y/n] ");
            fflush(stdout);
            lerString(buffer);

            while (strlen(buffer) != 1 || (tolower((unsigned char)buffer[0]) != 'y' && tolower((unsigned char)buffer[0]) != 'n'))
            {
                printf("\nResposta invalida! Responda 'y' ou 'n' [y/n]: ");
                fflush(stdout);

                lerString(buffer);
            }

            liberarPGM(paralelo.Average);
            liberarPGM(paralelo.Median);
            liberarPGM(sequencial.Average);
            liberarPGM(sequencial.Median);



        }else{
                MPI_Recv(&qtd_linhas_original, 1, MPI_INT,0, tag, MPI_COMM_WORLD, &st);
                MPI_Recv(&qtd_colunas_original, 1, MPI_INT,0, tag, MPI_COMM_WORLD, &st);
                MPI_Recv(&inicio, 1, MPI_INT,0, tag, MPI_COMM_WORLD, &st);
                MPI_Recv(&fim, 1, MPI_INT,0, tag, MPI_COMM_WORLD, &st);
                MPI_Recv(&janelaFiltro, 1, MPI_INT,0, tag, MPI_COMM_WORLD, &st);
                MPI_Recv(&visaoDetalhada, 1, MPI_INT,0, tag, MPI_COMM_WORLD, &st);

                if(visaoDetalhada){
                    printf(
                        "\n\nProcesso %d recebeu:"
                        "\n  Linhas originais : %d"
                        "\n  Colunas originais: %d"
                        "\n  Inicio           : %d"
                        "\n  Fim              : %d"
                        "\n  Tamanho filtro   : %d"
                        "\n  Visao Detalhada  : %d\n",
                        id,
                        qtd_linhas_original,
                        qtd_colunas_original,
                        inicio,
                        fim,
                        janelaFiltro,
                        visaoDetalhada
                    );
                }
                fflush(stdout);

                pgm->height = qtd_linhas_original;
                pgm->width = qtd_colunas_original;

                pgmParaleloAverage = malloc(sizeof(PGM));
                pgmParaleloMedian = malloc(sizeof(PGM));

                pgmParaleloAverage->height = fim - inicio + 1;
                pgmParaleloAverage->width = qtd_colunas_original;

                pgmParaleloMedian->height = fim - inicio + 1;
                pgmParaleloMedian->width = qtd_colunas_original;

                inicializa_PGM(pgm);
                inicializa_PGM(pgmParaleloAverage);
                inicializa_PGM(pgmParaleloMedian);

                // Recebe o PGM original
                for(i=0;i<qtd_linhas_original;i++){
                    MPI_Recv(pgm->data[i], qtd_colunas_original, MPI_UNSIGNED_CHAR, 0, tag, MPI_COMM_WORLD, &st);
                }

                // Realiza a paralelização com o filtro de média
                filtroMPIAverage(inicio, fim, janelaFiltro, id, visaoDetalhada);

                MPI_Send(&inicio, 1, MPI_INT, 0, tag, MPI_COMM_WORLD);
                MPI_Send(&fim, 1, MPI_INT, 0, tag, MPI_COMM_WORLD);
                for(j=0;j<(fim-inicio+1);j++){;
                    MPI_Send(pgmParaleloAverage->data[j], qtd_colunas_original, MPI_UNSIGNED_CHAR, 0, tag, MPI_COMM_WORLD);
                }

                liberarPGM(pgmParaleloAverage);

                MPI_Barrier(MPI_COMM_WORLD);

                // Realiza a paralelização com o filtro de mediana
                filtroMPIMedian(inicio, fim, janelaFiltro, id, visaoDetalhada);

                MPI_Send(&inicio, 1, MPI_INT, 0, tag, MPI_COMM_WORLD);
                MPI_Send(&fim, 1, MPI_INT, 0, tag, MPI_COMM_WORLD);
                for(j=0;j<(fim-inicio+1);j++){;
                    MPI_Send(pgmParaleloMedian->data[j], qtd_colunas_original, MPI_UNSIGNED_CHAR, 0, tag, MPI_COMM_WORLD);
                }

                liberarPGM(pgmParaleloMedian);

                MPI_Barrier(MPI_COMM_WORLD);
            }


        if(id==0){
            if(tolower(buffer[0]) == 'y'){
                loop+=1;
            }else{
                loop=0;
            }
        }

        MPI_Barrier(MPI_COMM_WORLD);

        MPI_Bcast(&loop, 1, MPI_INT, 0, MPI_COMM_WORLD);

    }while(loop);

    liberarPGM(pgm);

    MPI_Finalize();

    return 0;
}
