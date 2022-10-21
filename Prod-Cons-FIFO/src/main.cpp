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
    productores = 2,
    consumidores = 3;

//items y tamaño del buffer:
const int
    items = 61,
    tamBuffer = 17;

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
    primera_libre = 0,
    primera_ocupada = 0;

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
    Insertar,
    Extraer,
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
    
    
    //Ojo, se puede mezclar la salida con la de consumir dato ya que son dos mutexs distintos, podríamos usar un mismo mutex para evitarlo (consola).
    cout << "Hebra: " <<num_hebra<<" producido: " << dato_producido << endl;
    
    ProducirDato.unlock();
    
    return dato_producido;
}

void consumir_dato(int dato, int num_hebra) {
    
    this_thread::sleep_for(chrono::milliseconds(aleatorio<minConsTime, maxConsTime>()));
    
    //ConsumirDato.lock();
    
    
    //Ojo, se puede mezclar la salida con la de producir dato ya que son dos mutexs distintos, podríamos usar un mismo mutex para evitarlo (consola).
    cout << "Hebra: "<< num_hebra <<" Consumido: " << dato << endl;
    
    //ConsumirDato.unlock();
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
        
        Insertar.lock();

        buffer[primera_libre%tamBuffer] = dato;
        primera_libre++;
        
        
        Insertar.unlock();        

        ocupadas.sem_signal();
    }
}

void funcion_hebra_consumidora_FIFO(int num_hebra) {
    int dato;
    int inicio = Cantidad_por_hebra_consumidores*num_hebra;     //cphc = 14
    int fin = Cantidad_por_hebra_consumidores + Cantidad_por_hebra_consumidores*num_hebra;
    
    if (num_hebra == consumidores - 1)
        fin = items;
    
    for (unsigned i = inicio; i < fin; i++) {
               
        ocupadas.sem_wait();        
        
        Extraer.lock();
        
        dato = buffer[primera_ocupada%tamBuffer];
        primera_ocupada++;
           
        Extraer.unlock();
        
        ExtraccionBuffer[dato]++;   //se puede hacer fuera ya que dato siempre será distinto, no necesita exclusión mutua.
        
        libres.sem_signal();
        
        consumir_dato(dato, num_hebra);

        
    }
}

int main(int argc, char** argv) {

    thread hebrasproductoras[productores];
    thread hebrasconsumidoras[consumidores];
    
    for(int i=0; i< productores; i++)
        hebrasproductoras[i] = thread (funcion_hebra_productora_FIFO,i);
        
    for(int i=0; i< consumidores; i++)
        hebrasconsumidoras[i] = thread (funcion_hebra_consumidora_FIFO,i);
    
    
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

