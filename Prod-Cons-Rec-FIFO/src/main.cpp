/* 
 * File:   main.cpp
 * Author: albertoplaza
 *
 * Created on 5 de octubre de 2021, 10:00
 */

#include <iostream>
#include <cstdlib>
#include <chrono> 
#include "scd.h"
#include <mutex>
#include <thread>
using namespace std;
using namespace scd;

// ---------------- VAR GLOBALES MODIFICABLES--------------------

const int 
    productores = 4,
    consumidores = 2,
    recicladores = 3;

//items y tamaños de los buffers
const int
    items = 6,
    tamBuffer = 11,
    tamBufferRec = 5;

//Tiempos(ms)          
const int   
    minProdTime = 20,
    maxProdTime = 100,
    minConsTime = 20,
    maxConsTime = 100,
    minRecTime = 20,
    maxRecTime = 100;

// ---------------- VAR GLOBALES NO MODIFICABLES--------------------

//buffers y variables de gestión de los mismos
int 
    buffer[tamBuffer],
    bufferRec[tamBufferRec];

int
    siguiente_dato = 0,
    primera_libre = 0,
    primera_ocupada = 0,
    primera_libre_rec = 0,
    primera_ocupada_rec = 0;


//Reparto de tareas a cada hebra
const int 
    Cantidad_por_hebra_productores = items/productores,  
    Cantidad_por_hebra_consumidores = items/consumidores,
    cantidad_por_hebra_recicladores = items/recicladores;


//Semáforos
Semaphore
    libres = tamBuffer,
    ocupadas = 0,
    libres_reciclado = tamBufferRec,
    ocupadas_reciclado = 0;

//Mutexs
mutex
    InsertarBuffer,
    ExtraerBuffer,    
    ProducirDato,
    ConsumirDato,
    InsertarBufferReciclado,
    ExtraerBufferReciclado,
    Consola;

//Contadores para verificar que todo está correcto
int
    IntroduccionBuffer[items] = {0},
    ExtraccionBuffer[items] = {0},
    IntroduccionBufferReciclado[items] = {0},
    ExtraccionBufferReciclado[items] = {0};


// ---------------- CÓDIGO --------------------

unsigned producir_dato(int num_hebra) {
    
    this_thread::sleep_for(chrono::milliseconds(aleatorio<minProdTime, maxProdTime>()));
    
    ProducirDato.lock();
    
    const unsigned dato_producido = siguiente_dato;
    siguiente_dato++;
    
    //puedo usar el mutex consola para que no se mezclen las salidas.
    cout << "Hebra: " <<num_hebra<<" producido: " << dato_producido << endl;
    
    ProducirDato.unlock();
    
    return dato_producido;
}

void consumir_dato(int dato, int num_hebra) {
    
    this_thread::sleep_for(chrono::milliseconds(aleatorio<minConsTime, maxConsTime>()));
    
    //ConsumirDato.lock();
    
    //puedo usar el mutex consola para que no se mezclen las salidas.
    cout << "Hebra: "<< num_hebra <<" Consumido: " << dato << endl;
    
    //ConsumirDato.unlock();
}

void reciclar_dato(int dato, int num_hebra){
    this_thread::sleep_for(chrono::milliseconds(aleatorio<minConsTime, maxConsTime>()));
   
    cout << "Hebra: "<< num_hebra <<" Reciclado: " << dato << endl;
    
}


void funcion_hebra_productora_FIFO(int num_hebra) {
    int dato;
    int inicio = Cantidad_por_hebra_productores*num_hebra;
    int fin = Cantidad_por_hebra_productores + Cantidad_por_hebra_productores*num_hebra;
    
   

    if (num_hebra == productores - 1)
        fin = items;

    for (unsigned i = inicio; i < fin; i++) {
        dato = producir_dato(num_hebra);
        
        libres.sem_wait();
        
        IntroduccionBuffer[dato]++;     //se puede hacer fuera ya que dato siempre será distinto, no necesita exclusión mutua.
        
        InsertarBuffer.lock();

        buffer[primera_libre%tamBuffer] = dato;
        primera_libre++;

        InsertarBuffer.unlock();

        ocupadas.sem_signal();
    }
}


void funcion_hebra_consumidora_FIFO(int num_hebra) {
    int dato;
    int inicio = Cantidad_por_hebra_consumidores*num_hebra;     
    int fin = Cantidad_por_hebra_consumidores + Cantidad_por_hebra_consumidores*num_hebra;
    
    if (num_hebra == consumidores - 1)
        fin = items;
    
    for (unsigned i = inicio; i < fin; i++) {
        

        ocupadas.sem_wait();

        ExtraerBuffer.lock();

        dato = buffer[primera_ocupada%tamBuffer];
        primera_ocupada++;
        
        ExtraerBuffer.unlock();

        ExtraccionBuffer[dato]++;      //se puede hacer fuera ya que dato siempre será distinto, no necesita exclusión mutua.

        libres.sem_signal();
        
        consumir_dato(dato, num_hebra);
        
        //------------------------------------------
        
        libres_reciclado.sem_wait();
        
        IntroduccionBufferReciclado[dato]++;    //se puede hacer fuera ya que dato siempre será distinto, no necesita exclusión mutua.
        
        InsertarBufferReciclado.lock();
        
        
        cout << "introduzco " << dato << " en el buffer de reciclado" <<endl;
        
        bufferRec[primera_libre_rec%tamBufferRec] = dato;
        primera_libre_rec++;
        
        
        InsertarBufferReciclado.unlock();
        
        ocupadas_reciclado.sem_signal();     
    }
}


void funcion_hebra_recicladora_FIFO(int num_hebra) {
    int dato;
    int inicio = cantidad_por_hebra_recicladores*num_hebra;     //cphc = 14
    int fin = cantidad_por_hebra_recicladores + cantidad_por_hebra_recicladores*num_hebra;
    
    if (num_hebra == recicladores - 1)
        fin = items;
    
    
    for (unsigned i = inicio; i < fin; i++) {
        
        
        ocupadas_reciclado.sem_wait();

        ExtraerBufferReciclado.lock();
               
        dato = bufferRec[primera_ocupada_rec%tamBufferRec];
        primera_ocupada_rec++;
        
        ExtraerBufferReciclado.unlock();
        
        ExtraccionBufferReciclado[dato]++;  //se puede hacer fuera ya que dato siempre será distinto, no necesita exclusión mutua.
        
        libres_reciclado.sem_signal();
        
        reciclar_dato(dato, num_hebra);

        
    }
}
int main(int argc, char** argv) {

    thread hebrasproductoras[productores];
    thread hebrasconsumidoras[consumidores];
    thread hebrasrecicladoras[recicladores];
    
    for(int i=0; i< productores; i++)
        hebrasproductoras[i] = thread (funcion_hebra_productora_FIFO,i);
        
    for(int i=0; i< consumidores; i++)
        hebrasconsumidoras[i] = thread (funcion_hebra_consumidora_FIFO,i);
    
    for(int i=0; i< recicladores; i++)
        hebrasrecicladoras[i] = thread (funcion_hebra_recicladora_FIFO,i);
    
    for(int i=0; i< productores; i++)
        hebrasproductoras[i].join();
    
    for(int i=0; i< consumidores; i++)
        hebrasconsumidoras[i].join();
    
    for(int i=0; i< recicladores; i++)
        hebrasrecicladoras[i].join();
   
   
        

    for (int i = 0; i < items; i++)
        cout << "La producción " << i << " es: " << IntroduccionBuffer[i] << endl;



    for (int i = 0; i < items; i++)
        cout << "La consumición " << i << " es :" << ExtraccionBuffer[i] << endl;

    for (int i = 0; i < items; i++)
        cout << "El reciclado " << i << " es :" << IntroduccionBufferReciclado[i] << endl;


    for (int i = 0; i < items; i++)
        cout << "El reciclado " << i << " es :" << ExtraccionBufferReciclado[i] << endl;
    
    return 0;
}

