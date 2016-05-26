/* Magdalena Molenda
   nr albumu: 345746 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include "err.h"

//Łącza do komunikacji z dzieckiem
int send[2], get[2];
//Tablica pomocnicza do wypisywania obliczonych wartości na standardowe wyjście.
char message[11];
//n - żądana liczba procesów W; tmp - zmienna pomocnicza do wysyłania oraz odczytywania danych z łącza
long n, tmp;

//Sprawdza poprawność argumentów.
int checkArguments(int argc, char *argv[])
{
  if (argc != 2)
  {
    fprintf(stderr, "Incorrect number of arguments.\n");
    return -1;
  }
  
  n = atoi(argv[1]);
  
  if (n < 1)
  {
    fprintf(stderr, "Incorrect argument. Enter a number greater that 0.\n");
    return -1;
  }
  
  return 0;
}

//Tworzy łącze pomiędzy rodzicem i dzieckiem.
void createPipes()
{
  if (pipe(send) == -1)
    syserr("Error in pipe send\n");
  
  if (pipe(get) == -1)
    syserr("Error in pipe get\n");
}

//Wprowadza odpowiednie ustawienia łącza w dziecku.
void setChildPipes()
{
  if (close(send[1]) == -1) 
    syserr("Error in closing send[1] in child\n");
     
  if (close(get[0]) == -1) 
    syserr("Error in closing get[0] in child\n");
  //Zamknięcie standardowego wejścia, w celu podmiany go na uprzednio utworzone łącze.    
  if (close(0) == -1) 
    syserr("Error in closing std input\n");
  //Zamknięcie standardowego wyjścia, w celu podmiany go na uprzednio utworzone łącze.  
  if (close(1) == -1)
    syserr("Error in closing std output\n");
      
  if (dup(send[0]) != 0) 
    syserr("Error in duplicating send[0]\n");
      
  if (dup(get[1]) != 1) 
    syserr("Error in duplicating get[1]\n");
      
  if (close(send[0]) == -1) 
    syserr("Error in closing send[0] in child\n");
     
  if (close(get[1]) == -1)
    syserr("Error in closing get[1] in child\n");
}

//Wprowadza odpowiednie ustawienia łącza w rodzicu.
void setParentPipes()
{
  if (close(send[0]) == -1)
    syserr("Error in closing send[0] in parent\n");
     
  if (close(get[1]) == -1)
    syserr("Error in closing get[1] in parent\n");
}

//Wysyła dane do procesu W.
void sendDataToW()
{ 
  if (write(send[1], &n, sizeof(n)) == -1) 
    syserr("Error in sending 'n' to W\n");
      
  tmp = 1;
  
  long j;
  //Przesłanie n jedynek - każda z ich inicjuje kolejny krok obliczeń.
  for (j=1; j<=n; j++)
  {
    if (write(send[1], &tmp, sizeof(tmp)) == -1)
      syserr("Error in sending data to child\n");
  }
      
  tmp = -1;
  //Przesłanie wartości -1, jako sygnału końca obliczeń.
  if (write(send[1], &tmp, sizeof(tmp)) == -1)
    syserr("Error in sending data to child\n");
      
  if (close(send[1]) == -1)
    syserr("Error in closing send[1] in parent\n");
}

//Odbiera dane od procesu W i wypisuje je na standardowe wyjście.
void getResultsFromW()
{
  int r;
  while ((r = read(get[0], &tmp, sizeof(tmp))) != 0)
  {
    if (r == -1)
      syserr("Error in reading data from W\n");
    
    //Wpisanie otrzymanej od potomka wartości do tablicy charów i wypisanie jej na standardowe wyjście.
    sprintf(message, "%ld ", tmp);
    if (write(1, &message, sizeof(tmp)) == -1)
      syserr("Error in printing data\n");
  }
    if (close(get[0]) == -1)
	syserr("Error in closing get[0] in parent\n");
      
    char end = '\n';
    if (write(1, &end, sizeof(end)) == -1)
      syserr("Error in writing new line in parent\n");
}

//Tworzy proces potomny.
void splitProcess()
{
  switch (fork())
  {
    case -1: 
      syserr("Error in fork\n");
      break;
      
    //Proces potomny.  
    case 0:
      
      setChildPipes();
      
      //Uruchomienie procesu W.
      if (execl("./W", "W", (char *) 0) == -1)
	syserr("Error in executing W\n");
      
      break;
      
    //Proces macierzysty.
    default:
      
      setParentPipes();
      sendDataToW();
      getResultsFromW();
      
      if (wait(0) == -1)
	syserr("Error in waiting for child\n");
  }
}

int main (int argc, char *argv[])
{ 
  if (checkArguments(argc, argv) == -1)
    return -1;
  
  createPipes();
  splitProcess();
  
  return 0;
}