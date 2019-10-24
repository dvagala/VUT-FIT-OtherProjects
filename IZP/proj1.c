#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_CHAR 100                                            //max. počet znakov na jeden riadok - ostatné oreže
#define MAX_ADRS 42                                             //max. počet adries - ostatné oreže

void load_file(int words[MAX_ADRS+1][MAX_CHAR+1], bool error_code[4])               //čítanie znakov z stdin do words
{
    int i = 0;
    for (int j = 0; (words[i][j] = getchar()) != EOF; ++j)      //nahráva znaky z stdin do words, ak narazí na EOF skončí
    {
        if(words[i][j] >= 0 && words[i][j]<=31)                 //31 = horná hranica netlačiteľých znakov
            if(words[i][j] != '\n')                             //ak je vo words netlačiteľný znak, okrem line feed
            {                                                   //zmení error_code a daný znak preskočí
                error_code[3] = true;
                j--;
            }

        if(isalpha(words[i][j]))                                //všetky znaky prevedie ne veľké
            words[i][j] = toupper(words[i][j]);

        if(words[i][j] == '\n')                                 //ak našiel '\n' znamená to, že nasleduje ďalší reťazec
        {
            words[i][j] = 0;                                    //'\n' nahradí null characterom na konci reťazca
            j = -1;                                             //aby mohol ďalšie slovo zapísať do ďalšieho indexu words[i][0]
            i++;                                                //tak sa musí j-1 aby sa vyrušila inkrementácia z cyklu
        }

        if (j == MAX_CHAR)                                      //ak je v riadku viac znakov ako MAX_CHAR
        {                                                       //ostatné znaky prečíta, zahodí a dá null character na koniec
            words[i][j] = 0;
            for (int l = 0; getchar() != '\n'; ++l);
            j = -1;
            i++;                                                //posunutie sa do ďalšieho reťazca vo words
            error_code[1] = true;                               //zmení error_code
        }

        if(i == MAX_ADRS)                                       //ak je v stdin viac riadkov ako je MAX_ADRS ostatné ignoruje
        {                                                       //nastaví na začiatku ďalšieho riadku EOF a ukončí cyklus
            words[i][0] = EOF;
            error_code[2] = true;
            break;
        }
    }
}

/*kontroluje zhodu znakov z *user_in a adresami **words, vráti počet enabled znakov*/
int check_match(int words[MAX_ADRS+1][MAX_CHAR+1], const char *user_in, char *enabled, char *found, char *prefound)
{
    int enabled_size = 0;
    int j = 0;                                                     // j => koľký znak
    for (int i = 0; words[i][j] != EOF; ++i, j = 0)                //pokiaľ nie je na EOF. i => koľký reťazec
    {
        if(user_in[0] == 0)                                        //ak bol vstupný agrument prázdny reťazec
            enabled[enabled_size++] = (char) words[i][0];          //do enabled sa nahrá vždy prvý znak adresy

        for (j = 0; (words[i][j]) != 0 && words[i][j] == user_in[j]; ++j)
        {                                                          //iba ak nie je na konci reťazca a znaky sa zhodujú
            if(user_in[j+1] == 0)                                  //ak cyklus práve kontroluje posledný znak z user_in
            {
                if(words[i][j+1] == 0 && found[0] == 0)            //ak je na konci kontrolovanej adresy tak ju našiel
                    for (int k = 0; words[i][k] != 0; ++k)         //postupne ju nahrá do found
                        found[k] = (char) words[i][k];
                else
                    for (int k = 0; words[i][k] != 0; ++k)         //ak nie je na konci adresy tak len prefound
                    {
                        prefound[k+1] = 0;                         //všetky ďalšie znaky nahrádza 0
                        prefound[k] = (char) words[i][k];
                    }

                if(words[i][j+1] != 0)                                  //ak ďalší znak nie je null character
                    enabled[enabled_size++] = (char) words[i][j+1];     //uloží si nasledúci enable znak do enable
            }
        }
    }
    return enabled_size;                                        //koľko znakov sa nahralo do enabled
}

void alpha_sort(char *enabled)                                  //enabled znaky zoradí podľa abecedy - bubble sort
{
    char trans;
    for (int j = 0; enabled[j] != 0; j++)
    {
        for(int i = 1+j; enabled[i] != 0; i++)
        {
            if(enabled[i] < enabled[j])
            {
                trans = enabled[j];
                enabled[j] = enabled[i];
                enabled[i] = trans;
            }
        }
    }
}

void duplicate_rm(char *enabled)                                   //duplikované znaky v enabled zmaže
{
    int k = 0;
    for (int i = 0; enabled[i] != 0; ++i)             //prepisuje znaky pokiaľ nie je na konci reťazca
    {
        if (enabled[i] != enabled[k])
        {
            if (i != 0)
                k++;
            enabled[k] = enabled[i];
        }
    }

    for (int i = k+1; enabled[i] != 0 ; ++i)          //kedže som nevytváral nové pole ale prepisoval to s duplicitnými znakmi
        enabled[i]=0;                                 //tak som musel ešte ostatné znaky prepísať null characterom
}

void print_results(const char *enabled, const char *found, const char *prefound, const int enabled_size)  //vytlačí výstup podľa argumentov
{
    if(found[0] != 0)                           //ak je niečo nahraté vo found
    {
        printf("Found: %s",found);
        if(enabled_size>=1)                     //ak je v enabled viac nájených znakov
            printf("\nEnable: %s",enabled);
    }

    else if(enabled_size == 1 && prefound[0] != 0)          //ak je v enabled práve jeden znak, znamená to, že v prefound
        printf("Found: %s",prefound);                       //existuje iba jedno doplnené-nájdené mesto, tak ho vypíše

    else if(enabled_size>=1)                                //ak je v enabled viac nájených znakov
        printf("Enable: %s",enabled);

    else
        printf("Not found");

    printf("\n");

}

void print_error(const bool error_code[4])                                    //vytlačí error podľa error_code
{
    if(error_code[0])                                          //ak bolo viac symbolov vo vstupnom argumente ako MAX_CHAR
        fprintf(stderr,"More symbols in argument then expected! Cropping occured.\n");
    if(error_code[1])                                        //ak bolo viac symbolov v riadku ako MAX_CHAR
        fprintf(stderr,"More symbols in single address then expected! Cropping occured.\n");
    if(error_code[2])                                        //ak bolo viac adries ako MAX_ADRS
        fprintf(stderr,"More addresses in file then expected! Cropping occured.\n");
    if(error_code[3])                                        //vo vstupnom argumente bol netlačiteľný znak
        fprintf(stderr,"Invalid symbol in file with addresses! Symbol skipped.\n");
}

int main(int argc, char *argv[])
{
    (void)argc;
    int words [MAX_ADRS+1][MAX_CHAR+1];                         //dvojrozmerné pole na nahrávanie adries z stdin
    char user_in[MAX_CHAR] = {0};                                    //pole na zadaný argument
    char enabled[MAX_ADRS+1] = {0};                             //pole na všetky enabled znaky
    char found[MAX_CHAR+1] = {0};                               //pole na nájdenú presne zadanú adresu
    char prefound[MAX_CHAR+1] = {0};                            //pole na doplnenú nájdenú adresu
    int enabled_size = 0;                                       //počet enabled znakov
    bool error_code[4] = {0};                    /*0 = viac symbolov vo vstupnom argumente ako MAX_CHAR
                                                   1 = viac symbolov v riadku ako MAX_CHAR
                                                   2 = viac adries ako MAX_ADRS
                                                   3 = vo vstupnom argumente bol netlačiteľný znak*/

    if(argc > 1)                                                      //iba ak užívateľ zadal argument
        for (int i = 0; (user_in[i] = argv[1][i]) != 0; ++i)          //nahratie argumentu do user_in
        {
            user_in[i] = (char) toupper(user_in[i]);            //všetky znaky prevedie na veľké
            if(i>=MAX_CHAR)                                     //ak je argument prekročil MAX_CHAR hodnotu
            {
                user_in[i+1] = 0;
                error_code[0] = true;
                break;
            }
        }

    load_file(words, error_code);                                         //číta znaky z stdin do words a kontroluje chyby
    enabled_size = check_match(words, user_in, enabled, found, prefound); //kontroluje zhodu user_in s adresami words, vráti počet enabled znakov
    alpha_sort(enabled);                                                  //enabled znaky zoradí podľa abecedy - bubble sort
    duplicate_rm(enabled);                                                //duplikované znaky v enabled zmaže
    print_results(enabled, found, prefound, enabled_size);                //vytlačí výstup podľa argumentov
    print_error(error_code);                                              //vytlačí chabové hlášky podľa error_code

    return 0;
}