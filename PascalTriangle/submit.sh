#!/bin/sh

if [ $# -le 2 ]; then
  echo submit: Uzywaj submit adresat numer-zadania pliki_katalogi_do_wyslania
else
  ADR=$1
  ZAD=$2  
  shift 2
  tar cf - $* | uuencode `whoami`-${ZAD}.tar | \
              mail -s `whoami`-zad$ZAD -c `whoami` $ADR
fi