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

// ---------------- VAR GLOBALES MODIFICABLES --------------------

const int
    productores = 5,
    consumidores = 2;

//items y tamaño del buffer:
const int
    items = 56,
    tamBuffer = 11;

//Tiempos (ms)
const int   
    minProdTime = 20,
    maxProdTime = 100,
    minConsTime = 20,
    maxConsTime = 100;

// ---------------- VAR GLOBALES NO MODIFICABLES --------------------

//Buffer y variables para inserción/extracción en buffer
int buffer[tamBuffer];
int
    siguiente_dato = 0,
    primera_libre = 0;

//Reparto trabajo de hebras
const int 
    Cantidad_por_hebra_productores = items/productores,   
    Cantidad_por_hebra_consumidores = items/consumidores; 

//Semáforos
Semaphore
    libres = tamBuffer,
    ocupadas = 0;

//Mutexs
mutex
    Acceso,
    ProducirDato,
    ConsumirDato,
    Consola;



//Contadores para verificar que todo está correcto
int
    IntroduccionBuffer[items] = {0},
    ExtraccionBuffer[items] = {0};



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
    cout << "Hebra: " << num_hebra << " Consumido: " << dato << endl;

    //ConsumirDato.unlock();
}


void funcion_hebra_productora_LIFO(int num_hebra) {
    int dato;
    int inicio = Cantidad_por_hebra_productores*num_hebra;
    int fin = Cantidad_por_hebra_productores + Cantidad_por_hebra_productores*num_hebra;
    
   

    if (num_hebra == productores - 1)
        fin = items;

    for (unsigned i = inicio; i < fin; i++) {
        dato = producir_dato(num_hebra);
        
        libres.sem_wait();
        
        IntroduccionBuffer[dato]++;    //se puede hacer fuera ya que dato siempre será distinto, no necesita exclusión mutua.
        
        Acceso.lock();
               

        buffer[primera_libre] = dato;
        primera_libre++;

        Acceso.unlock();        

        ocupadas.sem_signal();
    }
}

void funcion_hebra_consumidora_LIFO(int num_hebra) {
    int dato;
    int inicio = Cantidad_por_hebra_consumidores*num_hebra;     
    int fin = Cantidad_por_hebra_consumidores + Cantidad_por_hebra_consumidores*num_hebra;
    
    if (num_hebra == consumidores - 1)
        fin = items;
    
    for (unsigned i = inicio; i < fin; i++) {
        

        ocupadas.sem_wait();

        Acceso.lock();

        primera_libre--;
        dato = buffer[primera_libre];      
        
        Acceso.unlock();
        
        ExtraccionBuffer[dato]++;       //se puede hacer fuera ya que dato siempre será distinto, no necesita exclusión mutua.

        libres.sem_signal();
        
        consumir_dato(dato, num_hebra);
    }
}

int main(int argc, char** argv) {

    thread hebrasproductoras[productores];
    thread hebrasconsumidoras[consumidores];
    
    for(int i=0; i< productores; i++)
        hebrasproductoras[i] = thread (funcion_hebra_productora_LIFO,i);
        
    for(int i=0; i< consumidores; i++)
        hebrasconsumidoras[i] = thread (funcion_hebra_consumidora_LIFO,i);
    
    
    for(int i=0; i< productores; i++)
        hebrasproductoras[i].join();
    
    for(int i=0; i< consumidores; i++)
        hebrasconsumidoras[i].join();
   
   
    for (int i=0; i < items; i++)
            cout << "La producción " << i << " es: " << IntroduccionBuffer[i]<< endl;
        
    

    for (int i=0; i < items; i++)
            cout << "La consumición " << i << " es :" <<ExtraccionBuffer[i] << endl;

    
    return 0;
}

