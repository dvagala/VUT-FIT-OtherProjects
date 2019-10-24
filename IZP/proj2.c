#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define ERROR_2 "Argumnet is in wrong format! See --help\n"

#define MAX_ANGLE 1.4                                       //maximálny uhol pre počítanie tangensu 0 < A <= 1.4 < π/2.
#define MAX_TYL_CHAIN 13                                    //maximálny počet iterácií taylorovho rádu
#define ACCURACY 9                                          //počet iterácií pre -m, pri presnosti na 10 desatinných miest stačí 9, pri max. uhle 1.4

/*kontroluje vstupné argumenty, či sú v správnych rozsahoch, ak hej načíta ich*/
int input_handle(char **argv, const int *argc, double *a, double *b, unsigned int *n, unsigned int* m, double *sens_height);
void print_help();                                                      //vytlačí nápovedu
void measure(double a, double b, double sens_height, unsigned int n);   //vypočíta vzdialenosť a veľkosť meraného predmetu
void compare(double a, unsigned int n);                     //porovná výsledky taylor_tan, cfrac_tan a tan z math.h
double taylor_tan(double x, unsigned int n);                //výpočet tangensu cez taylorov polynom, n = počet iterácií
double cfrac_tan(double x, unsigned int n);                 //výpočet tangensu cez zretezené zlomky, n = počet iterácií

int main(int argc, char *argv[])
{
    double sens_height = 1.5;           //výška senzoru na meranie
    double a = 0;                       //uhol a v radiánoch
    double b = 0;                       //uhol b v radiánoch
    unsigned int n = 0;                          //počiatočná hranica na výpis iterácií
    unsigned int m = 0;                          //koncová hranica na výpis iterácií

    /* input_handle return code: 0 - na vstupe programu bol neočakávaný argument - program sa skončí
     *                           1 - argumentom programu bol --help
     *                           2 - argmunetom programu bol --tan A N M
     *                           3 - argmunetom programu bol [-c X] -m A [B]
     */
    switch(input_handle(argv, &argc, &a, &b, &n, &m, &sens_height))    //podľa return hodnoty input_handle sa budú spúšťať jednotlivé funkcie
    {                                                                  //return code: 0 error, 1 --help, 2 --tan A N M, 3 [-c X] -m A [B]
        case 1:
            print_help();
            break;

        case 2:
            for (; n <= m; ++n)                                       //vypisuje výsledky do N do M
                compare(a,n);
            break;

        case 3:
            measure(a, b, sens_height, ACCURACY);
            break;

        default:                                                      //pokiaľ bol return 0, alebo sa nebol 1,2,3
            fprintf(stderr, ERROR_2);
            break;
    }

    return 0;
}

/*kontroluje vstupné argumenty, či sú v správnych rozsahoch, ak hej načíta ich
 * return code: 0 - --help
 *              1 - error occured
 *              2 - --tan
 *              3 - -m */

void print_help()
{
    printf("\nDESCRIPTION\n"
    "\n--tan A N M     Compare accuracy of tanget calculation of implemented funcions taylor polynom"
                   "\n\t\tand continued fractions with tan() funcion form math.h library.\n"
                   "\t\tArgumnet A is angle in radians.  0 < A <= 1.4 < π/2"
                   "\n\t\tArguments N and M sets range of iterations.  0 < N <= M < 14\n"
    "\n[-c X] -m A [B] Calculate tanget with     and measure distance and height."
                   "\n\t\tArgument -c X (optional) set hight of meter - implicit height is 1.5m.  0 < X <= 100"
                   "\n\t\tArgumnet A is angle in radians.  0 < A <= 1.4 < π/2"
                   "\n\t\tArgumnet B (optional) is angle β in radians.  0 < B <= 1.4 < π/2\n"
    "\n--help          display this help and exit\n\n");
}

int input_handle(char **argv, const int *argc, double *a, double *b, unsigned int *n, unsigned int* m, double *sens_height)
{
    int shift = 0;                        //premenná na posúvanie indexov v argv, aby mohol byť argument -c voliteľný

    if(*argc <2 )                         //v každom prípade sa vyžaduje zadať aspoň 3 argumenty, ak neboli tak return 1
        return 0;

    if(strcmp(argv[1],"--help") == 0)           //ak bol argument --help
        return 1;

    else if(strcmp(argv[1],"--tan") == 0 && *argc > 4)    //pri --tan argumente musí byť 5 argumentov, ak sú niektoré navyše, ignoruje ich
    {
        *a = strtod(argv[2],NULL);
        *m = (unsigned int) strtol(argv[4],NULL,10);
        *n = (unsigned int) strtol(argv[3],NULL,10);

        if((*a <= 0 || *a > MAX_ANGLE) || (*m <= 0 || *m > MAX_TYL_CHAIN) || (*n <= 0 || *n > *m))     //0 < A <= 1.4 < π/2.
            return 0;

        return 2;
    }

    if(strcmp(argv[1],"-c") == 0 && *argc >=5)                      //ak bol zadaný aj -c, tak sa pomocou shift posúvajú indexy v argv.
    {                                                  //shift = 2, pretože pridaním -c argumentu
        shift = 2;                                     //Implicitne je 0, čiže ak -c argument nebol zadaný,
        *sens_height = strtod(argv[2],NULL);           //tak to indexy neovplyvní

    }

    if(*argc < 3+shift)                               //pri -m, sa vyžadujú aspoň 3 argumenty
        return 0;

    if(strcmp(argv[1+shift],"-m") == 0)
    {
        if(*argc > 3+shift)                            //skontroluje sa, či bol zadaný aj voliteľmný argument B
        {
            *b = strtod(argv[3+shift],NULL);
            if(*b <= 0 || *b > MAX_ANGLE)                     //0 < B <= 1.4 < π/2
                return 0;
        }

        *a = strtod(argv[2+shift],NULL);
        if((*a <= 0 || *a > MAX_ANGLE) || (*sens_height <= 0 || *sens_height > 100))        //0 < A <= 1.4 < π/2
            return 0;

        return 3;
    }

    return 0;
}

void measure(double a, double b, double sens_height, unsigned int n)
{
    double distance = sens_height/cfrac_tan(a,n);                   //vypočítanie a výpis vzdialenosti
    printf("%.10e\n",sens_height/cfrac_tan(a,n));

    if(b > 0)                                                       //ak bol zadaný voliteľný B argument
    {
        double height = (cfrac_tan(b,n)*distance) + sens_height;    //vypočítanie a výpis výšky
        printf("%.10e\n",height);
    }
}

void compare(double a, unsigned int n)
{
    double cfrac_result = cfrac_tan(a,n);
    double taylor_result = taylor_tan(a,n);
    double math_result = tan(a);

    printf("%d %e %e %e %e %e\n",n,math_result,taylor_result,
           fabs(taylor_result-math_result),cfrac_result,fabs(cfrac_result-math_result));

    /* Printed arguments order:
        * 1. počet iterácií
        * 2. výsledok tan funkcie z math.h
        * 3. výsledok z taylorovho polynomu
        * 4. nepresnosť tylorovho polynomu
        * 5. výsledok zretezeného zlomku
        * 6. neprosnosť zretezeného zlomku
     */
}

double taylor_tan(double x, unsigned int n)
{
    unsigned long long numerat[MAX_TYL_CHAIN+1] =                                                     //rada čiteteľov
            {1,1,2,17,62,1382,21844,929569,6404582,443861162,18888466084,113927491862,58870668456604};
    unsigned long long denominat[MAX_TYL_CHAIN+1] =                                                   //rada menovateľov
            {1,3,15,315,2835,155925,6081075,638512875,10854718875,1856156927625,194896477400625,49308808782358125,3698160658676859375};

    double result = 0;
    double exponent = x;

    for (unsigned int i = 0; i < n; i++)
    {
        result += (numerat[i] * exponent) / denominat[i];     // nemusí sa vždy rátať exponent v čiteteli od začiatku,
        exponent *= x*x;                                      // tak len predošlý exponent ^2
    }

    return result;
}

double cfrac_tan(double x, unsigned int n)
{
    double cf = 0.;                               //cf = dočasný výsledok
                                                  //n = počet iterácií
    for (int i = n-1; i > 0 ; --i)                //posledná operácia sa líši od predošlých, preto len n-1 krát
        cf = (x*x) / ((i*2 + 1)-cf);

    cf = x/(1-cf);                                //posledná operácia

    return cf;
}
