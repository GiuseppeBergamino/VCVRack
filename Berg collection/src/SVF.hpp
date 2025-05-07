/*--------------------------- ABC ---------------------------------*
 *
 * Author: Leonardo Gabrielli <l.gabrielli@univpm.it>
 * License: GPLv3
 *
 * For a detailed guide of the code and functions see the book:
 * "Developing Virtual Synthesizers with VCV Rack" by L.Gabrielli
 *
 * Copyright 2020, Leonardo Gabrielli
 *
 *-----------------------------------------------------------------*/

#include "rack.hpp"


using namespace rack;

template <typename T> //può processare float o double con typename T
struct SVF {
	T hp, bp, lp, phi, gamma; //3 stati in out e parametri
	T fc, damp;

public:
	SVF(T fc, T damp) {  //costruttore con lo stesso nome della classe, si aspetta due parametri
		setCoeffs(fc, damp);
		reset();    //resetto le variabili interne
	}

	void setCoeffs(T fc, T damp) {
		if (this->fc != fc || this->damp != damp) {  //ottimizzo risorse, ricalcolo so se cambiano

            this->fc = fc; //perche non this.fc?, perché è un pointer?
			this->damp = damp;

			phi = clamp(2.0*sin(M_PI * fc * APP->engine->getSampleTime()), 0.f, 1.f);
            //frequenza di taglio, clamp limita il valore tra 0 e 1, sample time è 1/freq sampling

			gamma = clamp(2.0 * damp, 0.f, 1.f); //damping, se elimino il clamp? se lo faccio adattivo?
            
            //che relazione c'è tra phi e gamma in ottica stabilità? Posso avare gamma alto a phi basso?

        }
	}

	// float *a; //creo un puntatore a un float
	// a = &b;   //dico che a punta al contenuto di b
	// *a = 21;  //quello puntato da a è pari a 21, cioè b = 21;

	void reset() {
		hp = bp = lp = 0.0;
	}

	void process(T xn, T* hpf, T* bpf, T* lpf) { //l'unico modo per avere 3 uscite in C è dargli in ingresso 3 puntatori, 
		                                         //gli chiedo un indirizzo T* corrisponde a & nel cpp
		bp = *bpf = phi*hp + bp;
		lp = *lpf = phi*bp + lp;
		hp = *hpf = xn - lp - gamma*bp; //z-1 è implicito nella formulazione C++
	}
};
