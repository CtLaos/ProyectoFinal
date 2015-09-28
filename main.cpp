#include <allegro.h>
#include <ctime>
#include "inicia.h"
#include <map>
#include <iostream>
#include <fstream>

using namespace std;
const int TAM = 16;//tamaño pieza 16x16
const int FILAS = 22;//Numero de filas del tablero
const int COLUMNAS = 10;//Numero de columnas del tablero
const int NUMPIEZAS = 11;//Numero de ladrillos de color que forman las diferentes piezas del juego
bool partidaTerminada=FALSE,//Controlamos la salida del bucle del juego
     terminar,//controlamos la salida de la aplicacion
     alto;//Controlamos la entrada y salida de pausa
int tecla;//Guardamos que tecla se ha presionado
int tics;//variable para el control de velocidad en que bajan las piezas
int nivel;//para ir aumentando la velocidad de las piezas cuando bajan
int puntos;//Guardamos cuantos ha logrado el jugador
int filas;//Guardamos cuantas filas ha destruido el jugador
string nombre;//Para guardar el nombre en el fichero que nos ayudará a mostrar highscores
typedef int Tablero[COLUMNAS][FILAS];/*aqui guardaremos los colores de nuestras piezas para conocer
                                            cuando se forma una linea o si el jugador pierde */
multimap<int, string> jugadores;//inicializar vector con la cantidad de scores necesarios para el ranking
const int CANTIDAD_SCORES = 6;
const int SIZE_FORMATO = 14;

struct Coordenadas
{
    int x, y;
};//coordenadas de nuestras piezas dentro del tablero
struct Pieza  //Estructura que compone cada una de nuestras piezas
{
    Coordenadas origen;// bloque central (posición absoluta)
    Coordenadas perifericas[3];// bloques periféricos (posición relativa)
    int tipoLadrillo;//color de la pieza
    Coordenadas posicion(int n) const; // n entre 0 y 3 (0 = central, 1-3 = perif.)
    //funcion que utilizaremos para conocer la posicion de nuestras piezas en el tablero
};
const Coordenadas perifs[7][3] =  //Cada una de las piezas del juego
{
    { { 1,0 }, { 0,1 }, { 1,1 } }, // cuadrado
    { { 1,0 }, {-1,1 }, { 0,1 } }, // S
    { { 0,1 }, { 1,1 }, {-1,0 } }, // 2
    { { 0,1 }, { 0,-1}, { 1,1 } }, // L
    { { 0,1 }, { 0,-1}, {-1,1 } }, // Lr
    { {-1,0 }, { 1,0 }, { 0,1 } }, // T
    { { 0,1 }, { 0,-1}, { 0,2 } }, // Palo
};
BITMAP *buffer,//para eliminar el parpadeo
       *cursor;//imagen para el cusor del juego
BITMAP *img,//imagen que contiene todos los ladrillos de diferentes colores
       *piezas[NUMPIEZAS],//imagen que contiene cada uno de los ladrillos de diferentes colores
       *tablero,//aqui dibujaremos el tablero del juego
       *pieza_sig,//mostraremos que pieza es la sig
       *mascara,//imagen para decorar el juego
       *GameOver,//imagen que mostraremos para decir que ha perdido
       *Logo1,//imagen que mostraremos en la intro del juego
       *fondoGameOver,//aqui iremos dibujando la imagen de gameover poco a poco
       *pause,//imagen de pausa del juego
       *ayuda,//imagen donde mostramos las diferentes teclas ha usar en el juego
       *puntuacionesMax;//Imagen de fondo de la pantalla puntuaciones max
PALETTE pal;//paleta de colores de nuestras imagenes
SAMPLE *mover,*fila,*gameover,*incrustar,*rotar,*choque,*pausar;//Sonidos dentro del juego
MIDI *musica;//musica de fondo de nuestro juego
FONT *fuente;//tipo de fuente que utilizaremos dentro del juego


string toString(int number)
{
    if (number == 0)
        return "0";
    std::string temp="";
    std::string returnvalue="";
    while (number>0)
    {
        temp+=number%10+48;
        number/=10;
    }
    for (int i=0; i<(int)temp.length(); i++)
        returnvalue+=temp[temp.length()-i-1];
    return returnvalue;
}

//IMPRIMIR RANKING
void showRanking(){
        /**/
        int scorePosition = 1;
        int y = 0;
        //Imprime
        for(multimap<int, string>::iterator x = jugadores.begin();//initial value
            x != jugadores.end(); //limit
            x++){//increment
            //Variable que almacena datos
            string playerScore = toString(scorePosition);
            scorePosition++;
            //cout<<scorePosition;
            //cout<<". ";
            playerScore+=". ";
            playerScore+=((*x).second);
            int timeScore = (*x).first;
            playerScore += " PUNTUACION: ";
            playerScore += toString(timeScore);

                cout<<playerScore<<endl;
                textout_ex(buffer, fuente, "Instrucciones ", 253, 317, 0xFFFFFF, -1);
            y++;
            if (y == CANTIDAD_SCORES)//Limita la cantidad de scores

                break;

    }
}

/**
    Guardar Scoresg
**/

void writeScore(string nombre, int seg){
    string archivo = "scores.vrs";
    ofstream out(archivo.c_str());
    int y = 0;
    jugadores.insert(pair<int, string>(seg, nombre));
    for(multimap<int, string>::iterator x = jugadores.begin(); x != jugadores.end(); x++){
        int time = (int)(*x).first;
        string name = (*x).second;
        out.write((char*)&time, 4);
        out.write(name.c_str(), 10);
        y++;
        if (y >= CANTIDAD_SCORES)
            break;
    }
    out.close();
}


/**
    Leer Scores
**/
void readScores(){
    //xff3d
    ifstream in("scores.vrs");
    if (!in)
        return;
    in.seekg(0, ios::end);
    int tamano = in.tellg();
    int cant = tamano / SIZE_FORMATO;
    in.seekg(0, ios::beg);
    jugadores.clear();//limpiar multimap
    for (int x = 0; x < cant; x++){
        string nombre0;
        char* n = new char[10];
        int time;
        in.read((char*)&time, 4);
        in.read(n, 10);
        nombre0 = n;
        jugadores.insert(pair<int, string>(time, nombre0));
    }


    in.close();
}


const int puntos_limite[10] =  //array con el cual controlaremos el nivel del juego
{
    150, 300, 450, 600, 750, 900, 1050, 1200, 1450, 1600
};
const int tics_nivel[10] =  //array para el control de velocidad de nuestras piezas (dificultad del juego)
{
    33, 25, 20, 18, 16, 14, 12, 10, 8, 2
};
Coordenadas Pieza::posicion(int n) const
{
    /*retornamos las posiciones de cada cuadro
                                             que componen cada pieza del juego*/
    Coordenadas ret = { origen.x, origen.y };
    if (n != 0)
    {
        ret.x += perifericas[n-1].x;
        ret.y += perifericas[n-1].y;
    }
    return ret;
}
//PROTOTIPOS
void tablero_pinta(Tablero& T);
void tablero_limpia(Tablero& T);
void cuadrado(int x, int y, int tipoL);
void pinta_pieza_sig(int x, int y, int tipoL);
void pinta_pieza(const Pieza& P,int tipo);
void pieza_nueva(Pieza& P);
void play(int x);
void tablero_incrusta_pieza(Tablero& T, const Pieza& P) ;
Coordenadas rota_derecha(const Coordenadas& c) ;
void rota_derecha(Pieza& P);
void tablero_pinta(const Tablero& T);
bool tablero_colision(const Tablero& T, const Pieza& P);
bool tablero_fila_llena(const Tablero& T, int fila);
void tablero_colapsa(Tablero& T, int fila);
int tablero_cuenta_lineas(Tablero& T) ;
void GAMEOVER();
void pausa();
void comprobarTeclas(Tablero& T,Pieza& p,Pieza& sig);
void dibuja_pantalla();
void init();
void deinit();
void intro();
void juego();
void menu_inicio();
////////////////////////////////////////////////////////////////////////////////
int main()
{
    init();
    intro();
    set_window_title("Tetris");
    play(0);
    set_mouse_sprite(cursor);
    show_mouse(screen);
    do //bucle principal del juego mostramos el menu para iniciar un nuevo juego,ayuda o salir del prog
    {
        clear_keybuf();
        clear_to_color(buffer,0xffffff);
        draw_sprite(buffer,Logo1,0,0);
        menu_inicio();
        blit(buffer,screen,0,0,0,0,640,480);
        rest(40);
    }

    while(!terminar);
    deinit();
    return 0;
}
END_OF_MAIN();
////////////////////////////////////////////////////////////////////////////////
//DEFINICIONES
void tablero_pinta(Tablero& T) //dibujamos el tablero en el buffer del tablero para mostrarlo en pantalla
{
    for(int i = 0; i<COLUMNAS; i++)
    {
        for(int j = 0; j<FILAS; j++)
        {
            if(T[i][j]!=-1)
                draw_sprite(tablero,piezas[T[i][j]],i * TAM, j * TAM);
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
void tablero_limpia(Tablero& T) //limpiamos el tablero para un nuevo juego
{
    for(int i = 0; i<COLUMNAS; i++)
    {
        for(int j = 0; j<FILAS; j++)
        {
            T[i][j]=-1;
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
void cuadrado(int x, int y, int tipoL) //dibujamos cada uno de los ladrillos de la pieza en el buffer tablero
{
    draw_sprite(tablero,piezas[tipoL],x*TAM,y*TAM);
}
////////////////////////////////////////////////////////////////////////////////
void pinta_pieza_sig(int x, int y, int tipoL) //dibujamos la pieza sig en el buffer pieza_sig
{
    draw_sprite(pieza_sig,piezas[tipoL],x*TAM,y*TAM);
}/*para asi terner mayor libertad
 de colocar el buffer pieza sig en cualquier posicion de la ventana del juego*/
////////////////////////////////////////////////////////////////////////////////
void pinta_pieza(const Pieza& P,int tipo)  //dibujamos la pieza
{
    if(tipo==1)
    {
        clear_bitmap(pieza_sig);
        clear_to_color(pieza_sig, 0x000000);
    }
    for (int i = 0; i < 4; i++)
    {
        Coordenadas c = P.posicion(i);
        if(tipo==0)
            cuadrado(c.x, c.y,P.tipoLadrillo);
        else
            pinta_pieza_sig(c.x, c.y+1,P.tipoLadrillo);
    }
}
////////////////////////////////////////////////////////////////////////////////
void pieza_nueva(Pieza& P)  //generamos una nueva pieza
{
    P.origen.x = 5;
    P.origen.y = 1;
    P.tipoLadrillo = rand() % 11;//color aleatorio de la pieza
    // Pieza al azar
    int r = rand() % 7;
    for (int i = 0; i < 3; i++)
    {
        P.perifericas[i] = perifs[r][i];
    }
}
////////////////////////////////////////////////////////////////////////////////
void play(int x) //funcion que controla que sonido se reproduce dentro del juego
{
    switch(x)
    {
    case 0:
        play_midi(musica, TRUE);
        break;
    case 1:
        play_sample(mover,255,128,1000,0);
        break;
    case 2:
        play_sample(fila,255,128,1000,0);
        break;
    case 3:
        play_sample(incrustar,255,128,1000,0);
        break;
    case 4:
        play_sample(rotar,255,128,1000,0);
        break;
    case 5:
        play_sample(gameover,255,128,1000,0);
        break;
    case 6:
        play_sample(pausar,255,128,1000,0);
        break;
    }
}
////////////////////////////////////////////////////////////////////////////////
void tablero_incrusta_pieza(Tablero& T, const Pieza& P)
{
    /*colocamos la pieza en el tablero
                                         cuando ha colicionado con el fondo o con otras piezas*/
    for (int i = 0; i < 4; i++)
    {
        Coordenadas c = P.posicion(i);
        T[c.x][c.y] = P.tipoLadrillo;
    }
    play(3);
}
////////////////////////////////////////////////////////////////////////////////
Coordenadas rota_derecha(const Coordenadas& c)  //invertimos las coordenadas de la pieza
{
    Coordenadas ret = { -c.y, c.x };
    play(4);
    return ret;
}
////////////////////////////////////////////////////////////////////////////////
void rota_derecha(Pieza& P)  //rotamos la pieza
{
    for (int i = 0; i < 3; i++)
    {
        P.perifericas[i] = rota_derecha(P.perifericas[i]);
    }
}
////////////////////////////////////////////////////////////////////////////////
void tablero_pinta(const Tablero& T)  //dibujamos el tablero con su contenido
{
    for (int i = 0; i < COLUMNAS; i++)
    {
        for (int j = 0; j < FILAS; j++)
        {
            cuadrado(i, j,T[i][j]);
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
bool tablero_colision(const Tablero& T, const Pieza& P)  //verificamos los limites del tablero
{
    for (int i = 0; i < 4; i++)                //asi como si hay residuos de piezas dentro del
    {
        Coordenadas c = P.posicion(i);          //tablero
        // Comprobar límites
        if (c.x < 0 || c.x >= COLUMNAS)
        {
            return true;
        }
        if (c.y < 0 || c.y >= FILAS)
        {
            return true;
        }
        // Mirar "basurilla" (residuos de piezas)
        if (T[c.x][c.y] != -1)
        {
            return true;
        }
    }
    return false;
}
////////////////////////////////////////////////////////////////////////////////
bool tablero_fila_llena(const Tablero& T, int fila)  //verificamos si hay una fila llena
{
    for (int i = 0; i < COLUMNAS; i++)
    {
        if (T[i][fila] == -1) return false;
    }
    return true;
}
////////////////////////////////////////////////////////////////////////////////
void tablero_colapsa(Tablero& T, int fila)  //si encontramos una fila llena tenemos que borrarla
{
    // Copiar de abajo a arriba
    for (int j = fila; j > 0; j--)
    {
        for (int i = 0; i < COLUMNAS; i++)
        {
            T[i][j] = T[i][j-1];
        }
    }
    // Vaciar la de arriba
    for (int i = 0; i < COLUMNAS; i++)
    {
        T[i][0] = -1;
    }
}
////////////////////////////////////////////////////////////////////////////////
int tablero_cuenta_lineas(Tablero& T)  //verificamos cuantas filas han sido completadas
{
    int fila = FILAS - 1, cont = 0;
    while (fila >= 0)
    {
        if (tablero_fila_llena(T, fila))
        {
            tablero_colapsa(T, fila);
            play(2);
            cont++;
        }
        else
        {
            fila--;
        }
    }
    return cont;
}
////////////////////////////////////////////////////////////////////////////////
void GAMEOVER()  //funcion que nos dice que hemos perdido el juego
{
    int i=0;
    draw_sprite(fondoGameOver, buffer, 0, 0);
    do
    {
        set_trans_blender(0, 0, 0, i);
        draw_trans_sprite(fondoGameOver, GameOver, 0, 0);
        rest(5);
        i+=3;
        blit(fondoGameOver,screen,0,0,0,0,640,480);
    }
    while(i<256);
    clear_to_color(buffer,0x000000);
    rest(1000);
}
////////////////////////////////////////////////////////////////////////////////
void pausa() //funcion con la cual detenemos la ejecucion del juego
{
    int i=0,k=0;
    set_volume(-1,35);
    clear_keybuf();
    play(6);
    while(!alto)
    {
        if(k>5)
        {
            k=0;
            i+=5;
            if(i>640)
            {
                i=-pause->w-30;
            }
        }
        clear_to_color(buffer,0x44455+i);
        draw_sprite(buffer,pause,i,200);
        blit(buffer, screen, 0, 0, 0, 0, 640, 480);
        rest(20);
        k++;
        if (keypressed())
        {
            tecla = readkey() >> 8;
            if(tecla == KEY_ENTER)
            {
                alto=true;
            }
        }
        clear_keybuf();
    }
    clear_keybuf();
    play(6);
    set_volume(-1,70);
}
////////////////////////////////////////////////////////////////////////////////
void comprobarTeclas(Tablero& T,Pieza& p,Pieza& sig) //comprobamos si una tecla ha sido presionada
{
    Pieza copia = p;
    if (keypressed())
    {
        tecla = readkey() >> 8;
        if ( tecla == KEY_ESC )
            partidaTerminada = TRUE;
        if ( tecla == KEY_RIGHT || tecla == KEY_6_PAD )
            p.origen.x++;
        if ( tecla == KEY_LEFT || tecla == KEY_4_PAD )
            p.origen.x--;
        if ( tecla == KEY_DOWN || tecla == KEY_2_PAD)
            p.origen.y++;
        if ( tecla == KEY_UP || tecla == KEY_SPACE || tecla == KEY_8_PAD)
            rota_derecha(p);
        if ( tecla == KEY_N )//tecla oculta para aumentar la dificultad
            nivel++;
        if ( tecla == KEY_ENTER)
        {
            pausa();
        }
        clear_keybuf();
    }
    else
    {
        if(tics> tics_nivel[nivel]||nivel>10)
        {
            tics=0;
            p.origen.y++;
            tecla = KEY_DOWN;
        }
        alto=false;
    }
    if (tablero_colision(T, p))
    {
        p = copia;
        if (tecla == KEY_DOWN)
        {
            tablero_incrusta_pieza(T, p);
            int cont = tablero_cuenta_lineas(T);
            filas += cont;
            puntos += 12 * cont;
            if (puntos > puntos_limite[nivel] )
            {
                nivel++;
            }
            p = sig;
            pieza_nueva(sig);
            pinta_pieza(sig,1);
            if (tablero_colision(T, p))
            {
                pinta_pieza(p,0);
                partidaTerminada=TRUE;
                set_volume(255,-1);
                play(5);
                rest(500);
                GAMEOVER();
                set_volume(70,-1);
            }
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
void dibuja_pantalla() //volcamos los diferentes buffers y mostramos en pantalla
{
    draw_sprite(buffer,pieza_sig,250,94);
    draw_sprite(buffer, tablero, 65, 62);
    draw_sprite(buffer,mascara,0,0);
    textprintf_ex(buffer, fuente, 369,228, 0xffffff,-1,"%d",puntos);
    textprintf_ex(buffer, fuente, 369,260, 0xffffff,-1,"%d",nivel);
    textprintf_ex(buffer, fuente, 369,293, 0xffffff,-1,"%d",filas);
    blit(buffer, screen, 0, 0, 0, 0, 640, 480);
}
////////////////////////////////////////////////////////////////////////////////
void init() //inicializamos todos nuestros archivos a utilizar en el juego
{

    inicia_allegro(640, 480);
    inicia_audio(70,70);
    srand((unsigned int)time(0)); // Inicializar los números al azar (poner la semilla)
    mascara=load_bitmap("resources/images/mascara.bmp",pal);
    buffer = create_bitmap(640, 480);
    img=load_bmp("resources/images/ladrillos.bmp",pal);
    tablero = create_bitmap(TAM*COLUMNAS, TAM*FILAS);
    pieza_sig = create_bitmap(250, 250);
    for(int i=0; i<NUMPIEZAS; i++)
    {
        piezas[i]= create_bitmap(TAM, TAM);
        // Y los extraigo de la imagen "grande"
        blit(img,piezas[i]//bitmaps de origen y destino
             , TAM*i, 0 // coordenadas de origen
             , 0, 0 // posición de destino
             , TAM, TAM); // anchura y altura
    }
    musica = load_midi("resources/sound/tetris.mid");
    mover = load_sample("resources/sound/mover.wav");
    fila = load_sample("resources/sound/filas.wav");
    gameover = load_sample("resources/sound/GameOver.wav");
    incrustar = load_sample("resources/sound/incrustar.wav");
    rotar = load_sample("resources/sound/rotar.wav");
    fuente=load_font("resources/font/fuente.pcx",NULL,NULL);
    GameOver=load_bmp("resources/images/game-over.bmp",pal);
    Logo1=load_bmp("resources/images/logo1.bmp",pal);
    fondoGameOver=create_bitmap(640,480);
    ayuda=load_bmp("resources/images/Ayuda.bmp",pal);
    choque=load_sample("resources/sound/Die.wav");
    cursor=load_bmp("resources/images/cursor.bmp",pal);
    pause=load_bmp("resources/images/pause.bmp",pal);
    pausar=load_sample("resources/sound/pausa1.wav");
    puntuacionesMax=load_bmp("resources/images/max.bmp",pal);
}
////////////////////////////////////////////////////////////////////////////////
void deinit() //liberamos memoria antes de terminar la aplicacion
{
    clear_keybuf();
    destroy_bitmap(buffer);
    for(int i=0; i<NUMPIEZAS; i++)
    {
        destroy_bitmap(piezas[i]);
    }
    destroy_bitmap(img);
    destroy_bitmap(tablero);
    destroy_bitmap(pieza_sig);
    destroy_midi(musica);
    destroy_sample(mover);
    destroy_sample(gameover);
    destroy_sample(fila);
    destroy_sample(incrustar);
    destroy_sample(rotar);
    destroy_font(fuente);
    destroy_bitmap(GameOver);
    destroy_bitmap(Logo1);
    destroy_bitmap(fondoGameOver);
    destroy_sample(choque);
    destroy_sample(pausar);
    destroy_bitmap(ayuda);
    destroy_bitmap(cursor);
    destroy_bitmap(pause);
    destroy_bitmap(puntuacionesMax);
}
////////////////////////////////////////////////////////////////////////////////
void intro()  //intro juego
{
    tics=0;
    int i=640;
    while (i>0)
    {
        if(tics > 5)
        {
            i-=10;
            tics=0;
        }
        if(i>=0)
        {
            clear_to_color(buffer,0x444455+i);
            draw_sprite(buffer,Logo1,0,i);
        }
        vsync();
        blit(buffer,screen,0,0,0,0,640,480);
        tics++;
        rest(10);
    }
    play_sample(choque,255,128,1000,0);
}
////////////////////////////////////////////////////////////////////////////////
void juego() //funcion de nuestro juego
{
    terminar=FALSE;
    partidaTerminada=FALSE;
    Tablero T;
    Pieza p,sig;
    tablero_limpia(T);
    cin>>nombre;
    puntos = 0;
    nivel = 0;
    filas = 0;
    pieza_nueva(p);
    pieza_nueva(sig);
    pinta_pieza(sig,1);
    tics=0;
    do
    {
        clear_bitmap(buffer);
        clear_bitmap(tablero);
        pinta_pieza(p,0);
        comprobarTeclas(T,p,sig);
        tablero_pinta(T);
        dibuja_pantalla();
        rest(40);
        tics++;
    }
    while(!partidaTerminada);
    puntos=+puntos;
    writeScore(nombre,puntos);
}

void menu_inicio() //menu principal del juego 37 pixeles de espacio en Y entre cada boton
{
    readScores();
    if((mouse_x>236&&mouse_x<410) && (mouse_y>290&&mouse_y<327))
    {
        textout_ex(buffer, fuente, "Ini cio ", 253, 282, 0xFFFFFF, -1);
        if(mouse_b & 1)
        {
            scare_mouse();
            cout<<"Porfavor ingrese su nombre"<<endl;
            juego();
        }
    }
    else
    {
        textout_ex(buffer, fuente, "Inicio ", 273, 282, 0x769FDD, -1);
    }
    //Instrucciones
    if((mouse_x>236&&mouse_x<410) && (mouse_y>328&&mouse_y<364))
    {
        textout_ex(buffer, fuente, "Instrucciones ", 253, 317, 0xFFFFFF, -1);
        if(mouse_b & 1)
        {
            draw_sprite(buffer, ayuda, 0, 0);
            textout_ex(buffer, fuente, "Presiona una tecla", 190, 430, 0xFFFFFF, -1);
            blit(buffer,screen,0,0,0,0,640,480);
            readkey();
        }
    }
    else
    {
        textout_ex(buffer, fuente, "Ayuda ", 273, 317, 0x769FDD, -1);
    }
    if((mouse_x>236&&mouse_x<410) && (mouse_y>365&&mouse_y<402))
    {
        textout_ex(buffer, fuente, "Sa lir ", 273, 352, 0xFFFFFF, -1);
        if(mouse_b & 1)
        {
            drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
            set_trans_blender(0, 0, 0, 128);
            rectfill(buffer, 0, 0, buffer->w, buffer->h, makecol(0, 0, 0));
            textout_ex(buffer, fuente, "Hasta luego", 250, 240, 0xFFFFFF, -1);//agregado
            solid_mode();
            blit(buffer, screen, 0, 0, 0, 0, 640, 480);
            rest(2000);
            terminar=TRUE;
        }
    }
    else
    {
        textout_ex(buffer, fuente, "Salir ", 273, 352, 0x769FDD, -1);
    }

    if((mouse_x>236&&mouse_x<410) && (mouse_y>403&&mouse_y<439))
    {
        textout_ex(buffer, fuente, "Mayores Puntuajes ", 273, 388, 0x769FDD, -1);
        if(mouse_b & 1)
        {
            draw_sprite(buffer, puntuacionesMax, 0, 0);
            textout_ex(buffer, fuente, "Presiona una tecla", 190, 430, 0xFFFFFF, -1);
            blit(buffer,screen,0,0,0,0,640,480);
            showRanking();
            readkey();
        }
    }
    else
    {
        textout_ex(buffer, fuente, "MayoresPuntuajes ", 273, 388, 0x000000, -1);
    }
}
