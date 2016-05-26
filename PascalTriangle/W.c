/* Magdalena Molenda
   nr albumu: 345746 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include "err.h"

//Łącza do komunikacji z dzieckiem (send[1], get[0] - u dziecka pozostałe)
int send[2], get[2];
//Łącza do komunikacji z rodzicem
int backget, backsend;
/*tmp - zmienna pomocnicza do wysyłania oraz odczytywania danych z łącza;
  value - wartość obliczana dla każdego z procesów; beginNr - żądana liczba procesów; processNr - numer każdego z procesów*/
long tmp, value, beginNr, processNr;

//Inicjalizuje zmienne.
void setvalues()
{
  //Odczyt żądanej liczby procesów, przysłanej z procesu Pascal.
  if (read(0, &beginNr, sizeof(beginNr)) == -1)
    syserr("Error in read in process nr %d\n", getpid());

  value = 0;
  backget = 0;
  backsend = 1;
  processNr = 1; 
}

//Tworzy łącze pomiędzy rodzicem i dzieckiem.
void createPipes()
{
  if (pipe(send) == -1)
    syserr("Error in pipe send\n");
  
  if (pipe(get) == -1)
    syserr("Error in pipe get\n");
}

//Wprowadza odpowiednie ustawienia łącza w rodzicu.
void setParentPipes()
{
  if (close(send[0]) == -1) //0 do czytania
    syserr("Error in closing send[0] in process nr %d\n", getpid());
     
  if (close(get[1]) == -1) //1 do pisania
    syserr("Error in closing send[0] in process nr %d\n", getpid());
}

//Wprowadza odpowiednie ustawienia łącza w dziecku.
void setChildPipes()
{
  if (close(send[1]) == -1)
    syserr("Error in closing send[1] in process nr %d\n", getpid());
     
  if (close(get[0]) == -1)
    syserr("Error in closing get[0] in process nr %d\n", getpid());

  if (backsend != 1)
    if (close(backsend) == -1)
      syserr("Error in closing backsend in process nr %d\n", getpid());
     
  if (backget != 0)
    if (close(backget) == -1)
      syserr("Error in closing backget in process nr %d\n", getpid());
      
  backget = send[0];
  backsend = get[1];
}

//Dotyczy procesu W(n). Odbiera wszystkie obliczone wartości i wysyła je w drogę powrotną.
void sendBackAllvalues()
{
  int r;
  while ((r = read(backget, &tmp, sizeof(tmp))) != 0)
  {
    if (r == -1)
      syserr("Error in read in process nr %d\n", getpid());
    
    /*Jeśli wartość W(n) nie jest jeszcze obliczona, proces przyjmuje otrzymaną liczbę jako swoją wartość,
      W.p.p. to, co otrzymał, jest jedną z końcowych wartości poprzednich procesów i należy przesłać to do rodzica.*/
    if (tmp != -1)
    {
      if (value == 0)
	value = tmp;
      else if (tmp != -1)
	if (write(backsend, &tmp, sizeof(tmp)) == -1)
	  syserr("Error in write in process nr %d\n", getpid());
    }
    //Jeśli nadszedł sygnał do zakończenia obliczeń (wartość "-1"), proces wysyła swoją obliczoną wartość.
    else
      if (write(backsend, &value, sizeof(tmp)) == -1)
	syserr("Error in write in process nr %d\n", getpid());
  }
  
  if (close(backget) == -1)
    syserr("Error in close in process nr %d\n", getpid());

  if (close(backsend) == -1)
    syserr("Error in close in process nr %d\n", getpid());
}

//Odbiera dane od swojego potomka i przesyła je do swojego rodzica.
void fromChildToParent()
{
  int r;
  while ((r = read(get[0], &tmp, sizeof(tmp))) != 0)
  {
    if (r == -1)
      syserr("Error in read in process nr %d\n", getpid());
    
    if (write(backsend, &tmp, sizeof(tmp)) == -1)
      syserr("Error in write in process nr %d\n", getpid());
  }
  
  if (close(backsend) == -1)
    syserr("Error in close in process nr %d\n", getpid());
      
  if (close(get[0]) == -1)
    syserr("Error in close in process nr %d\n", getpid());
}

//Dotyczy W(1). Odbiera dane od procesu Pascal i przesyła je do swojego potomka.
void fromPascalToChild()
{
  int r;
  while ((r = read(0, &tmp, sizeof(tmp))) != 0)
  {
    if (r == -1)
      syserr("Error in read in process nr %d\n", getpid());
    
    //Jeżeli wartość procesu nie jest jeszcze obliczona, przyjmuje ją jako otrzymaną liczbę.
    if (value == 0)
      value = tmp;
    
    else
      if (write(send[1], &tmp, sizeof(tmp)) == -1)
        syserr("Error in write in process nr %d\n", getpid());
   
    //Jeżeli proces odebrał sygnał do zakończenia obliczeń, wysyła swoją obliczoną wartość.
    if (tmp == -1)
      if (write(send[1], &value, sizeof(value)) == -1)
        syserr("Error in write in process nr %d\n", getpid());
  }
 
  if (close(send[1]) == -1)
    syserr("Error in closing send[0] in process nr %d\n", getpid());
}

//Dotyczy procesów { W(2)...W(n-1) }. Odbiera dane od swojego rodzica i przesyła je do swojego potomka.
void fromParentToChild()
{
  //flag - zmienna wskazuje, czy został już wcześniej odebrany sygnał do zakończenia obliczeń, czy nie.
  int r, flag = 0;
  while ((r = read(backget, &tmp, sizeof(tmp))) != 0)
  {
    if (r == -1)
      syserr("Error in read in process nr %d\n", getpid());

    //Jeśli nie otrzymano jeszcze sygnału do zakończenia obliczeń, wartość dla procesu jest uaktualniana na podstawie otrzymanej liczby.
    if ((tmp != -1) && (flag == 0))
    {
      if (value != 0)
	if (write(send[1], &value, sizeof(tmp)) == -1)
	  syserr("Error in write in process nr %d\n", getpid());
	
      value += tmp;
    }
    
    //Jeśli otrzymano sygnał kończący obliczenia, wszystkie następne otrzymane wartości wysyłane są prosto dalej.
    else
    {
      flag = 1; 
	    
      if (write(send[1], &tmp, sizeof(tmp)) == -1)
	syserr("Error in write in process nr %d\n", getpid());
	    
      if (tmp == -1)
	if (write(send[1], &value, sizeof(tmp)) == -1)
	  syserr("Error in write in process nr %d\n", getpid());
    }
  }
       
  if (close(backget) == -1)
    syserr("Error in close in process nr %d\n", getpid());

  if (close(send[1]) == -1)
    syserr("Error in close in process nr %d\n", getpid());  
}

//Tworzy proces potomny.
void splitProcess(long processNr)
{
  switch (fork())
  {
    case -1:
      syserr("Error in fork in process nr %d\n", getpid());
      break;
     
    //Proces potomny  
    case 0:
      processNr++;
      setChildPipes();

      //Proces W(N)
      if (processNr == beginNr)
        sendBackAllvalues();
      
      else
      {
	createPipes();
	splitProcess(processNr);
      }
      
      break;
	
    //Proces macierzysty  
    default:
     setParentPipes();
     
     //Proces W(1)
     if (processNr == 1)
       fromPascalToChild();

     else
       fromParentToChild();
       
     fromChildToParent();
     
     if (wait(0) == -1)
       syserr("Error in waiting in process nr %d\n", getpid());
  }
}

int main (int argc, char *argv[])
{
  setvalues();
  
  //Gdy n == 1, pierwszy proces jest zarówno procesem ostatnim.
  if (beginNr == 1)
    sendBackAllvalues();

  else
  { 
    createPipes();
    splitProcess(processNr);
  }

  return 0; 
}