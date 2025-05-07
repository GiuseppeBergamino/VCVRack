#include "plugin.hpp"
#include "LUT_LFO.h"
#include "LUT_SQUARE.h"
#include "LUT_TRI.h"
#include "SVF.hpp"

#define TABLE_SIZE 1024

float generaRandom(float valMin, float valMax) {
    return valMin + (valMax - valMin) * rack::random::uniform();
}

float genWave(float indice, int tipo, float freq) { //tipo = 0 -> square, tipo = 1 -> tri, tipo = 2 -> sin

const float* puntaTabella = nullptr; // puntatore per la LUT
    
//forse più bello usare uno switch case
   if (tipo == 0) {
      //---sarebbe da rendere più smooth il passaggio, più LUT? Qualcosa tipo crossfade tra LUT?
        if (freq <= 100){
            puntaTabella = SQUARE_100_TABLE;
        }
        else if (freq > 100 && freq <= 400) {
            puntaTabella = SQUARE_400_TABLE;
        }
        else {
            puntaTabella = SQUARE_1600_TABLE;
        }
    }
   else if (tipo == 1) {
       if (freq <= 100){
           puntaTabella = TRI_100_TABLE;
       }
       else if (freq > 100 && freq <= 400) {
           puntaTabella = TRI_400_TABLE;
       }
       else {
           puntaTabella = TRI_1600_TABLE;
       }
   }
   else {
       puntaTabella = LFO_SINE_TABLE;
   }

    int i = floor(indice);                   //parte intera dell'input
    float err = indice - i;                  //errore frazionrio
    float sample_sx = puntaTabella[i];       //leggo LUT come se non avessi errore
    float sample_dx = (i + 1 >= TABLE_SIZE ? puntaTabella[0] : puntaTabella[i+1]); //scelgo valore dx
    
    /* Alternativa randomica ridotta, scelgo casualmente uno dei due sample
    float r = generaRandom(0.f, 1.f);
    float output = (r < err) ? sample_dx : sample_sx; */
    
    // Genero valore random in funzione dell'errore, poi normalizzo i pesi
    float a = generaRandom(0.f, 1.f - err); //se err = 0, a è casuale, ma peso1 = a/a = 1
    float b = generaRandom(0.f, err); //se err = 0, anche b = 0
    float somma = a + b; //  + 0.000001f potrebbe essere che somma = 0?
        
    //peso1 + peso2 = 1
    float peso1 = a / somma;
    float peso2 = b / somma;
        
    float output = peso1 * sample_sx + peso2 * sample_dx; //crossfade samples con pesi random(err)

    return output;

}


struct Pomodrone : Module {
	enum ParamId {
		LFO_FREQ_PARAM,
        LFO_DRIVE_PARAM,
        PITCH_PARAM,
        DETUNE_PARAM,
        GAIN1_PARAM,
        GAIN2_PARAM,
        GAIN_NOISE_PARAM,
        LFO_AMOUNT_OSC_PARAM,
        LFO_AMOUNT_NOISE_PARAM,
        LFO_AMOUNT_DETUNE_PARAM,
        FILTER_FREQ_PARAM,
        FILTER_RESO_PARAM,
        ATTACK_PARAM,
        DECAY_PARAM,
        SUSTAIN_PARAM,
        RELEASE_PARAM,
        OSC1_WAVE_PARAM,
        OSC2_WAVE_PARAM,
        ADSR_LOOP_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		PITCH_IN,
        FILTER_FREQ_IN,
        ADSR_IN,
		INPUTS_LEN
	};
	enum OutputId {
		LFO_OUT,
        MAIN_OUT,
        ADSR_OUT,
        ADSR_END_OUT,
		OUTPUTS_LEN
	};
    
	float lettore_LFO = 0.f;
    float lettore_OSC1 = 0.f;
    float lettore_OSC2 = 0.f;
    
    SVF<float> *filtro = new SVF<float>(100, 0.1); //richiamo il costruttore del filtro
    float hpf, bpf, lpf;
    float bpf_pre = 0.f;
    
    dsp::SchmittTrigger gateDetector;
    float inviluppo = 0.f;
    bool isRunning = false;
    float incr[5] = {0.f, 0.f, 0.f, 0.f, 0.f};
    int indiceIncr = 0;
    dsp::PulseGenerator fineRelease;
    dsp::PulseGenerator adsrLoop;


    Pomodrone() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN);
		configParam(LFO_FREQ_PARAM, 0.f, 0.5f, 0.f, "LFO Rate", " Hz");
        configParam(LFO_DRIVE_PARAM, 0.f, 1.f, 0.f, "LFO Drive", " %", 0, 100, 0);
        configParam(PITCH_PARAM, 50.f, 1600.f, 120.f, "OSC Pitch", " Hz");
        configParam(DETUNE_PARAM, -1.f, 1.f, 0.f, "Δ");
        
        configParam(OSC1_WAVE_PARAM, 0, 1, 0, "Tri/Square");
        configParam(OSC2_WAVE_PARAM, 0, 1, 0, "Tri/Square");
        
        configParam(GAIN1_PARAM, 0.f, 1.0f, 0.5f, "Gain OSC 1");
        configParam(GAIN2_PARAM, 0.f, 1.0f, 0.5f, "Gain OSC 2");
        configParam(GAIN_NOISE_PARAM, 0.f, 1.0f, 0.f, "Gain Noise");
        
        configParam(LFO_AMOUNT_OSC_PARAM, 0.f, 1.f, 0.f, "LFO -> Osc Amp", " %", 0, 100, 0);
        configParam(LFO_AMOUNT_NOISE_PARAM, 0.f, 1.f, 0.f, "LFO -> Noise Amp", " %", 0, 100, 0);
        configParam(LFO_AMOUNT_DETUNE_PARAM, 0.f, 1.f, 0.f, "LFO -> Detune", " %", 0, 100, 0);
        
        float freqCampionamento = APP->engine->getSampleRate();
        configParam(FILTER_FREQ_PARAM, 20.f, freqCampionamento / 6.f, freqCampionamento / 6.f, "Cutoff",
                    " Hz"); //rendere il pot logaritmico
        configParam(FILTER_RESO_PARAM, 1e-6, 20.f, 1.f, "Resonance");
        
        configParam(ATTACK_PARAM, 1e-3, 5.f, 0.5f, "Attack", " s");
        configParam(DECAY_PARAM, 1e-3, 5.f, 0.5f, "Decay", " s");
        configParam(SUSTAIN_PARAM, 1e-3, 100, 50, "Sustain", " %");
        configParam(RELEASE_PARAM, 1e-3, 5.f, 0.5f, "Release", " s");
        configParam(ADSR_LOOP_PARAM, 0, 1, 0, "Loop");
        
    
		configInput(PITCH_IN, "V/Oct");
        configInput(FILTER_FREQ_IN, "Filter Cutoff");
        configInput(ADSR_IN, "ADSR Gate");
                
		configOutput(LFO_OUT, "LFO");
        configOutput(MAIN_OUT, "Main");
        configOutput(ADSR_OUT, "ADSR");
        configOutput(ADSR_END_OUT, "End of Release");
		
	}

    void process(const ProcessArgs& args) override {
   
        //----------------leggo valori UI---------------//
        //--------LFO
        float LFO_freq = params[LFO_FREQ_PARAM].getValue();
        float LFO_fold = params[LFO_DRIVE_PARAM].getValue();
        
        //--------OSCILLATORI
        float OSC1_freq = params[PITCH_PARAM].getValue();
        float detune = params[DETUNE_PARAM].getValue();
        
        float gain1 = params[GAIN1_PARAM].getValue();
        float gain2 = params[GAIN2_PARAM].getValue();
        float gainNoise = params[GAIN_NOISE_PARAM].getValue();
        
        float oscMod = params[LFO_AMOUNT_OSC_PARAM].getValue();
        float noiseMod = params[LFO_AMOUNT_NOISE_PARAM].getValue();
        float detuneMod = params[LFO_AMOUNT_DETUNE_PARAM].getValue();
        
        bool switchOsc1 = params[OSC1_WAVE_PARAM].getValue();
        bool switchOsc2 = params[OSC2_WAVE_PARAM].getValue();
        
        float vott_in = 12.f * inputs[PITCH_IN].getVoltage(); //mi allineo +1V = + 12 semitoni
        
        //------FILTRO
        float fc = params[FILTER_FREQ_PARAM].getValue();
        if (inputs[FILTER_FREQ_IN].isConnected()) {
            fc *= std::pow(2.f, inputs[FILTER_FREQ_IN].getVoltage() / 12.f); //non so se seguire V/ott
        }
        
        float damp = 1.f / params[FILTER_RESO_PARAM].getValue();
        
        //-----ADSR

        float sustainVal = params[SUSTAIN_PARAM].getValue() * 0.01;
        float attackIncr = 1.f / (args.sampleRate * params[ATTACK_PARAM].getValue());
        float decayIncr = (1.f - sustainVal) / (args.sampleRate * params[DECAY_PARAM].getValue());
        float releaseIncr = sustainVal / (args.sampleRate * params[RELEASE_PARAM].getValue());
        float noClickIncr = inviluppo / (args.sampleRate * 0.001f);
        incr[0] = attackIncr;
        incr[1] = -decayIncr;
        incr[2] = 0.f;
        incr[3] = -releaseIncr;
        incr[4] = -noClickIncr;
        bool switchADSR = params[ADSR_LOOP_PARAM].getValue();
        float tempoADR =  (params[ATTACK_PARAM].getValue() +
                           params[DECAY_PARAM].getValue()) * 2.f;
       
        bool gateIn;
        if (inputs[ADSR_IN].isConnected()) {
           gateIn = inputs[ADSR_IN].getVoltage() >= 1.f;
        }
        else if (switchADSR) {
             gateIn = adsrLoop.process(args.sampleTime);
            if (!gateIn && !isRunning) {
                adsrLoop.trigger(tempoADR);
            }
        }
                
    
        //----------------LFO-----------------//
        lettore_LFO += LFO_freq * args.sampleTime * TABLE_SIZE;
        if (lettore_LFO >= TABLE_SIZE) {
            lettore_LFO -= TABLE_SIZE;
        }
        float sineLFO = genWave(lettore_LFO, 2, 0.f);
        
        //---Drive
        sineLFO *= 1.f + LFO_fold * 5.f;
        sineLFO = std::fmod(sineLFO + 1.f, 4.f);  // sposto su intervallo (‑4, +4)
        //come se l'oda riflettesse più volte contro dei muri
        if (sineLFO < 0.f)
            sineLFO += 4.f;          // fmod può restituire negativo
        if (sineLFO > 2.f)
            sineLFO = 4.f - sineLFO; // riflessione triangolare
        sineLFO -= 1.f;              // risultato finale: ‑1 … +1

        //----Detune
        detune += sineLFO * detuneMod;
        
        //----------OSC 1 & 2 & NOISE----------//
        //----OSC1---//
        if (inputs[PITCH_IN].isConnected()) {
            OSC1_freq *= std::pow(2.f, vott_in / 12.f);
        }
            
        lettore_OSC1 += OSC1_freq * args.sampleTime * TABLE_SIZE;
        if (lettore_OSC1 >= TABLE_SIZE) {
            lettore_OSC1 -= TABLE_SIZE;
        }
        float OSC1 = genWave(lettore_OSC1, switchOsc1, OSC1_freq) * (1.f - !switchOsc1 * 0.5f);
        
        //---OSC2---//
        float OSC2_freq = OSC1_freq * std::pow(2.f, detune);
        lettore_OSC2 += OSC2_freq * args.sampleTime * TABLE_SIZE;
        if (lettore_OSC2 >= TABLE_SIZE) {
            lettore_OSC2 -= TABLE_SIZE;
        }
        float OSC2 = genWave(lettore_OSC2, switchOsc2, OSC2_freq) * (1.f - !switchOsc2 * 0.5f);
        
        //---NOISE---//
        //abbastanza brutto così, suona troppo chiaro digital chiptune
        float rumore = (rack::random::uniform() - 0.5f) * 2.f;
        //posso aggiungere un comb intonato con OSC1, le armoniche di osc1 corrispondono con i notch
        float LFOrumore = sineLFO * 0.5f + 0.5f;
        rumore *= (1.f - noiseMod) + noiseMod * LFOrumore;

        //-----------------MIXER PRE FILTRO--------------//
        OSC1 *= (1.f - oscMod) + oscMod * -sineLFO;
        OSC2 *= (1.f - oscMod) + oscMod * -sineLFO;// * detune;
        float oscOut = OSC1 * gain1 + OSC2 * gain2 + rumore * gainNoise;
       
         float sommaGain = gain1 + gain2 + gainNoise;
        // oscOut = tanh(in * val_gain + sommaGain) - sommaGain;
         if (sommaGain > 1.0f) { //qua ci starebbe bene una non linearità tipo tanh
         oscOut /= sommaGain; //da rivedere, così praticamente normalizzo in caso di clipping
         }
       
        //---------------FILTRO SV (Chamberlin)------------//
        filtro->setCoeffs(fc, damp); //avrei potuto fare filtro.setCoeffs? No perché è un puntatore?
        filtro->process(oscOut, &hpf, &bpf, &lpf);
        float filtroOut = lpf + bpf_pre * 0.75f; // + bpf per avere più effetto whowho? che succede alla fase?
        bpf_pre = bpf;
        
        //---------------------ADSR-----------------------//
        if (inputs[ADSR_IN].isConnected() || switchADSR) {
            //problema se stacco il cavo gate in non azzero inviluppo
            //clicca sia con switch che con cavo
            if (gateDetector.process(gateIn)) {
                indiceIncr = 0;
                inviluppo = 0.f;
                isRunning = true;
            }
            if (isRunning) {
                if(gateIn) {
                    if (inviluppo >= 1.f && incr[indiceIncr] > 0.f) {
                        indiceIncr++;
                    }
                    if (inviluppo <= sustainVal && incr[indiceIncr] < 0.f) {
                        indiceIncr++;
                    }
                }
                else {
                    indiceIncr = 3;
                    if (inviluppo <= 0.f) {
                        inviluppo = 0.f;
                        isRunning = false;
                        fineRelease.trigger(1e-3);//genera trigger fine ADSR
                     }
                }
                inviluppo += incr[indiceIncr];
            }
            filtroOut *= inviluppo;
        }

      //---------------------GEN OUTPUT-----------------------//
       float mainOut = tanh(filtroOut);
       
       if (outputs[ADSR_OUT].isConnected()) {
            outputs[ADSR_OUT].setVoltage(10.f * inviluppo);
        }
       bool EndOfRel = fineRelease.process(args.sampleTime);
       if (outputs[ADSR_END_OUT].isConnected()) {
             outputs[ADSR_END_OUT].setVoltage(EndOfRel ? 10.f : 0.f);
         }
        
       if (outputs[LFO_OUT].isConnected()) {
            outputs[LFO_OUT].setVoltage(5.f * sineLFO);
        }
       if (outputs[MAIN_OUT].isConnected()) {
            outputs[MAIN_OUT].setVoltage(5.f * mainOut);
        }
        //https://vcvrack.com/manual/VoltageStandards
        

	}
}; //end struct module


struct PomodroneWidget : ModuleWidget {
    PomodroneWidget(Pomodrone* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Pomodrone.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(47.80, 17.846)), module, Pomodrone::LFO_FREQ_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(36.598, 24.562)), module, Pomodrone::LFO_DRIVE_PARAM));
        
        addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(19.573, 46.091)), module, Pomodrone::PITCH_PARAM));
        
        addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(66.004, 64.128)), module, Pomodrone::DETUNE_PARAM));
        
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(91.276, 63.857)), module, Pomodrone::GAIN1_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(83.703, 81.813)), module, Pomodrone::GAIN2_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(66.253, 89.316)), module, Pomodrone::GAIN_NOISE_PARAM));
        
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(65.929, 39.232)), module, Pomodrone::LFO_AMOUNT_OSC_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(41.657, 64.255)), module, Pomodrone::LFO_AMOUNT_NOISE_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(48.506, 47.094)), module, Pomodrone::LFO_AMOUNT_DETUNE_PARAM));
        
        addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(39.522, 105.833)), module, Pomodrone::FILTER_FREQ_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(22.782, 88.298)), module, Pomodrone::FILTER_RESO_PARAM));
        
        addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(87.570, 19.315)), module, Pomodrone::ATTACK_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(99.071, 27.239)), module, Pomodrone::DECAY_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(108.957, 38.414)), module, Pomodrone::SUSTAIN_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(114.221, 52.513)), module, Pomodrone::RELEASE_PARAM));
        
        addParam(createParam<CKSS>(mm2px(Vec(95.298, 34.983)), module, Pomodrone::ADSR_LOOP_PARAM));
        addParam(createParam<CKSS>(mm2px(Vec(98.207, 60.287)), module, Pomodrone::OSC1_WAVE_PARAM));
        addParam(createParam<CKSS>(mm2px(Vec(90.704, 78.461)), module, Pomodrone::OSC2_WAVE_PARAM));


        
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(101.300, 99.240)), module, Pomodrone::PITCH_IN));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(82.773, 110.970)), module, Pomodrone::FILTER_FREQ_IN));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(112.200, 82.428)), module,
            Pomodrone::ADSR_IN));
    
        
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(112.522, 109.227)), module, Pomodrone::MAIN_OUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(103.372, 116.529)), module, Pomodrone::LFO_OUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(120.596, 100.050)), module, Pomodrone::ADSR_OUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(124.737, 56.097)), module, Pomodrone::ADSR_END_OUT));

	}
};


Model* modelPomodrone = createModel<Pomodrone, PomodroneWidget>("Pomodrone");
