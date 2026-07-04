#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

typedef struct PGM
{
  char type[3];
  unsigned char **data;
  unsigned int width;
  unsigned int height;
  unsigned int grayvalue;
  double time;
} PGM;

typedef struct tipoPack
{
  int tid,ini, fim,linTotal,colTotal,tamFiltro, tamFiltro2;
} tipoPack;

PGM *pgm;
PGM *pgmParalelo;
PGM *pgmSequencial;

void ignoreComments(FILE *fp)
{
  int ch;
  char line[100];
  // Ignore any blank lines
  while ((ch = fgetc(fp)) != EOF && isspace(ch))
    ;
  // Recursively ignore comments in a PGM image
  // commented lines start with a '#'
  if (ch == '#')
  {
    fgets(line, sizeof(line), fp);
    // To get cursor to next line
    ignoreComments(fp);
  }
  // check if anymore comments available
  else
  {
    fseek(fp, -1, SEEK_CUR);
  }
  // Beginning of current line
}

int openPGM(PGM *pgm, char fname[])
{
  FILE *pgmfile = fopen(fname, "rb");
  if (pgmfile == NULL)
  {
    printf("File does not exist\n");
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
    printf("No extension found in file %s", filename);
  else // portion after .
    printf("File format     : %s\n", ext + 1);

  printf("PGM File type   : %s\n", pgm->type);
  printf("Width of img    : %d px\n", pgm->width);
  printf("Height of img   : %d px\n", pgm->height);
  printf("Max Gray value  : %d\n", pgm->grayvalue);

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
int cmp(const void *a, const void *b)
{
  return (*(int *)a - *(int *)b);
}

void *filtroThread(void *ptr){
  int i, j, k, l, d, e, m, p;
  tipoPack *pack;

  pack = (tipoPack*) ptr;

  int *a = (int *)malloc(pack->tamFiltro2 * sizeof(int));

  //printf("Thread %i: Executando linha %i.\n",pack->tid, pack->lin);
  //Dog_BeforeFilter.pgm
  for(i=pack->ini;i<=pack->fim;i++)
  {
    for (j = 0; j < pgm->width; j++)
    {
      d = 0;
      e = 0;
      m = 0;

      for (k = -pack->tamFiltro; k <= pack->tamFiltro; k++)
      {
        for (l = -pack->tamFiltro; l <= pack->tamFiltro; l++)
        {
          if (i+ k > 0 && j + l > 0 && i + k < pack->linTotal && j + l < pack->colTotal)
          {
            a[m] = (int)pgm->data[i + k][j + l];
            m++;
          }
        }
      }
      qsort(a, m, sizeof(int), cmp);
      p = a[m / 2];
      // e = e / d;
      pgmParalelo->data[i][j] = (char)p;
    }
  }
  free(a);
}

PGM *filtroParalelo(PGM *pgm, int filt_size)
{
  int i, n_int, n_threads, n_resto, aux_ini, aux_fim;
  clock_t time_ini, time_fim;

  n_threads=0;
  do{
    printf("\nQuantas Threads deseja? (>=1 e <=%i)=> ", pgm->height);
    scanf("%d",&n_threads);
  }while(n_threads<1 || n_threads>pgm->height);

  time_ini = clock();

  pthread_t thread[pgm->height];
  int iret[pgm->height];
  tipoPack pack[pgm->height];

  pgmParalelo = malloc(sizeof(PGM));

  strcpy(pgmParalelo->type, pgm->type);
  pgmParalelo->height = pgm->height;
  pgmParalelo->width = pgm->width;
  pgmParalelo->grayvalue = pgm->grayvalue;
  pgmParalelo->data = malloc(pgm->height * sizeof(unsigned char *));

  for (int i = 0; i < pgm->height; i++)
  {
    pgmParalelo->data[i] = malloc(pgm->width *
                           sizeof(unsigned char));
  }

  // Define a parte inteira e os restos da divisão de trabalho das threads
  n_int = pgm->height/n_threads;
  n_resto = pgm->height % n_threads;

  // Aux_inicio determina o início do intervalo de cálculo de uma thread
  aux_ini = 0;
	for(i=0; i<n_threads; i++){
		// Esse if/esle controla se existe um resto a ser adicionado ao intervalo das threads
		if (n_resto > 0){
			aux_fim = 1;
			n_resto--;
		}else if (n_int == 0){
				aux_ini= 0;
				aux_fim = 1;
			}else{
				aux_fim = 0;
			}

    pack[i].tid = i+1;
		pack[i].ini = aux_ini;
		pack[i].fim = aux_ini - 1 + n_int + aux_fim;

    pack[i].linTotal = pgm->height;
    pack[i].colTotal = pgm->width;
    pack[i].tamFiltro = filt_size / 2;
    pack[i].tamFiltro2 = filt_size * filt_size;

    aux_ini =  pack[i].fim + 1;
  }

  for(i=0; i<n_threads;i++){
    iret[i] = pthread_create(&thread[i], NULL, filtroThread, (void*) &pack[i]);
  }

  for (int i = 0; i <  pgm->height; i++)
  {
    pthread_join(thread[i], NULL);
    //printf("Thread %i finalizada.\n",pack[i].tid);
  }

  time_fim = clock();

  pgmParalelo->time = ((double)(time_fim - time_ini))/CLOCKS_PER_SEC;

  return pgmParalelo;
}

PGM *filtroSequencial(PGM *pgm, int filt_size)
{
  clock_t time_ini, time_fim;

  time_ini = clock();

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
  }else{
    printf("\nExistem divergencias entre as matrizes Sequencial e Paralela.\n");
  }

}

int main()
{
  pgm = malloc(sizeof(PGM));
  printf("Enter the PGM file name with extension\n");
  char ch[30];
  scanf("%s", ch);
  if (openPGM(pgm, ch))
  {
    printImageDetails(pgm, ch);
    int choice;
    printf("Enter the size of the filter window\n");
    scanf("%d", &choice);
    if (choice && 1)
    {
      PGM *paraleloFiltrado = filtroParalelo(pgm, choice);
      saveImage(paraleloFiltrado, "FiltroParalelo.pgm");

      PGM *sequencialFiltrado = filtroSequencial(pgm, choice);
      saveImage(sequencialFiltrado, "FiltroSequencial.pgm");

      printf("\nA imagem foi Filtrada.\n");

      comparaMatriz(paraleloFiltrado, sequencialFiltrado);

      printf("\nTempo de execucao Filtro Sequencial: %f", sequencialFiltrado->time);
      printf("\nTempo de execucao Filtro Paralelo: %f\n", paraleloFiltrado->time);

      free(paraleloFiltrado->data);
      free(paraleloFiltrado);

      free(sequencialFiltrado->data);
      free(sequencialFiltrado);
    }
    else
    {
      printf("Wrong size for the filter window\n");
      exit(EXIT_FAILURE);
    }
  }
  free(pgm->data);
  free(pgm);
  return 0;
}
