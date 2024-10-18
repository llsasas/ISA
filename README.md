# Dokumentace - ISA TFTP 
## Autor: Samuel Čus (xcussa00)
## Datum 19.11. 2023

### Popis

Program je implementací serverové a klientské aplikace TFTP protokolu. Jak klient tak server po spuštění zpracují zadané parametry a provede se TFTP přenos. Buď se přenáší soubor z stdin klienta na server a nebo klient stahuje soubor ze serveru. Na standartní chybový výstup se vypisují zprávy dle požadavků v zadání. 

### Seznam souborů
    Makefile    
    tftp-client.c     
    tftp-client.h      
    tftp-server.c
    tftp-server.h
    messages.c
    messages.h
    README.md
    manual.pdf    

### Překlad
    
    $make           - překlad projektu
    $make server    - překlad pouze server
    $make client    - překlad pouze klient
    $make clean     - vymaž přeložený projekt

### Spuštění

**Klient**

tftp-client -h hostname [-p port] [-f filepath] -t dest_filepath

-h IP adresa/doménový název vzdáleného serveru
-p port vzdáleného serveru, nepovinný argument, pokud není specifikován předpokládá se výchozí dle specifikace
-f cesta ke stahovanému souboru na serveru (download) - pokud není specifikován používá se obsah stdin (upload)
-t cesta, pod kterou bude soubor na vzdáleném serveru/lokálně uložen

**Klient příklad**

./tftp-client -h hostname -p 1656 -t < klient/mkd.txt dest_filepath/mkd.txt

-h IP adresa/doménový název vzdáleného serveru
-p port vzdáleného serveru hodnota 1656
-f není specifikován používá se obsah stdin (upload)
-t cesta, pod kterou bude soubor na vzdáleném serveru

**Server příklad**

./tftp-server 1656 root_dirpath

-p 1656, server nám RRQ a WRQ zprávy přijímá na portu 1656
root_dirpath: cesta k adresáři, pod kterým se budou ukládat příchozí soubory

**Server**

tftp-server [-p port] root_dirpath

-p místní port, na kterém bude server očekávat příchozí spojení
root_dirpath: je cesta k adresáři, pod kterým se budou ukládat příchozí soubory

### Rozšíření/Obmezení
V projektu nebyly implementovány oproti zadání žádná rozšíření. Taktéž projekt není nijak obmedzen a měl by tedy splňovat zadání v plném rozsahu.