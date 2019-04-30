//FINAL
//  main.c
//  PKS_2 Komunikator
//
//  Created by Jozef Varga on 25.10.17.
//  Copyright © 2017 Jozef Varga. All rights reserved.
//

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<pthread.h>

#define Fragmentovana_hlavicka 48
#define MAX_fragment 1500
#define PRENOS_TEXTU 5000

int odhlasenie, prepnutie, spojenie = 0,zobrazovanie_fragmentov=0, odpojenie = 0;
pthread_t dalsie_vlakno;

struct argument_vlakna{
    int spojenie;
    int   blud;
    struct sockaddr_in si_server;
};

struct hlavicka_fragmentu{
    unsigned short typ;
    unsigned int index;
    unsigned int pocet_fragmentov;
    unsigned int velkost_fragmentu;
    unsigned short CRC;
};

void *vlakno_chyba_SERVER(void *vargq){
    sleep(3);
    prepnutie = 1;
    return NULL;
}

void *vlakno(struct argument_vlakna *argument){
    char inicializacia[sizeof(struct hlavicka_fragmentu)];
    struct hlavicka_fragmentu *hlavicka;
    hlavicka = (struct hlavicka_fragmentu*)inicializacia;
    hlavicka->typ = 22;
    hlavicka->CRC = 0;
    hlavicka->index = 0;
    hlavicka->velkost_fragmentu = 0;
    while(1){
        sendto(argument->spojenie, inicializacia, sizeof(struct hlavicka_fragmentu) , 0 , (struct sockaddr *) &argument->si_server, argument->blud);
        sleep(1);
    }
    return NULL;
}

void *vlakno_vypnutie(void *vargp){
    sleep(3);
    if(odhlasenie != 4){
        if(odhlasenie == 1){
            printf("Klient bol odpojeny!!!\n");
            if(odpojenie == 1)printf("Prijatie dat sa nepodarilo pre pokracovanie servera \nzadajte \\pokracuj\n");
            printf("-------------------------------------------------------------\n");
        }
        odhlasenie = 0;
        prepnutie = 0;
        return NULL;
    }
}

unsigned short getCRC(unsigned char *ukazovatel_na_data, unsigned int dlzka_spravy){
    unsigned int i;
    unsigned short CRC, x;
    unsigned char pomoc;
    CRC = 1;
    for( i=0; i<dlzka_spravy; i++ ){
        pomoc=ukazovatel_na_data[i];
        for( x=0; x<8; x++ ){
            if( CRC & 0x8000 ){
                CRC = (CRC<<1)| (pomoc & 1);
                CRC ^= 0x1083;
            }
            else
                CRC = (CRC<<1)| (pomoc & 1);
            pomoc >>= 1;
        }
    }
    return CRC;
}

void *vlakno_KLIENT(void *vargp){
    zobrazovanie_fragmentov = 0;
    int port = 0, i=0, x=0, fragment = 0;;
    struct sockaddr_in si_server;
    char ip_servera[15], enter;
    printf("===================> NASTAVENIE KLIENT <=====================\n");
    printf("Zadajte IP adresu servera: ");
    scanf("%s",ip_servera);
    printf("Zadajte cislo portu servera: ");
    scanf("%d",&port);
    printf("Zadajte velkost fragmentu spravy: ");
    scanf("%d",&fragment);
    while (fragment < Fragmentovana_hlavicka || fragment > MAX_fragment) {
        printf("Zadali ste neplatnu velkost.\nVelkost musi byt z rozsahu %d - %d\nZadajte velkost fragmentu: ",Fragmentovana_hlavicka + 1,MAX_fragment);
        scanf("%d",&fragment);
    }
    fragment -= Fragmentovana_hlavicka;
    printf("Zadajte ci sa ku kazdej sprave maju zobrazovat velkosti \nfragmentov. Ak ano zadajte 1, ak nie 0: ");
    scanf("%d%c",&zobrazovanie_fragmentov,&enter);
    printf("========================> KLIENT <===========================\n");
    printf("* ak na zaciatok vety zadate \"\\chyba\" zaslete chybny ramec\n");
    printf("* ak na zaciatok vety zadate \"\\koniec\" ukoncite klienta\n");
    printf("* ak na zaciatok vety zadate \"\\subor\" dostanete sa do menu\n  zasielania suboru\n");
    printf("=============================================================\n");
    if ( (spojenie=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
        printf("Chyba Socket\n");
        prepnutie = 3;
    }
    memset((char *) &si_server, 0, sizeof(si_server));
    si_server.sin_family = AF_INET;
    si_server.sin_port = htons(port);
    if (inet_aton(ip_servera , &si_server.sin_addr) == 0)
    {
        fprintf(stderr, "!!!!!!> Zle zadana IP servera <!!!!!!\n");
        prepnutie = 3;
    }
    struct hlavicka_fragmentu *hlavicka;
    unsigned int pocet_fragmentov = 0, chyba_ramec = 0, velkost_spravy = 0;
    unsigned char *ukazovatel_na_data;
    char inicializacia[sizeof(struct hlavicka_fragmentu)];
    char potvrdenie[sizeof(struct hlavicka_fragmentu)];
    char *sprava, meno_suboru[100];
    sprava = (char*)malloc(PRENOS_TEXTU*sizeof(char));
    char sprava_fragment[fragment + sizeof(struct hlavicka_fragmentu)];
    long velkost_subor, velkost_posielanej_spravy=0;
    int blud = sizeof(si_server);
    struct argument_vlakna *vlakno_argument;
    vlakno_argument = (struct argument_vlakna*)malloc(sizeof(struct argument_vlakna));
    vlakno_argument->blud = blud;
    vlakno_argument->si_server = si_server;
    vlakno_argument->spojenie = spojenie;
    pthread_t vlakno_chyba_server;
    while(1){
        chyba_ramec = 0;
        pthread_create(&dalsie_vlakno, NULL, vlakno, (void *)vlakno_argument);
        printf("Zadaj spravu : ");
        fgets(sprava,PRENOS_TEXTU,stdin);
        pthread_cancel(dalsie_vlakno);
        if(sprava[0]=='\\'&& sprava[1]=='c'&& sprava[2]=='h'&& sprava[3]=='y'&& sprava[4]=='b' && sprava[5] =='a'){
            pthread_create(&dalsie_vlakno, NULL, vlakno, (void *)vlakno_argument);
            printf("Zadaj spravu (chybny ramec): ");
            fgets(sprava,PRENOS_TEXTU,stdin);
            pthread_cancel(dalsie_vlakno);
            chyba_ramec = 1;
        }
        else if(sprava[0]=='\\'&& sprava[1]=='k'&& sprava[2]=='o'&& sprava[3]=='n'&& sprava[4]=='i' && sprava[5] =='e' && sprava[6] == 'c'){
            prepnutie = 3;
            break;
            
        }
        hlavicka = (struct hlavicka_fragmentu*)inicializacia;
        if(sprava[0]=='\\'&& sprava[1]=='s'&& sprava[2]=='u'&& sprava[3]=='b'&& sprava[4]=='o' && sprava[5] =='r'){
            pthread_create(&dalsie_vlakno, NULL, vlakno, (void *)vlakno_argument);
            printf("Zadajte prosim celu cestu k suboru: ");
            scanf("%s",meno_suboru);
            scanf("%c",&enter);
            pthread_cancel(dalsie_vlakno);
            FILE *subor;
            if((subor = fopen(meno_suboru,"rb+")) == NULL){
                printf("Subor neexistuje!\n");
                prepnutie = 3;
                break;
            }
            fseek(subor, 0, SEEK_END);
            velkost_subor = ftell(subor);
            rewind (subor);
            sprava = (char*)malloc(velkost_subor*sizeof(char));
            fread (sprava,1, velkost_subor,subor);
            velkost_posielanej_spravy = velkost_subor;
            fclose(subor);
            if(meno_suboru[strlen(meno_suboru)-3] == 'j' && meno_suboru[strlen(meno_suboru)-2] == 'p' && meno_suboru[strlen(meno_suboru)-1] == 'g')hlavicka->typ = 5;
            else if(meno_suboru[strlen(meno_suboru)-3] == 'p' && meno_suboru[strlen(meno_suboru)-2] == 'n' && meno_suboru[strlen(meno_suboru)-1] == 'g')hlavicka->typ = 6;
            else if(meno_suboru[strlen(meno_suboru)-3] == 't' && meno_suboru[strlen(meno_suboru)-2] == 'x' && meno_suboru[strlen(meno_suboru)-1] == 't')hlavicka->typ = 7;
            else if(meno_suboru[strlen(meno_suboru)-3] == 'p' && meno_suboru[strlen(meno_suboru)-2] == 'd' && meno_suboru[strlen(meno_suboru)-1] == 'f')hlavicka->typ = 8;
            else if(meno_suboru[strlen(meno_suboru)-3] == 'p' && meno_suboru[strlen(meno_suboru)-2] == 'p' && meno_suboru[strlen(meno_suboru)-1] == 't')hlavicka->typ = 9;
            else if(meno_suboru[strlen(meno_suboru)-2] == '.' && meno_suboru[strlen(meno_suboru)-1] == 'c')hlavicka->typ = 10;
            else if(meno_suboru[strlen(meno_suboru)-4] == 'd' && meno_suboru[strlen(meno_suboru)-3] == 'o' && meno_suboru[strlen(meno_suboru)-2] == 'c' && meno_suboru[strlen(meno_suboru)-1] == 'x')hlavicka->typ = 11;
        }else {
            hlavicka->typ = 0;
            velkost_posielanej_spravy = strlen(sprava);
        }
        if(velkost_posielanej_spravy>fragment)pocet_fragmentov =(int)velkost_posielanej_spravy/(int)fragment + 1;
        else pocet_fragmentov = 1;
        hlavicka->pocet_fragmentov = pocet_fragmentov;
        hlavicka->index = (unsigned int)velkost_posielanej_spravy;
        hlavicka->velkost_fragmentu = fragment;
        hlavicka->CRC = 0;
        hlavicka->CRC = getCRC((unsigned char *)inicializacia, sizeof(struct hlavicka_fragmentu));
        if (sendto(spojenie, inicializacia, sizeof(struct hlavicka_fragmentu) , 0 , (struct sockaddr *) &si_server, blud) == -1){;prepnutie = 3;}
        pthread_create(&vlakno_chyba_server, NULL, vlakno_chyba_SERVER, NULL);
        if (recvfrom(spojenie, potvrdenie, sizeof(struct hlavicka_fragmentu), 0, (struct sockaddr *) &si_server, &blud) == -1){prepnutie = 3;}
        pthread_cancel(vlakno_chyba_server);
        if(strcmp(inicializacia, potvrdenie) == 0){
            for(i=0;i<pocet_fragmentov;i++){
                if(fragment * (i+1)<velkost_posielanej_spravy)velkost_spravy = fragment;
                else velkost_spravy = (unsigned int)(fragment - ((fragment * (i+1)) - velkost_posielanej_spravy));
                do {
                    hlavicka = (struct hlavicka_fragmentu*)sprava_fragment;
                    hlavicka->typ = 1;
                    hlavicka->index = i;
                    hlavicka->pocet_fragmentov = pocet_fragmentov;
                    hlavicka->velkost_fragmentu = velkost_spravy;
                    ukazovatel_na_data = (unsigned char*)sprava_fragment;
                    if(zobrazovanie_fragmentov == 1)printf("Fragment %d ma velkost: %d \n",i + 1, velkost_spravy + Fragmentovana_hlavicka);
                    for(x=0;x<velkost_spravy;x++){
                        sprava_fragment[sizeof(struct hlavicka_fragmentu) + x] = sprava[(fragment * i) + x];
                    }
                    hlavicka->CRC = 0;
                    hlavicka->CRC = getCRC(ukazovatel_na_data, velkost_spravy) + chyba_ramec;
                    chyba_ramec = 0;
                    sendto(spojenie, sprava_fragment, velkost_spravy+sizeof(struct hlavicka_fragmentu) , 0 , (struct sockaddr *) &si_server, blud);
                    pthread_create(&vlakno_chyba_server, NULL, vlakno_chyba_SERVER, NULL);
                    recvfrom(spojenie, potvrdenie, sizeof(struct hlavicka_fragmentu), 0, (struct sockaddr *) &si_server, &blud);
                    pthread_cancel(vlakno_chyba_server);
                    hlavicka = (struct hlavicka_fragmentu*)potvrdenie;
                } while (hlavicka->typ != 4);
                
            }
        }
        printf("-------------------------------------------------------------\n");
    }
    prepnutie = 3;
    return NULL;
}

void *vlakno_SERVER(int port){
    short typ_spravy = 0;
    odhlasenie = 0;
    odpojenie = 0;
    int i=0, x=0, klient, fragment = 0;
    struct hlavicka_fragmentu *hlavicka;
    struct sockaddr_in si_klient, si_server;
    unsigned int velkost_klienta = sizeof(si_klient);
    if ((spojenie=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){printf("socket");return 0;}
    memset((char *) &si_server, 0, sizeof(si_server));
    si_server.sin_family = AF_INET;
    si_server.sin_port = htons(port);
    si_server.sin_addr.s_addr = htonl(INADDR_ANY);
    if( bind(spojenie , (struct sockaddr*)&si_server, sizeof(si_server) ) == -1){printf("Port je vyuzivaný inou aplikaciou ");return 0;}
    unsigned int pocet_fragmentov, ukoncenie=0, velkost_suboru;
    unsigned short CRC;
    char inicializacia[sizeof(struct hlavicka_fragmentu)];
    char *meno_suboru;
    meno_suboru = (char*)malloc(15*sizeof(char));
    while(1){
        fflush(stdout);
        odpojenie = 0;
        prepnutie = 0;
        (klient = (int)(recvfrom(spojenie, inicializacia, sizeof(struct hlavicka_fragmentu), 0, (struct sockaddr *) &si_klient, &velkost_klienta)));
        hlavicka = (struct hlavicka_fragmentu*)inicializacia;
        if(hlavicka->typ == 22 && odhlasenie == 0){
            printf("Klient bol pripojeny!!!\n");
            printf("-------------------------------------------------------------\n");
        }
        else pthread_cancel(dalsie_vlakno);
        pthread_create(&dalsie_vlakno, NULL, vlakno_vypnutie, NULL);
        odhlasenie = 1;
        odpojenie = 1;
        if(hlavicka->typ == 22)continue;
        velkost_suboru = hlavicka->index;
        CRC = hlavicka->CRC;
        hlavicka->CRC = 0;
        if(getCRC((unsigned char *)inicializacia, sizeof(struct hlavicka_fragmentu)) == CRC || CRC == 0){
            if(CRC == 0)continue;
            prepnutie = 1;
            fragment = hlavicka->velkost_fragmentu;
            char *sprava = (char*)malloc((hlavicka->index + 1)*sizeof(char));
            char *sprava_fragment = (char*)malloc((fragment + sizeof(struct hlavicka_fragmentu))*sizeof(char));
            pocet_fragmentov = hlavicka->pocet_fragmentov;
            if(hlavicka->typ == 0 || (hlavicka->typ >= 5 && hlavicka->typ <=11)){
                if(hlavicka->typ >= 5){
                    typ_spravy = 1;
                    switch (hlavicka->typ) {
                        case 5: meno_suboru = "prijate.jpg";break;
                        case 6: meno_suboru = "prijate.png";break;
                        case 7: meno_suboru = "prijate.txt";break;
                        case 8: meno_suboru = "prijate.pdf";break;
                        case 9: meno_suboru = "prijate.ppt";break;
                        case 10: meno_suboru = "prijate.c";break;
                        case 11: meno_suboru = "prijate.docx";break;
                    }
                }else typ_spravy = 0;
                //if(pocet_fragmentov>10)odhlasenie = 4;
                sendto(spojenie, inicializacia, sizeof(struct hlavicka_fragmentu), 0, (struct sockaddr*) &si_klient, sizeof(si_klient));
                pthread_cancel(dalsie_vlakno);
                for(i=0;i<pocet_fragmentov;i++){
                    odpojenie = 1;
                    pthread_create(&dalsie_vlakno, NULL, vlakno_vypnutie, NULL);
                    klient = (int)(recvfrom(spojenie, sprava_fragment, fragment + sizeof(struct hlavicka_fragmentu), 0, (struct sockaddr *) &si_klient, &velkost_klienta));
                    pthread_cancel(dalsie_vlakno);
                    if(hlavicka->typ == 22)continue;
                    hlavicka = (struct hlavicka_fragmentu*)sprava_fragment;
                    CRC = hlavicka->CRC;
                    if(zobrazovanie_fragmentov == 1 && odhlasenie != 0)printf("Fragment %d ma velkost: %d \n",hlavicka->index + 1, hlavicka->velkost_fragmentu + Fragmentovana_hlavicka);
                    hlavicka->CRC = 0;
                    if(CRC != (int)getCRC((unsigned char *)sprava_fragment, hlavicka->velkost_fragmentu)){
                        i--;
                        if(odhlasenie != 0)printf("!!!Chyba v kontrolnom sucte!!!\n");
                        sendto(spojenie, inicializacia, sizeof(struct hlavicka_fragmentu), 0, (struct sockaddr*) &si_klient, sizeof(si_klient));
                        continue;
                    }
                    for(x=0;x<hlavicka->velkost_fragmentu;x++){
                        sprava[hlavicka->index*fragment + x] = sprava_fragment[x + sizeof(struct hlavicka_fragmentu)];
                    }
                    ukoncenie = hlavicka->index*fragment + x-1;
                    hlavicka = (struct hlavicka_fragmentu*)inicializacia;
                    hlavicka->typ = 4;
                    sendto(spojenie, inicializacia, sizeof(struct hlavicka_fragmentu), 0, (struct sockaddr*) &si_klient, sizeof(si_klient));
                }
                if(typ_spravy == 0){
                    sprava[ukoncenie] = '\0';
                    if(odhlasenie != 0){
                        printf("Prisla sprava od: %s : %d\n", inet_ntoa(si_klient.sin_addr), ntohs(si_klient.sin_port));
                        printf("Sprava: %s\n" , sprava);
                        printf("-------------------------------------------------------------\n");
                    }
                }else{
                    if(odhlasenie != 0){
                        printf("Prisiel subor od: %s : %d\n", inet_ntoa(si_klient.sin_addr), ntohs(si_klient.sin_port));
                        printf("Subor najdete pri aplikacii pod menom prijate\n");
                        FILE *subor;
                        subor = fopen(meno_suboru,"w+");
                        fwrite(sprava, 1, velkost_suboru, subor);
                        printf("-------------------------------------------------------------\n");
                        fclose(subor);
                    }
                }
            }
        }
    }
    return NULL;
}

int main() {
    while(1){
        odhlasenie = 0;
        int menu = 0;
        //printf("%d",sizeof(struct hlavicka_fragmentu));
        printf("=============================================================\n");
        printf("Vyberte o ktoru funkciu mate zaujem :\n");
        printf("1 -Server\n");
        printf("2 -Klient\n");
        printf("0 -Koniec aplikacie\n");
        printf("=============================================================\n");
        printf("Zadajte cislo vasej volby: ");
        scanf("%d",&menu);
        if(menu == 1){
            prepnutie = 0;
            pthread_t server;
            int port=0;
            printf("===================> NASTAVENIE SERVER <=====================\nZadajte cislo portu na ktorom ma server pocuvat: ");
            scanf("%d",&port);
            printf("Zadajte ci sa ku kazdej sprave maju zobrazovat velkosti \nfragmentov. Ak ano zadajte 1, ak nie 0: ");
            scanf("%d",&zobrazovanie_fragmentov);
            printf("========================> SERVER <===========================\n");
            printf("* ak na zaciatok vety zadate \"\\koniec\" ukoncite server\n");
            printf("* ak na zaciatok vety zadate \"\\pokracuj\" server sa obnovy\n  (napr ak klient prestane vysielat)\n");
            printf("=============================================================\n");
            pthread_create(&server, NULL, vlakno_SERVER, (void *)port);
            char *sprava;
            sprava = (char*)malloc(50*sizeof(char));
            memset(sprava, '\0',50);
            while(1){
                fflush(stdin);
                fgets(sprava,50,stdin);
                if(sprava[0]=='\\'&& sprava[1]=='p'&& sprava[2]=='o'&& sprava[3]=='k'&& sprava[4]=='r' && sprava[5] =='a' && sprava[6] == 'c' && sprava[7] == 'u' &&sprava[8] == 'j'){
                    prepnutie = 0;
                    pthread_cancel(server);
                    pthread_cancel(dalsie_vlakno);
                    close(spojenie);
                    pthread_create(&server, NULL, vlakno_SERVER, (void *)port);
                }
                if(sprava[0]=='\\'&& sprava[1]=='k'&& sprava[2]=='o'&& sprava[3]=='n'&& sprava[4]=='i' && sprava[5] =='e' && sprava[6] == 'c' && prepnutie == 0)break;
                else if(sprava[0]=='\\'&& sprava[1]=='k'&& sprava[2]=='o'&& sprava[3]=='n'&& sprava[4]=='i' && sprava[5] =='e' && sprava[6] == 'c' && prepnutie == 1)printf("Server sa neda ukoncit nakolko prebieha prijimanie\n");
            }
            odhlasenie = 3;
            pthread_cancel(server);
            pthread_cancel(dalsie_vlakno);
            close(spojenie);
        }
        else if(menu == 2){
            prepnutie = 0;
            pthread_t klient;
            pthread_create(&klient, NULL, vlakno_KLIENT, NULL);
            while(1){
                if(prepnutie == 1){
                    printf("Server nie je aktivny\n");
                    break;
                }
                if(prepnutie == 3)break;
            }
            pthread_cancel(klient);
            pthread_cancel(dalsie_vlakno);
            close(spojenie);
        }
        else return 0;
    }
    printf("KONIEC APLIKACIE\n");
    return 0;
}

